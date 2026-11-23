#include "DropCeilingArtNet.h"

#include <stdio.h>
#include <string.h>

namespace {
const char ARTNET_HEADER[] = "Art-Net";
const uint16_t OP_DMX = 0x5000;
const uint16_t OP_POLL = 0x2000;

bool hasArtNetHeader(const uint8_t *data, uint16_t length) {
  return length >= 10 && memcmp(data, ARTNET_HEADER, 8) == 0;
}

bool setLevel(uint16_t levels[DROPCEILING_CHANNELS], uint8_t channel, uint16_t level) {
  if (levels[channel] == level) {
    return false;
  }
  levels[channel] = level;
  return true;
}
}

DropCeilingArtNet::ParseResult DropCeilingArtNet::parsePacket(const uint8_t *data, uint16_t length, const DropCeilingConfig &config, uint16_t levels[DROPCEILING_CHANNELS]) {
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

  uint16_t universe = data[14] | ((data[15] & 0x7F) << 8);
  if (universe != config.artnetUniverse) {
    return result;
  }

  uint16_t dmxLength = (data[16] << 8) | data[17];
  uint16_t available = length - 18;
  if (dmxLength > available) {
    dmxLength = available;
  }
  const uint8_t *slots = data + 18;
  result.type = PacketDmx;

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    if (config.artnetResolution == 16) {
      uint32_t hi = static_cast<uint32_t>(config.artnetStartChannel) + 2u * channel;
      if (hi + 1 < dmxLength) {
        uint16_t level = static_cast<uint16_t>((slots[hi] << 8) | slots[hi + 1]);
        result.changed = setLevel(levels, channel, level) || result.changed;
      }
    } else {
      uint32_t slot = static_cast<uint32_t>(config.artnetStartChannel) + channel;
      if (slot < dmxLength) {
        result.changed = setLevel(levels, channel, DropCeilingLevels::fromByte(slots[slot])) || result.changed;
      }
    }
  }

  return result;
}

size_t DropCeilingArtNet::buildPollReply(uint8_t *reply, size_t replySize, const DropCeilingConfig &config, const IPAddress &localIp,
                                         bool live, uint16_t replies) {
  if (replySize < PollReplySize) {
    return 0;
  }

  // Offsets from the Art-Net 4 ArtPollReply table.
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

  reply[18] = (config.artnetUniverse >> 8) & 0x7F;  // NetSwitch
  reply[19] = (config.artnetUniverse >> 4) & 0x0F;  // SubSwitch

  strncpy(reinterpret_cast<char *>(&reply[26]), config.nodeShortName, 17);
  strncpy(reinterpret_cast<char *>(&reply[44]), config.nodeLongName, 63);

  // NodeReport: "#xxxx [yyyy] text". 0001 is RcPowerOk.
  snprintf(reinterpret_cast<char *>(&reply[108]), 64, "#0001 [%04u] %s", static_cast<unsigned>(replies % 10000),
           config.nodeReport == nullptr ? "" : config.nodeReport);

  reply[172] = 0;     // NumPortsHi
  reply[173] = 1;     // NumPortsLo
  reply[174] = 0x80;  // PortTypes[0]: output, DMX512
  reply[182] = live ? 0x80 : 0x00;  // GoodOutputA[0]: data being transmitted
  reply[190] = config.artnetUniverse & 0x0F;  // SwOut[0]

  memcpy(&reply[201], config.mac, 6);  // MAC
  reply[207] = localIp[0];             // BindIp
  reply[208] = localIp[1];
  reply[209] = localIp[2];
  reply[210] = localIp[3];
  reply[211] = 1;  // BindIndex
  // Status2: bit 3 15-bit port addresses, bit 2 DHCP capable, bit 1 address set by DHCP.
  reply[212] = 0x08 | 0x04 | (config.useDhcp ? 0x02 : 0x00);

  return PollReplySize;
}
