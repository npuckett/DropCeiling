#ifndef DROPCEILING_DISPLAY_H
#define DROPCEILING_DISPLAY_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingTypes.h"

class DropCeilingDisplay {
public:
  virtual ~DropCeilingDisplay() {}
  virtual bool begin(const DropCeilingConfig &config) = 0;
  virtual void drawStaticUI(const IPAddress &localIP, uint16_t universe, const uint8_t values[DROPCEILING_CHANNELS]) = 0;
  virtual void setConnectionStatus(bool connected) = 0;
  virtual void updateChannels(const uint8_t values[DROPCEILING_CHANNELS]) = 0;
};

#endif
