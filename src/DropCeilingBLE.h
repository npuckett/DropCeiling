#ifndef DROPCEILING_BLE_H
#define DROPCEILING_BLE_H

#include <Arduino.h>
#include "DropCeilingInput.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#endif

class DropCeilingBLEInput : public DropCeilingInput
#if defined(ARDUINO_ARCH_ESP32)
  , public BLECharacteristicCallbacks, public BLEServerCallbacks
#endif
{
public:
  DropCeilingBLEInput();

  bool begin(const DropCeilingConfig &config, const uint8_t currentValues[DROPCEILING_CHANNELS]) override;
  bool read(uint8_t values[DROPCEILING_CHANNELS], bool &changed) override;
  bool connected() const override;
  IPAddress localIP() const override;

#if defined(ARDUINO_ARCH_ESP32)
  void onWrite(BLECharacteristic *characteristic) override;
  void onConnect(BLEServer *server) override;
  void onDisconnect(BLEServer *server) override;
#endif

private:
  DropCeilingConfig _config;
  uint8_t _pendingValues[DROPCEILING_CHANNELS];
  bool _hasPending;
  bool _clientConnected;

#if defined(ARDUINO_ARCH_ESP32)
  BLEServer *_server;
  BLECharacteristic *_characteristic;
#endif
};

#endif
