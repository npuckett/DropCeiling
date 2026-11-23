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

  // levels are 16-bit (DropCeilingLevels.h). In 8-bit mode a slot value v becomes v * 257.
  // Only channels whose slots are present in the frame change.
  ParseResult parsePacket(const uint8_t *data, uint16_t length, const DropCeilingConfig &config, uint16_t levels[DROPCEILING_CHANNELS]);

  // live sets GoodOutput's "data being transmitted" bit. replies counts replies sent, for the node report.
  size_t buildPollReply(uint8_t *reply, size_t replySize, const DropCeilingConfig &config, const IPAddress &localIp,
                        bool live, uint16_t replies);
}

#endif
