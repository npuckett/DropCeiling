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
};

#endif
