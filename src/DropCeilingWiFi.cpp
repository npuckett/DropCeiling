#include "DropCeilingWiFi.h"

DropCeilingWiFiInput::DropCeilingWiFiInput()
  : _lastReceive(0), _ready(false) {
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

  _ready = _udp.begin(_config.artnetPort) == 1;
  return _ready;
}

bool DropCeilingWiFiInput::read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
  changed = false;
  if (!_ready) {
    return false;
  }

  int packetSize = _udp.parsePacket();
  if (packetSize <= 0) {
    return false;
  }

  int length = _udp.read(_buffer, DropCeilingArtNet::MaxPacketSize);
  if (length <= 0) {
    return false;
  }

  DropCeilingArtNet::ParseResult result = DropCeilingArtNet::parsePacket(_buffer, length, _config, values);
  if (result.type == DropCeilingArtNet::PacketPoll) {
    sendPollReply();
  } else if (result.type == DropCeilingArtNet::PacketDmx) {
    _lastReceive = millis();
    changed = result.changed;
  }

  return result.type != DropCeilingArtNet::PacketIgnored;
}

bool DropCeilingWiFiInput::connected() const {
  return WiFi.status() == WL_CONNECTED && _lastReceive > 0 && millis() - _lastReceive < _config.connectionTimeoutMs;
}

IPAddress DropCeilingWiFiInput::localIP() const {
  return WiFi.localIP();
}

void DropCeilingWiFiInput::sendPollReply() {
  uint8_t reply[DropCeilingArtNet::PollReplySize];
  IPAddress ip = WiFi.localIP();
  size_t length = DropCeilingArtNet::buildPollReply(reply, sizeof(reply), _config, ip);
  if (length == 0) {
    return;
  }

  IPAddress broadcast(ip[0], ip[1], ip[2], 255);
  _udp.beginPacket(broadcast, _config.artnetPort);
  _udp.write(reply, length);
  _udp.endPacket();
}
