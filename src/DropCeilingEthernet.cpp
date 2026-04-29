#include "DropCeilingEthernet.h"

DropCeilingEthernetInput::DropCeilingEthernetInput()
  : _lastReceive(0), _ready(false) {
}

bool DropCeilingEthernetInput::begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) {
  (void)currentValues;
  _config = config;

  Ethernet.init(_config.ethernetCsPin);
  Ethernet.setRetransmissionTimeout(_config.dhcpRetransmitMs);
  Ethernet.setRetransmissionCount(_config.dhcpRetryCount);

  if (_config.useDhcp) {
    if (Ethernet.begin(_config.mac) == 0) {
      Ethernet.begin(_config.mac, _config.staticIp, _config.staticDns, _config.staticGateway, _config.staticSubnet);
    }
  } else {
    Ethernet.begin(_config.mac, _config.staticIp, _config.staticDns, _config.staticGateway, _config.staticSubnet);
  }

  _localIP = Ethernet.localIP();
  _ready = _udp.begin(_config.artnetPort) == 1;
  return _ready;
}

bool DropCeilingEthernetInput::read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) {
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

bool DropCeilingEthernetInput::connected() const {
  return _lastReceive > 0 && millis() - _lastReceive < _config.connectionTimeoutMs;
}

IPAddress DropCeilingEthernetInput::localIP() const {
  return _localIP;
}

void DropCeilingEthernetInput::sendPollReply() {
  uint8_t reply[DropCeilingArtNet::PollReplySize];
  size_t length = DropCeilingArtNet::buildPollReply(reply, sizeof(reply), _config, _localIP);
  if (length == 0) {
    return;
  }

  IPAddress broadcast(_localIP[0], _localIP[1], _localIP[2], 255);
  _udp.beginPacket(broadcast, _config.artnetPort);
  _udp.write(reply, length);
  _udp.endPacket();
}
