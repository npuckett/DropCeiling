#ifndef DROPCEILING_ARTNET_H
#define DROPCEILING_ARTNET_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingTypes.h"

namespace DropCeilingArtNet {
  static const uint16_t MaxPacketSize = 530;
  static const uint16_t PollReplySize = 239;

  enum PacketType {
    PacketIgnored,
    PacketDmx,
    PacketPoll
  };

  struct ParseResult {
    PacketType type;
    bool changed;
  };

  ParseResult parsePacket(const uint8_t *data, uint16_t length, const DropCeilingConfig &config, uint8_t values[DROPCEILING_CHANNELS]);
  size_t buildPollReply(uint8_t *reply, size_t replySize, const DropCeilingConfig &config, const IPAddress &localIp);
}

#endif
