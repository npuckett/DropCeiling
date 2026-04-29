#ifndef DROPCEILING_WIFI_H
#define DROPCEILING_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "DropCeilingArtNet.h"
#include "DropCeilingInput.h"

class DropCeilingWiFiInput : public DropCeilingInput {
public:
  DropCeilingWiFiInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) override;
  bool connected() const override;
  IPAddress localIP() const override;

private:
  DropCeilingConfig _config;
  WiFiUDP _udp;
  uint8_t _buffer[DropCeilingArtNet::MaxPacketSize];
  unsigned long _lastReceive;
  bool _ready;

  void sendPollReply();
};

#endif
