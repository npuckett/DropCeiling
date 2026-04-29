#ifndef DROPCEILING_SERIAL_H
#define DROPCEILING_SERIAL_H

#include <Arduino.h>
#include "DropCeilingInput.h"

class DropCeilingSerialInput : public DropCeilingInput {
public:
  DropCeilingSerialInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) override;
  bool connected() const override;
  IPAddress localIP() const override;

private:
  DropCeilingConfig _config;
  char _line[128];
  uint8_t _lineLength;
  unsigned long _lastReceive;

  bool parseLine(uint8_t values[DROPCEILING_CHANNELS], bool &changed);
};

#endif
