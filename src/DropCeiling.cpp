#include "DropCeiling.h"

DropCeiling::DropCeiling(DropCeilingMode mode)
  : DropCeiling(mode, DropCeilingConfig()) {
}

DropCeiling::DropCeiling(DropCeilingMode mode, const DropCeilingConfig &config)
  : _mode(mode), _config(config), _ready(false), _input(nullptr), _display(nullptr), _usingBuiltInDisplay(false), _lastConnectionStatus(false), _lastDisplayUpdate(0) {
  memset(_values, 0, sizeof(_values));
}

bool DropCeiling::setup() {
  beginDisplay();

  bool dacReady = _dac.begin(_config);
  _input = selectInput();
  bool inputReady = _input != nullptr && _input->begin(_config, _values);
  bool startupChanged = false;
  if (inputReady) {
    _input->read(_values, startupChanged);
  }

  _ready = dacReady && inputReady;
  applyValues(true);
  refreshDisplay(true);
  return _ready;
}

bool DropCeiling::read() {
  if (_input == nullptr) {
    return false;
  }

  bool changed = false;
  bool received = _input->read(_values, changed);
  if (changed) {
    applyValues(true);
  }
  refreshDisplay(false);
  return received;
}

bool DropCeiling::voltageWrite(uint8_t channel, uint8_t value) {
  return write(channel, value);
}

bool DropCeiling::voltageWrite(uint8_t channel, int value) {
  return write(channel, static_cast<uint8_t>(constrain(value, 0, 255)));
}

bool DropCeiling::voltageWrite(uint8_t channel, float volts) {
  float clamped = constrain(volts, 0.0f, 10.0f);
  uint8_t value = static_cast<uint8_t>(roundf((clamped / 10.0f) * 255.0f));
  return write(channel, value);
}

bool DropCeiling::voltageWrite(uint8_t channel, double volts) {
  return voltageWrite(channel, static_cast<float>(volts));
}

bool DropCeiling::write(uint8_t channel, uint8_t value) {
  int8_t index = channelToIndex(channel);
  if (index < 0) {
    return false;
  }

  if (_values[index] == value) {
    return true;
  }

  _values[index] = value;
  bool ok = _dac.writeChannel(index, value);
  refreshDisplay(true);
  return ok;
}

bool DropCeiling::writeAll(const uint8_t values[DROPCEILING_CHANNELS]) {
  memcpy(_values, values, DROPCEILING_CHANNELS);
  bool ok = applyValues(true);
  refreshDisplay(true);
  return ok;
}

uint8_t DropCeiling::getValue(uint8_t channel) const {
  int8_t index = channelToIndex(channel);
  if (index < 0) {
    return 0;
  }
  return _values[index];
}

const uint8_t *DropCeiling::getValues() const {
  return _values;
}

bool DropCeiling::connected() const {
  return _input != nullptr && _input->connected();
}

IPAddress DropCeiling::localIP() const {
  return _input == nullptr ? IPAddress(0, 0, 0, 0) : _input->localIP();
}

bool DropCeiling::ready() const {
  return _ready;
}

void DropCeiling::setWiFiCredentials(const char *ssid, const char *password) {
  _config.wifiSsid = ssid;
  _config.wifiPassword = password;
}

void DropCeiling::setDisplay(DropCeilingDisplay *display) {
  _display = display;
  _usingBuiltInDisplay = false;
}

DropCeilingConfig &DropCeiling::config() {
  return _config;
}

DropCeilingInput *DropCeiling::selectInput() {
  switch (_mode) {
    case DropCeilingMode_Ethernet:
      return &_ethernetInput;
    case DropCeilingMode_WiFi:
      return &_wifiInput;
    case DropCeilingMode_Bluetooth:
      return &_bleInput;
    case DropCeilingMode_Serial:
      return &_serialInput;
    case DropCeilingMode_Standalone:
      return &_standaloneInput;
    default:
      return nullptr;
  }
}

bool DropCeiling::applyValues(bool changed) {
  if (!changed) {
    return true;
  }
  return _dac.writeAll(_values);
}

int8_t DropCeiling::channelToIndex(uint8_t channel) const {
  if (channel < 1 || channel > DROPCEILING_CHANNELS) {
    return -1;
  }
  return static_cast<int8_t>(channel - 1);
}

void DropCeiling::beginDisplay() {
  if (_config.displayMode == DropCeilingDisplay_Disabled) {
    _display = nullptr;
    return;
  }

  if (_display == nullptr) {
    bool useBuiltIn = _config.displayMode == DropCeilingDisplay_Enabled || DropCeilingDisplayST7789::isLikelyAvailable();
    if (useBuiltIn) {
      _display = &_builtInDisplay;
      _usingBuiltInDisplay = true;
    }
  }

  if (_display != nullptr && !_display->begin(_config) && _usingBuiltInDisplay) {
    _display = nullptr;
    _usingBuiltInDisplay = false;
  }
}

void DropCeiling::refreshDisplay(bool force) {
  if (_display == nullptr) {
    return;
  }

  bool connectionStatus = connected();
  if (force) {
    _display->drawStaticUI(localIP(), _config.artnetUniverse, _values);
    _display->setConnectionStatus(connectionStatus);
    _lastConnectionStatus = connectionStatus;
    _lastDisplayUpdate = millis();
    return;
  }

  if (connectionStatus != _lastConnectionStatus) {
    _display->setConnectionStatus(connectionStatus);
    _lastConnectionStatus = connectionStatus;
  }

  unsigned long now = millis();
  if (now - _lastDisplayUpdate >= _config.displayRefreshMs) {
    _display->updateChannels(_values);
    _lastDisplayUpdate = now;
  }
}
