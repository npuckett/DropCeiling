#ifndef DROPCEILING_DAC_H
#define DROPCEILING_DAC_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DACX578.h>
#include "DropCeilingTypes.h"

class DropCeilingDAC {
public:
  DropCeilingDAC();

  bool begin(const DropCeilingConfig &config);
  bool writeChannel(uint8_t channelIndex, uint8_t value);
  bool writeAll(const uint8_t values[DROPCEILING_CHANNELS]);
  bool ready() const;

private:
  Adafruit_DACX578 _dac;
  bool _ready;
  uint16_t _maxValue;
  uint8_t _lastValues[DROPCEILING_CHANNELS];

  uint16_t toDacValue(uint8_t value) const;
};

#endif
