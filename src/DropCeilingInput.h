#ifndef DROPCEILING_INPUT_H
#define DROPCEILING_INPUT_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingTypes.h"

class DropCeilingInput {
public:
  virtual ~DropCeilingInput() {}
  virtual bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) = 0;
  virtual bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) = 0;
  virtual bool connected() const = 0;
  virtual IPAddress localIP() const = 0;

  // Inputs that carry 16-bit levels (Art-Net) override these. The others are read through read() and
  // their bytes are widened by DropCeiling.
  virtual bool providesLevels() const { return false; }
  virtual bool readLevels(uint16_t levels[DROPCEILING_CHANNELS], bool &changed) {
    (void)levels;
    changed = false;
    return false;
  }

  // Frames accepted since begin(). Inputs that count frames get the loss-of-signal fade.
  virtual bool countsFrames() const { return false; }
  virtual uint32_t frames() const { return 0; }

  // Called once per DropCeiling::read() whether or not a packet came, for housekeeping such as DHCP.
  virtual void maintain() {}

  // The output state, for replies that report it (ArtPollReply's GoodOutput).
  void setLive(bool live) { _live = live; }

protected:
  bool _live = false;
};

#endif
