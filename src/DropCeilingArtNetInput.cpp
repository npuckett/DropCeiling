#include "DropCeilingArtNetInput.h"

namespace {
// Packets handled per read() call, so a poll and a frame that arrive together are both taken.
const uint8_t PACKETS_PER_READ = 4;
}

bool DropCeilingArtNetInput::read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
  uint16_t levels[DROPCEILING_CHANNELS];
  for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
    levels[i] = DropCeilingLevels::fromByte(values[i]);
  }
  bool received = readLevels(levels, changed);
  for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
    values[i] = DropCeilingLevels::toByte(levels[i]);
  }
  return received;
}

bool DropCeilingArtNetInput::readLevels(uint16_t levels[DROPCEILING_CHANNELS], bool &changed) {
  changed = false;
  if (!_ready) {
    return false;
  }

  bool received = false;
  for (uint8_t n = 0; n < PACKETS_PER_READ; n++) {
    int packetSize = udp().parsePacket();
    if (packetSize <= 0) {
      break;
    }

    int length = udp().read(_buffer, DropCeilingArtNet::MaxPacketSize);
    if (length <= 0) {
      continue;
    }

    DropCeilingArtNet::ParseResult result = DropCeilingArtNet::parsePacket(_buffer, length, _config, levels);
    if (result.type == DropCeilingArtNet::PacketPoll) {
      sendPollReply(udp().remoteIP(), udp().remotePort());
    } else if (result.type == DropCeilingArtNet::PacketDmx) {
      _lastReceive = millis();
      _frames++;
      changed = changed || result.changed;
    }
    received = received || result.type != DropCeilingArtNet::PacketIgnored;
  }
  return received;
}

bool DropCeilingArtNetInput::receivedRecently() const {
  return _frames > 0 && millis() - _lastReceive < _config.connectionTimeoutMs;
}

// The reply goes back to the address and port the poll came from. A controller polling from 6454 gets
// it on 6454 as the standard expects; a tool polling from any other port still hears it.
void DropCeilingArtNetInput::sendPollReply(const IPAddress &to, uint16_t port) {
  uint8_t reply[DropCeilingArtNet::PollReplySize];
  char report[52] = {0};
  if (_config.nodeReportFn != nullptr) {
    _config.nodeReportFn(report, sizeof(report));
  }
  _replies++;
  size_t length = DropCeilingArtNet::buildPollReply(reply, sizeof(reply), _config, localIP(), _live, _replies,
                                                    _config.nodeReportFn != nullptr ? report : nullptr);
  if (length == 0) {
    return;
  }
  udp().beginPacket(to, port);
  udp().write(reply, length);
  udp().endPacket();
}
