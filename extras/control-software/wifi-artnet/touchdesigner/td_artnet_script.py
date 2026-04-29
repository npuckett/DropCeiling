"""
Drop Ceiling ArtNet Controller - TouchDesigner Module
Paste into a Text DAT named 'text_artnet'. Use as: op('text_artnet').module

Provides connect(), disconnect(), send_dmx(), get_status(), is_connected().
"""

import socket
import struct
import time

ARTNET_PORT = 6454
ARTNET_HEADER = b"Art-Net\x00"
OP_DMX = 0x5000
OP_POLL = 0x2000
OP_POLL_REPLY = 0x2100
PROTOCOL_VERSION = 14
NUM_CHANNELS = 8

_sock = None
_target_ip = ""
_sequence = 0
_connected = False
_status = "Disconnected"


def _init_socket():
    global _sock
    if _sock is None:
        _sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        _sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        _sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        _sock.settimeout(0.0)
        try:
            _sock.bind(("", ARTNET_PORT))
        except OSError:
            pass


def connect(target_ip="192.168.1.200"):
    """Discover an ArtNet node via ArtPoll, or fall back to target_ip."""
    global _target_ip, _connected, _status
    _init_socket()

    # Broadcast ArtPoll
    poll = bytearray()
    poll += ARTNET_HEADER
    poll += struct.pack("<H", OP_POLL)
    poll += struct.pack(">H", PROTOCOL_VERSION)
    poll += bytes([0x00, 0x00])

    try:
        _sock.sendto(poll, ("255.255.255.255", ARTNET_PORT))
    except Exception:
        pass

    # Listen for ArtPollReply (up to 2 seconds)
    deadline = time.time() + 2.0
    while time.time() < deadline:
        try:
            data, addr = _sock.recvfrom(1024)
            if len(data) >= 10 and data[:8] == ARTNET_HEADER:
                opcode = struct.unpack("<H", data[8:10])[0]
                if opcode == OP_POLL_REPLY:
                    _target_ip = addr[0]
                    _connected = True
                    name = ""
                    if len(data) >= 44:
                        name = data[26:44].split(b"\x00")[0].decode(
                            "ascii", errors="ignore"
                        )
                    _status = (
                        f"Connected: {name} ({_target_ip})"
                        if name
                        else f"Connected: {_target_ip}"
                    )
                    print(_status)
                    return True
        except (BlockingIOError, OSError):
            pass
        time.sleep(0.05)

    # No reply -- fall back to provided IP
    _target_ip = target_ip
    _connected = True
    _status = f"Sending to {_target_ip} (no discovery reply)"
    print(_status)
    return True


def disconnect():
    """Stop sending and reset state."""
    global _connected, _status, _target_ip
    _connected = False
    _target_ip = ""
    _status = "Disconnected"


def send_dmx(values):
    """Send an ArtDmx packet. values = list of up to 8 ints (0-255)."""
    global _sequence
    if not _connected or not _target_ip:
        return

    _init_socket()
    _sequence = (_sequence % 255) + 1

    dmx_data = bytes([max(0, min(255, int(v))) for v in values[:NUM_CHANNELS]])
    while len(dmx_data) < NUM_CHANNELS:
        dmx_data += b"\x00"
    dmx_length = len(dmx_data)
    if dmx_length % 2 != 0:
        dmx_data += b"\x00"
        dmx_length += 1

    packet = bytearray()
    packet += ARTNET_HEADER
    packet += struct.pack("<H", OP_DMX)
    packet += struct.pack(">H", PROTOCOL_VERSION)
    packet += bytes([_sequence])
    packet += bytes([0])
    packet += struct.pack("<H", 0)  # universe 0
    packet += struct.pack(">H", dmx_length)
    packet += dmx_data

    try:
        _sock.sendto(packet, (_target_ip, ARTNET_PORT))
    except Exception as e:
        print(f"ArtNet send error: {e}")


def get_status():
    """Return current connection status string."""
    return _status


def is_connected():
    """Return True if connected to a node."""
    return _connected
