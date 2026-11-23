#include "DropCeilingDAC.h"

DropCeilingDAC::DropCeilingDAC()
  : _dac(10), _ready(false), _bits(10), _errors(0) {
  memset(_lastCodes, 0, sizeof(_lastCodes));
  memset(_known, 0, sizeof(_known));
}

bool DropCeilingDAC::begin(const DropCeilingConfig &config, uint16_t startLevels[DROPCEILING_CHANNELS]) {
  _bits = config.dacResolution;
  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    startLevels[channel] = 0;
    _known[channel] = false;
  }

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
    uint16_t code = 0;
    bool read = false;
    for (uint8_t attempt = 0; attempt < 3 && !read; attempt++) {
      read = _dac.readChannelValue(channel, &code);
    }
    if (read) {
      _lastCodes[channel] = code;
      _known[channel] = true;
      // Back to a 16-bit level: the code in the top bits.
      startLevels[channel] = _bits >= 16 ? code : static_cast<uint16_t>(code << (16 - _bits));
    } else {
      _errors++;
    }
  }
  return true;
}

bool DropCeilingDAC::writeLevel(uint8_t channelIndex, uint16_t level) {
  if (!_ready || channelIndex >= DROPCEILING_CHANNELS) {
    return false;
  }

  uint16_t code = DropCeilingLevels::toCode(level, _bits);
  if (_known[channelIndex] && _lastCodes[channelIndex] == code) {
    return true;
  }

  if (!_dac.writeAndUpdateChannelValue(channelIndex, code)) {
    _errors++;
    _known[channelIndex] = false;  // try again next time
    return false;
  }
  _lastCodes[channelIndex] = code;
  _known[channelIndex] = true;
  return true;
}

bool DropCeilingDAC::writeLevels(const uint16_t levels[DROPCEILING_CHANNELS]) {
  bool ok = true;
  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    ok = writeLevel(channel, levels[channel]) && ok;
  }
  return ok;
}

bool DropCeilingDAC::ready() const {
  return _ready;
}

uint16_t DropCeilingDAC::code(uint8_t channelIndex) const {
  return channelIndex < DROPCEILING_CHANNELS ? _lastCodes[channelIndex] : 0;
}
