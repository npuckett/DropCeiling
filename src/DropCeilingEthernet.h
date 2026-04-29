#ifndef DROPCEILING_ETHERNET_H
#define DROPCEILING_ETHERNET_H

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "DropCeilingArtNet.h"
#include "DropCeilingInput.h"

class DropCeilingEthernetInput : public DropCeilingInput {
public:
  DropCeilingEthernetInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) override;
  bool connected() const override;
  IPAddress localIP() const override;

private:
  DropCeilingConfig _config;
  EthernetUDP _udp;
  IPAddress _localIP;
  uint8_t _buffer[DropCeilingArtNet::MaxPacketSize];
  unsigned long _lastReceive;
  bool _ready;

  void sendPollReply();
};

#endif
