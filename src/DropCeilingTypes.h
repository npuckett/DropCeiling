#ifndef DROPCEILING_TYPES_H
#define DROPCEILING_TYPES_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingLevels.h"

static const uint8_t DROPCEILING_CHANNELS = DROPCEILING_LEVEL_CHANNELS;

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
  unsigned long dhcpTimeoutMs = 60000;  // how long Ethernet.begin() waits for a DHCP server
  unsigned long dhcpRetransmitMs = 2000;
  uint8_t dhcpRetryCount = 4;

  uint16_t artnetPort = 6454;
  uint16_t artnetUniverse = 0;
  uint16_t artnetStartChannel = 0;
  // 8: one slot per channel, slot start+c. 16: two slots per channel, start+2c high byte then
  // start+2c+1 low byte, so 16 slots for 8 channels.
  uint8_t artnetResolution = 8;
  unsigned long connectionTimeoutMs = 3000;

  // Loss of signal, for the network inputs. 0 keeps the old behaviour: hold the last frame for ever.
  // See DropCeilingFader in DropCeilingLevels.h.
  unsigned long lossTimeoutMs = 0;
  unsigned long lossFadeMs = 30000;
  uint16_t lossLevel = 0;          // 16-bit level the output fades to
  unsigned long resumeFadeMs = 0;  // fade from where the output is to the first frames

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
  // Optional text for the ArtPollReply node report, after the status code and counter. Health goes here.
  const char *nodeReport = nullptr;

  uint8_t standaloneValues[DROPCEILING_CHANNELS] = {0, 0, 0, 0, 0, 0, 0, 0};
};

#endif
