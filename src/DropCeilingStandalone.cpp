#include "DropCeilingStandalone.h"

DropCeilingStandaloneInput::DropCeilingStandaloneInput()
  : _pendingInitialValues(false) {
}

bool DropCeilingStandaloneInput::begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) {
  (void)currentValues;
  _config = config;
  _pendingInitialValues = true;
  return true;
}

bool DropCeilingStandaloneInput::read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
  changed = false;
  if (!_pendingInitialValues) {
    return false;
  }

  _pendingInitialValues = false;
  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    if (values[channel] != _config.standaloneValues[channel]) {
      values[channel] = _config.standaloneValues[channel];
      changed = true;
    }
  }
  return true;
}

bool DropCeilingStandaloneInput::connected() const {
  return true;
}

IPAddress DropCeilingStandaloneInput::localIP() const {
  return IPAddress(0, 0, 0, 0);
}
