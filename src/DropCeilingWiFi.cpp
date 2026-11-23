#include "DropCeilingWiFi.h"

DropCeilingWiFiInput::DropCeilingWiFiInput() {
}

bool DropCeilingWiFiInput::begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) {
  (void)currentValues;
  _config = config;

  if (_config.wifiSsid == nullptr || strlen(_config.wifiSsid) == 0) {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(_config.wifiSsid, _config.wifiPassword == nullptr ? "" : _config.wifiPassword);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < _config.wifiConnectTimeoutMs) {
    delay(10);
  }

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFi.macAddress(_config.mac);  // report the radio's own MAC in ArtPollReply
  _ready = _udp.begin(_config.artnetPort) == 1;
  return _ready;
}

bool DropCeilingWiFiInput::connected() const {
  return WiFi.status() == WL_CONNECTED && receivedRecently();
}

IPAddress DropCeilingWiFiInput::localIP() const {
  return WiFi.localIP();
}
