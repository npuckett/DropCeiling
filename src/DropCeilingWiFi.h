#ifndef DROPCEILING_WIFI_H
#define DROPCEILING_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "DropCeilingArtNetInput.h"

class DropCeilingWiFiInput : public DropCeilingArtNetInput {
public:
  DropCeilingWiFiInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool connected() const override;
  IPAddress localIP() const override;

protected:
  UDP &udp() override { return _udp; }

private:
  WiFiUDP _udp;
};

#endif
