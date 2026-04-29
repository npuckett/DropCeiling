#ifndef DROPCEILING_STANDALONE_H
#define DROPCEILING_STANDALONE_H

#include <Arduino.h>
#include "DropCeilingInput.h"

class DropCeilingStandaloneInput : public DropCeilingInput {
public:
  DropCeilingStandaloneInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) override;
  bool connected() const override;
  IPAddress localIP() const override;

private:
  DropCeilingConfig _config;
  bool _pendingInitialValues;
};

#endif
