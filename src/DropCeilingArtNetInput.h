#ifndef DROPCEILING_ARTNET_INPUT_H
#define DROPCEILING_ARTNET_INPUT_H

#include <Arduino.h>
#include <Udp.h>
#include "DropCeilingArtNet.h"
#include "DropCeilingInput.h"

// What Ethernet and WiFi Art-Net share: reading ArtDmx into 16-bit levels, counting frames, and answering
// ArtPoll to whoever asked.
class DropCeilingArtNetInput : public DropCeilingInput {
public:
  bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) override;
  bool providesLevels() const override { return true; }
  bool readLevels(uint16_t levels[DROPCEILING_CHANNELS], bool &changed) override;
  bool countsFrames() const override { return true; }
  uint32_t frames() const override { return _frames; }

protected:
  DropCeilingConfig _config;
  unsigned long _lastReceive = 0;
  uint32_t _frames = 0;
  uint16_t _replies = 0;
  bool _ready = false;
  uint8_t _buffer[DropCeilingArtNet::MaxPacketSize];

  virtual UDP &udp() = 0;
  bool receivedRecently() const;
  void sendPollReply(const IPAddress &to, uint16_t port);
};

#endif
