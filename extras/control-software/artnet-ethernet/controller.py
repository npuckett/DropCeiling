"""
Drop Ceiling ArtNet Controller
Sends 8 channels of ArtNet DMX data via UDP.
Broadcasts ArtPoll on all network interfaces to discover nodes.
"""

import math
import sys
import socket
import struct

import netifaces
from PySide6.QtCore import Qt, QTimer, Slot
from PySide6.QtGui import QFont
from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QSlider, QPushButton, QFrame,
)

ARTNET_PORT = 6454
ARTNET_HEADER = b"Art-Net\x00"
OP_DMX = 0x5000
OP_POLL = 0x2000
OP_POLL_REPLY = 0x2100
PROTOCOL_VERSION = 14
NUM_CHANNELS = 8


def get_broadcast_addresses():
    """Return a list of broadcast addresses for all active network interfaces."""
    addrs = []
    for iface in netifaces.interfaces():
        af_inet = netifaces.ifaddresses(iface).get(netifaces.AF_INET, [])
        for info in af_inet:
            bcast = info.get("broadcast")
            if bcast and bcast != "127.255.255.255":
                addrs.append(bcast)
    return addrs if addrs else ["255.255.255.255"]


class ArtNetController(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Drop Ceiling ArtNet Controller")
        self.setFixedWidth(480)

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        except AttributeError:
            pass
        self.sock.bind(("", ARTNET_PORT))
        self.sock.setblocking(False)

        self.target_ip = ""
        self.universe = 0
        self.sequence = 0
        self.connected = False
        self.sliders = []
        self.value_labels = []
        self.values = [0] * NUM_CHANNELS

        self._poll_attempts = 0
        self._poll_timer = QTimer(self)
        self._poll_timer.timeout.connect(self._poll_for_reply)

        self._wave_timer = QTimer(self)
        self._wave_timer.timeout.connect(self._wave_tick)
        self._wave_phase = 0.0
        self._wave_running = False

        self._build_ui()

    def _build_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(12, 12, 12, 12)
        root.setSpacing(8)

        # -- Status bar --
        status_row = QHBoxLayout()
        self.status_dot = QLabel()
        self.status_dot.setFixedSize(14, 14)
        self._set_dot("red")
        status_row.addWidget(self.status_dot)

        self.status_label = QLabel("Disconnected")
        self.status_label.setFont(QFont("Helvetica", 12))
        status_row.addWidget(self.status_label)
        status_row.addStretch()

        self.connect_btn = QPushButton("Connect")
        self.connect_btn.setFixedWidth(100)
        self.connect_btn.clicked.connect(self._toggle_connect)
        status_row.addWidget(self.connect_btn)
        root.addLayout(status_row)

        # -- Separator --
        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        line.setFrameShadow(QFrame.Sunken)
        root.addWidget(line)

        # -- Channel sliders --
        for i in range(NUM_CHANNELS):
            row = QHBoxLayout()
            row.setSpacing(8)

            label = QLabel(f"CH {i + 1}")
            label.setFixedWidth(40)
            label.setFont(QFont("Helvetica", 11))
            row.addWidget(label)

            slider = QSlider(Qt.Horizontal)
            slider.setRange(0, 255)
            slider.setValue(0)
            slider.valueChanged.connect(lambda val, ch=i: self._on_slider(ch, val))
            row.addWidget(slider, 1)
            self.sliders.append(slider)

            val_label = QLabel("0")
            val_label.setFixedWidth(32)
            val_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
            val_label.setFont(QFont("Helvetica", 11, QFont.Bold))
            row.addWidget(val_label)
            self.value_labels.append(val_label)

            root.addLayout(row)

        # -- Action buttons --
        btn_row = QHBoxLayout()
        zero_btn = QPushButton("All Zero")
        zero_btn.clicked.connect(self._all_zero)
        btn_row.addWidget(zero_btn)

        max_btn = QPushButton("All Max")
        max_btn.clicked.connect(self._all_max)
        btn_row.addWidget(max_btn)

        self.wave_btn = QPushButton("Wave")
        self.wave_btn.clicked.connect(self._toggle_wave)
        btn_row.addWidget(self.wave_btn)

        btn_row.addStretch()
        root.addLayout(btn_row)

    # ---- helpers ----

    def _set_dot(self, color):
        self.status_dot.setStyleSheet(
            f"background-color: {color}; border-radius: 7px; min-width: 14px; min-height: 14px;"
        )

    def _set_status(self, text, color):
        self.status_label.setText(text)
        self._set_dot(color)

    # ---- connection ----

    @Slot()
    def _toggle_connect(self):
        if self.connected:
            self._disconnect()
        else:
            self.connected = True
            self.target_ip = ""
            self._poll_attempts = 0
            self.connect_btn.setText("Disconnect")
            self._set_status("Scanning all interfaces...", "orange")
            self._send_artpoll()
            self._poll_timer.start(100)

    def _disconnect(self):
        self._poll_timer.stop()
        self.connected = False
        self.target_ip = ""
        self.connect_btn.setText("Connect")
        self._set_status("Disconnected", "red")

    def _send_artpoll(self):
        packet = bytearray()
        packet += ARTNET_HEADER
        packet += struct.pack("<H", OP_POLL)
        packet += struct.pack(">H", PROTOCOL_VERSION)
        packet += bytes([0x00, 0x00])

        for bcast in get_broadcast_addresses():
            try:
                self.sock.sendto(packet, (bcast, ARTNET_PORT))
            except Exception:
                pass

    @Slot()
    def _poll_for_reply(self):
        if not self.connected:
            self._poll_timer.stop()
            return

        # Drain socket for ArtPollReply
        while True:
            try:
                data, addr = self.sock.recvfrom(1024)
            except BlockingIOError:
                break
            except Exception:
                break

            if len(data) < 10 or data[:8] != ARTNET_HEADER:
                continue
            opcode = struct.unpack("<H", data[8:10])[0]
            if opcode == OP_POLL_REPLY:
                node_ip = addr[0]
                node_name = ""
                if len(data) >= 44:
                    node_name = data[26:44].split(b"\x00")[0].decode("ascii", errors="ignore")
                self.target_ip = node_ip
                label = f"Connected: {node_name} ({node_ip})" if node_name else f"Connected: {node_ip}"
                self._set_status(label, "green")
                self._poll_timer.stop()
                self._send_artnet()
                return

        self._poll_attempts += 1
        if self._poll_attempts % 10 == 0:
            self._send_artpoll()
        if self._poll_attempts >= 30:
            self._poll_timer.stop()
            self._set_status("No ArtNet nodes found", "red")
            self._disconnect()

    # ---- sliders ----

    def _on_slider(self, ch, val):
        self.values[ch] = val
        self.value_labels[ch].setText(str(val))
        if self.connected and self.target_ip:
            self._send_artnet()

    @Slot()
    def _all_zero(self):
        for i in range(NUM_CHANNELS):
            self.sliders[i].setValue(0)
        if self.connected and self.target_ip:
            self._send_artnet()

    @Slot()
    def _all_max(self):
        for i in range(NUM_CHANNELS):
            self.sliders[i].setValue(255)
        if self.connected and self.target_ip:
            self._send_artnet()

    # ---- wave preset ----

    @Slot()
    def _toggle_wave(self):
        if self._wave_running:
            self._wave_timer.stop()
            self._wave_running = False
            self.wave_btn.setText("Wave")
        else:
            self._wave_phase = 0.0
            self._wave_running = True
            self.wave_btn.setText("Stop Wave")
            self._wave_timer.start(33)  # ~30 fps

    @Slot()
    def _wave_tick(self):
        self._wave_phase += 0.08
        for i in range(NUM_CHANNELS):
            val = int(127.5 + 127.5 * math.sin(self._wave_phase + i * (2 * math.pi / NUM_CHANNELS)))
            self.sliders[i].setValue(val)
        if self.connected and self.target_ip:
            self._send_artnet()

    # ---- ArtNet DMX ----

    def _send_artnet(self):
        if not self.connected or not self.target_ip:
            return

        self.sequence = (self.sequence % 255) + 1

        dmx_data = bytes(self.values)
        dmx_length = len(dmx_data)
        if dmx_length % 2 != 0:
            dmx_data += b"\x00"
            dmx_length += 1

        packet = bytearray()
        packet += ARTNET_HEADER
        packet += struct.pack("<H", OP_DMX)
        packet += struct.pack(">H", PROTOCOL_VERSION)
        packet += bytes([self.sequence])
        packet += bytes([0])
        packet += struct.pack("<H", self.universe)
        packet += struct.pack(">H", dmx_length)
        packet += dmx_data

        try:
            self.sock.sendto(packet, (self.target_ip, ARTNET_PORT))
        except Exception as e:
            self._set_status(f"Send error: {e}", "red")

    def closeEvent(self, event):
        self.sock.close()
        event.accept()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = ArtNetController()
    win.show()
    sys.exit(app.exec())
