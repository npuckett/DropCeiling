#ifndef DROPCEILING_TYPES_H
#define DROPCEILING_TYPES_H

#include <Arduino.h>
#include <IPAddress.h>

static const uint8_t DROPCEILING_CHANNELS = 8;

enum DropCeilingMode {
  DropCeilingMode_Ethernet,
  DropCeilingMode_WiFi,
  DropCeilingMode_Bluetooth,
  DropCeilingMode_Serial,
  DropCeilingMode_Standalone
};

enum DropCeilingDisplayMode {
  DropCeilingDisplay_Auto,
  DropCeilingDisplay_Disabled,
  DropCeilingDisplay_Enabled
};

struct DropCeilingConfig {
  uint8_t dacAddress = 0x4C;
  uint8_t dacResolution = 10;
  uint16_t dacMaxValue = 1023;

  int8_t i2cSdaPin = 42;
  int8_t i2cSclPin = 41;
  uint8_t ethernetCsPin = 10;

  bool useDhcp = true;
  IPAddress staticIp = IPAddress(192, 168, 1, 200);
  IPAddress staticSubnet = IPAddress(255, 255, 255, 0);
  IPAddress staticGateway = IPAddress(192, 168, 1, 1);
  IPAddress staticDns = IPAddress(192, 168, 1, 1);
  uint8_t mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
  unsigned long dhcpRetransmitMs = 2000;
  uint8_t dhcpRetryCount = 4;

  uint16_t artnetPort = 6454;
  uint16_t artnetUniverse = 0;
  uint16_t artnetStartChannel = 0;
  unsigned long connectionTimeoutMs = 3000;

  const char *wifiSsid = nullptr;
  const char *wifiPassword = nullptr;
  unsigned long wifiConnectTimeoutMs = 15000;

  unsigned long serialBaud = 115200;
  char serialDelimiter = ',';
  bool serialPartialUpdates = true;

  const char *bleDeviceName = "DropCeiling";
  const char *bleServiceUuid = "7b4f0001-3f3a-4f6f-9b0d-84e8f64f0f10";
  const char *bleChannelsUuid = "7b4f0002-3f3a-4f6f-9b0d-84e8f64f0f10";
  bool bleNotifyOnChange = true;

  DropCeilingDisplayMode displayMode = DropCeilingDisplay_Auto;
  uint16_t displayRefreshMs = 100;

  const char *nodeShortName = "DropCeiling";
  const char *nodeLongName = "Drop Ceiling 0-10V Controller";
  uint8_t firmwareVersionHi = 1;
  uint8_t firmwareVersionLo = 0;

  uint8_t standaloneValues[DROPCEILING_CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0};
};

#endif
