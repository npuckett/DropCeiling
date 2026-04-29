#include "DropCeilingArtNet.h"

namespace {
const char ARTNET_HEADER[] = "Art-Net";
const uint16_t OP_DMX = 0x5000;
const uint16_t OP_POLL = 0x2000;

bool hasArtNetHeader(const uint8_t *data, uint16_t length) {
  return length >= 10 && memcmp(data, ARTNET_HEADER, 8) == 0;
}
}

DropCeilingArtNet::ParseResult DropCeilingArtNet::parsePacket(const uint8_t *data, uint16_t length, const DropCeilingConfig &config, uint8_t values[DROPCEILING_CHANNELS]) {
  ParseResult result = {PacketIgnored, false};

  if (!hasArtNetHeader(data, length)) {
    return result;
  }

  uint16_t opcode = data[8] | (data[9] << 8);
  if (opcode == OP_POLL) {
    result.type = PacketPoll;
    return result;
  }

  if (opcode != OP_DMX || length < 18) {
    return result;
  }

  uint16_t universe = data[14] | (data[15] << 8);
  if (universe != config.artnetUniverse) {
    return result;
  }

  uint16_t dmxLength = (data[16] << 8) | data[17];
  result.type = PacketDmx;

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    uint16_t sourceChannel = config.artnetStartChannel + channel;
    uint16_t packetIndex = 18 + sourceChannel;
    if (packetIndex < length && sourceChannel < dmxLength) {
      uint8_t value = data[packetIndex];
      if (values[channel] != value) {
        values[channel] = value;
        result.changed = true;
      }
    }
  }

  return result;
}

size_t DropCeilingArtNet::buildPollReply(uint8_t *reply, size_t replySize, const DropCeilingConfig &config, const IPAddress &localIp) {
  if (replySize < PollReplySize) {
    return 0;
  }

  memset(reply, 0, PollReplySize);
  memcpy(reply, "Art-Net\0", 8);

  reply[8] = 0x00;
  reply[9] = 0x21;

  reply[10] = localIp[0];
  reply[11] = localIp[1];
  reply[12] = localIp[2];
  reply[13] = localIp[3];

  reply[14] = config.artnetPort & 0xFF;
  reply[15] = config.artnetPort >> 8;

  reply[16] = config.firmwareVersionHi;
  reply[17] = config.firmwareVersionLo;

  reply[18] = (config.artnetUniverse >> 8) & 0x7F;
  reply[19] = (config.artnetUniverse >> 4) & 0x0F;

  strncpy(reinterpret_cast<char *>(&reply[26]), config.nodeShortName, 17);
  strncpy(reinterpret_cast<char *>(&reply[44]), config.nodeLongName, 63);

  reply[172] = 0;
  reply[173] = 1;
  reply[174] = 0x80;
  reply[182] = 0x80;
  reply[190] = config.artnetUniverse & 0x0F;

  return PollReplySize;
}
