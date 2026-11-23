#include "DropCeilingEthernet.h"

DropCeilingEthernetInput::DropCeilingEthernetInput()
  : _hardwareFound(false), _usingDhcp(false) {
}

bool DropCeilingEthernetInput::begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) {
  (void)currentValues;
  _config = config;

  Ethernet.init(_config.ethernetCsPin);
  Ethernet.setRetransmissionTimeout(_config.dhcpRetransmitMs);
  Ethernet.setRetransmissionCount(_config.dhcpRetryCount);

  _usingDhcp = false;
  if (_config.useDhcp) {
    _usingDhcp = Ethernet.begin(_config.mac, _config.dhcpTimeoutMs) != 0;
    if (!_usingDhcp) {
      Ethernet.begin(_config.mac, _config.staticIp, _config.staticDns, _config.staticGateway, _config.staticSubnet);
    }
  } else {
    Ethernet.begin(_config.mac, _config.staticIp, _config.staticDns, _config.staticGateway, _config.staticSubnet);
  }

  // Without the W5500 there is nothing to listen on. Say so rather than pretend.
  _hardwareFound = Ethernet.hardwareStatus() != EthernetNoHardware;
  _localIP = Ethernet.localIP();
  _config.useDhcp = _usingDhcp;
  _ready = _hardwareFound && _udp.begin(_config.artnetPort) == 1;
  return _ready;
}

bool DropCeilingEthernetInput::connected() const {
  return receivedRecently();
}

IPAddress DropCeilingEthernetInput::localIP() const {
  return _localIP;
}

void DropCeilingEthernetInput::maintain() {
  if (!_usingDhcp) {
    return;
  }
  // Renews the lease. Without it a DHCP board keeps an address the server may give to someone else.
  int result = Ethernet.maintain();
  if (result == 2 || result == 4) {  // renew or rebind succeeded
    _localIP = Ethernet.localIP();
  }
}

bool DropCeilingEthernetInput::linkUp() const {
  return Ethernet.linkStatus() != LinkOFF;
}
