#include "DropCeilingSerial.h"

DropCeilingSerialInput::DropCeilingSerialInput()
  : _lineLength(0), _lastReceive(0) {
  memset(_line, 0, sizeof(_line));
}

bool DropCeilingSerialInput::begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) {
  (void)currentValues;
  _config = config;
  Serial.begin(_config.serialBaud);
  _lineLength = 0;
  return true;
}

bool DropCeilingSerialInput::read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
  changed = false;
  bool receivedLine = false;

  while (Serial.available() > 0) {
    char incoming = static_cast<char>(Serial.read());
    if (incoming == '\r') {
      continue;
    }
    if (incoming == '\n') {
      _line[_lineLength] = '\0';
      receivedLine = parseLine(values, changed) || receivedLine;
      _lineLength = 0;
      continue;
    }
    if (_lineLength < sizeof(_line) - 1) {
      _line[_lineLength++] = incoming;
    } else {
      _lineLength = 0;
    }
  }

  if (changed) {
    _lastReceive = millis();
  }
  return receivedLine;
}

bool DropCeilingSerialInput::connected() const {
  return _lastReceive > 0 && millis() - _lastReceive < _config.connectionTimeoutMs;
}

IPAddress DropCeilingSerialInput::localIP() const {
  return IPAddress(0, 0, 0, 0);
}

bool DropCeilingSerialInput::parseLine(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
  if (_lineLength == 0) {
    return false;
  }

  uint8_t parsedValues[DROPCEILING_CHANNELS];
  memcpy(parsedValues, values, DROPCEILING_CHANNELS);
  if (!_config.serialPartialUpdates) {
    memset(parsedValues, 0, DROPCEILING_CHANNELS);
  }

  char *cursor = _line;
  uint8_t channel = 0;
  while (cursor != nullptr && channel < DROPCEILING_CHANNELS) {
    char *next = strchr(cursor, _config.serialDelimiter);
    if (next != nullptr) {
      *next = '\0';
    }

    while (*cursor == ' ' || *cursor == '\t') {
      cursor++;
    }

    if (*cursor != '\0') {
      char *end = nullptr;
      long value = strtol(cursor, &end, 10);
      if (end != cursor) {
        parsedValues[channel] = static_cast<uint8_t>(constrain(value, 0, 255));
      }
    }

    channel++;
    cursor = next == nullptr ? nullptr : next + 1;
  }

  for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
    if (values[i] != parsedValues[i]) {
      values[i] = parsedValues[i];
      changed = true;
    }
  }

  return true;
}
