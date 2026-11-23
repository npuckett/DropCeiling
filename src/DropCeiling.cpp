#include "DropCeiling.h"

DropCeiling::DropCeiling(DropCeilingMode mode)
  : DropCeiling(mode, DropCeilingConfig()) {
}

DropCeiling::DropCeiling(DropCeilingMode mode, const DropCeilingConfig &config)
  : _mode(mode), _config(config), _ready(false), _dacReady(false), _inputReady(false), _lastFrames(0), _input(nullptr),
    _display(nullptr), _usingBuiltInDisplay(false), _lastConnectionStatus(false), _lastDisplayUpdate(0) {
  memset(_values, 0, sizeof(_values));
  memset(_target, 0, sizeof(_target));
  memset(_report, 0, sizeof(_report));
}

bool DropCeiling::setup() {
  if (_config.nodeReport == nullptr) {
    _config.nodeReport = _report;
  }
  beginDisplay();

  uint16_t start[DROPCEILING_CHANNELS];
  _dacReady = _dac.begin(_config, start);
  // Start from what the DAC holds, so a restart neither jumps nor drops.
  memcpy(_target, start, sizeof(_target));
  for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
    _values[i] = DropCeilingLevels::toByte(_target[i]);
  }

  _input = selectInput();
  _inputReady = _input != nullptr && _input->begin(_config, _values);

  bool fades = _inputReady && _input->countsFrames();
  _fader.lossTimeoutMs = fades ? _config.lossTimeoutMs : 0;
  _fader.lossFadeMs = _config.lossFadeMs;
  _fader.lossLevel = _config.lossLevel;
  _fader.resumeFadeMs = _config.resumeFadeMs;
  _fader.begin(start, millis());
  _lastFrames = 0;

  if (_inputReady) {
    bool startupChanged = false;
    if (_input->providesLevels()) {
      _input->readLevels(_target, startupChanged);
    } else if (_input->read(_values, startupChanged) && startupChanged) {
      for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
        _target[i] = DropCeilingLevels::fromByte(_values[i]);
      }
    }
  }

  _ready = _dacReady && _inputReady;
  updateOutput();
  refreshDisplay(true);
  return _ready;
}

bool DropCeiling::read() {
  if (_input == nullptr) {
    return false;
  }

  _input->maintain();
  bool changed = false;
  bool received;
  if (_input->providesLevels()) {
    received = _input->readLevels(_target, changed);
    if (changed) {
      for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
        _values[i] = DropCeilingLevels::toByte(_target[i]);
      }
    }
  } else {
    received = _input->read(_values, changed);
    if (changed) {
      for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
        _target[i] = DropCeilingLevels::fromByte(_values[i]);
      }
    }
  }

  uint32_t frames = _input->frames();
  if (frames != _lastFrames) {
    _lastFrames = frames;
    _fader.frame(millis());
  }

  updateOutput();
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
  return writeLevel(channel, static_cast<uint16_t>(roundf((clamped / 10.0f) * 65535.0f)));
}

bool DropCeiling::voltageWrite(uint8_t channel, double volts) {
  return voltageWrite(channel, static_cast<float>(volts));
}

bool DropCeiling::write(uint8_t channel, uint8_t value) {
  return writeLevel(channel, DropCeilingLevels::fromByte(value));
}

bool DropCeiling::writeLevel(uint8_t channel, uint16_t level) {
  int8_t index = channelToIndex(channel);
  if (index < 0) {
    return false;
  }
  _target[index] = level;
  _values[index] = DropCeilingLevels::toByte(level);
  updateOutput();
  refreshDisplay(false);
  return _dacReady;
}

bool DropCeiling::writeAll(const uint8_t values[DROPCEILING_CHANNELS]) {
  for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
    _values[i] = values[i];
    _target[i] = DropCeilingLevels::fromByte(values[i]);
  }
  updateOutput();
  refreshDisplay(true);
  return _dacReady;
}

uint8_t DropCeiling::getValue(uint8_t channel) const {
  int8_t index = channelToIndex(channel);
  return index < 0 ? 0 : _values[index];
}

const uint8_t *DropCeiling::getValues() const {
  return _values;
}

uint16_t DropCeiling::getLevel(uint8_t channel) const {
  int8_t index = channelToIndex(channel);
  return index < 0 ? 0 : _target[index];
}

uint16_t DropCeiling::getOutputLevel(uint8_t channel) const {
  int8_t index = channelToIndex(channel);
  return index < 0 ? 0 : _fader.output()[index];
}

uint16_t DropCeiling::getCode(uint8_t channel) const {
  int8_t index = channelToIndex(channel);
  return index < 0 ? 0 : _dac.code(index);
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

bool DropCeiling::dacReady() const {
  return _dacReady;
}

bool DropCeiling::inputReady() const {
  return _inputReady;
}

DropCeilingSignal DropCeiling::signal() const {
  return _fader.signal();
}

uint32_t DropCeiling::frames() const {
  return _input == nullptr ? 0 : _input->frames();
}

uint32_t DropCeiling::losses() const {
  return _fader.losses();
}

uint32_t DropCeiling::dacErrors() const {
  return _dac.errors();
}

void DropCeiling::setNodeReport(const char *text) {
  strncpy(_report, text == nullptr ? "" : text, sizeof(_report) - 1);
  _report[sizeof(_report) - 1] = '\0';
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

void DropCeiling::updateOutput() {
  const uint16_t *out = _fader.update(_target, millis());
  if (_dacReady) {
    _dac.writeLevels(out);
  }
  if (_input != nullptr) {
    _input->setLive(_fader.signal() == DropCeilingSignal_Live);
  }
}

void DropCeiling::outputBytes(uint8_t bytes[DROPCEILING_CHANNELS]) const {
  const uint16_t *out = _fader.output();
  for (uint8_t i = 0; i < DROPCEILING_CHANNELS; i++) {
    bytes[i] = DropCeilingLevels::toByte(out[i]);
  }
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

  uint8_t shown[DROPCEILING_CHANNELS];
  outputBytes(shown);
  bool connectionStatus = connected();
  if (force) {
    _display->drawStaticUI(localIP(), _config.artnetUniverse, shown);
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
    _display->updateChannels(shown);
    _lastDisplayUpdate = now;
  }
}
