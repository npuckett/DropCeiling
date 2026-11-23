#ifndef DROPCEILING_ETHERNET_H
#define DROPCEILING_ETHERNET_H

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "DropCeilingArtNetInput.h"

class DropCeilingEthernetInput : public DropCeilingArtNetInput {
public:
  DropCeilingEthernetInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool connected() const override;
  IPAddress localIP() const override;
  void maintain() override;

  // False when the W5500 did not answer at begin().
  bool hardwareFound() const { return _hardwareFound; }
  // True when the cable is in and the far end is up. Unknown counts as up.
  bool linkUp() const;
  bool usingDhcp() const { return _usingDhcp; }

protected:
  UDP &udp() override { return _udp; }

private:
  EthernetUDP _udp;
  IPAddress _localIP;
  bool _hardwareFound;
  bool _usingDhcp;
};

#endif
