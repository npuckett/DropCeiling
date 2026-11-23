#ifndef DROPCEILING_DAC_H
#define DROPCEILING_DAC_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DACX578.h>
#include "DropCeilingTypes.h"

class DropCeilingDAC {
public:
  DropCeilingDAC();

  // Reads back what each channel already holds into startLevels (16-bit), and leaves it there, so a
  // restart of the processor does not snap the outputs to 0. After power-up the DAC holds 0.
  bool begin(const DropCeilingConfig &config, uint16_t startLevels[DROPCEILING_CHANNELS]);
  bool writeLevel(uint8_t channelIndex, uint16_t level);
  bool writeLevels(const uint16_t levels[DROPCEILING_CHANNELS]);
  bool ready() const;

  // I2C writes that failed since begin(). A failed write is tried again on the next update.
  uint32_t errors() const { return _errors; }
  uint16_t code(uint8_t channelIndex) const;

private:
  Adafruit_DACX578 _dac;
  bool _ready;
  uint8_t _bits;
  uint16_t _lastCodes[DROPCEILING_CHANNELS];
  bool _known[DROPCEILING_CHANNELS];
  uint32_t _errors;
};

#endif
