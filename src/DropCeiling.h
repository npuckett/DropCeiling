#ifndef DROPCEILING_H
#define DROPCEILING_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingTypes.h"
#include "DropCeilingDAC.h"
#include "DropCeilingDisplay.h"
#include "DropCeilingDisplayST7789.h"
#include "DropCeilingEthernet.h"
#include "DropCeilingWiFi.h"
#include "DropCeilingSerial.h"
#include "DropCeilingBLE.h"
#include "DropCeilingStandalone.h"

class DropCeiling {
public:
  explicit DropCeiling(DropCeilingMode mode = DropCeilingMode_Ethernet);
  DropCeiling(DropCeilingMode mode, const DropCeilingConfig &config);

  bool setup();
  bool read();

  bool voltageWrite(uint8_t channel, uint8_t value);
  bool voltageWrite(uint8_t channel, int value);
  bool voltageWrite(uint8_t channel, float volts);
  bool voltageWrite(uint8_t channel, double volts);
  bool write(uint8_t channel, uint8_t value);
  bool writeAll(const uint8_t values[DROPCEILING_CHANNELS]);

  uint8_t getValue(uint8_t channel) const;
  const uint8_t *getValues() const;
  bool connected() const;
  IPAddress localIP() const;
  bool ready() const;

  void setWiFiCredentials(const char *ssid, const char *password);
  void setDisplay(DropCeilingDisplay *display);
  DropCeilingConfig &config();

private:
  DropCeilingMode _mode;
  DropCeilingConfig _config;
  DropCeilingDAC _dac;
  uint8_t _values[DROPCEILING_CHANNELS];
  bool _ready;

  DropCeilingInput *_input;
  DropCeilingEthernetInput _ethernetInput;
  DropCeilingWiFiInput _wifiInput;
  DropCeilingSerialInput _serialInput;
  DropCeilingBLEInput _bleInput;
  DropCeilingStandaloneInput _standaloneInput;

  DropCeilingDisplay *_display;
  DropCeilingDisplayST7789 _builtInDisplay;
  bool _usingBuiltInDisplay;
  bool _lastConnectionStatus;
  unsigned long _lastDisplayUpdate;

  DropCeilingInput *selectInput();
  bool applyValues(bool changed);
  int8_t channelToIndex(uint8_t channel) const;
  void beginDisplay();
  void refreshDisplay(bool force = false);
};

#endif
