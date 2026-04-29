#include "DropCeilingDAC.h"

DropCeilingDAC::DropCeilingDAC()
  : _dac(10), _ready(false), _maxValue(1023) {
  memset(_lastValues, 0xFF, sizeof(_lastValues));
}

bool DropCeilingDAC::begin(const DropCeilingConfig &config) {
  _maxValue = config.dacMaxValue;

#if defined(ARDUINO_ARCH_ESP32)
  if (config.i2cSdaPin >= 0 && config.i2cSclPin >= 0) {
    Wire.begin(config.i2cSdaPin, config.i2cSclPin);
  } else {
    Wire.begin();
  }
#else
  Wire.begin();
#endif

  _dac = Adafruit_DACX578(config.dacResolution);
  _ready = _dac.begin(config.dacAddress, &Wire);
  if (!_ready) {
    return false;
  }

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    _dac.writeAndUpdateChannelValue(channel, 0);
    _lastValues[channel] = 0;
  }
  return true;
}

bool DropCeilingDAC::writeChannel(uint8_t channelIndex, uint8_t value) {
  if (!_ready || channelIndex >= DROPCEILING_CHANNELS) {
    return false;
  }

  if (_lastValues[channelIndex] == value) {
    return true;
  }

  _dac.writeAndUpdateChannelValue(channelIndex, toDacValue(value));
  _lastValues[channelIndex] = value;
  return true;
}

bool DropCeilingDAC::writeAll(const uint8_t values[DROPCEILING_CHANNELS]) {
  bool ok = true;
  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    ok = writeChannel(channel, values[channel]) && ok;
  }
  return ok;
}

bool DropCeilingDAC::ready() const {
  return _ready;
}

uint16_t DropCeilingDAC::toDacValue(uint8_t value) const {
  return map(value, 0, 255, 0, _maxValue);
}
