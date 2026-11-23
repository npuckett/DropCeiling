#ifndef DROPCEILING_H
#define DROPCEILING_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingTypes.h"
#include "DropCeilingLevels.h"
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
  // 16-bit level, 0-65535. The DAC takes the top bits.
  bool writeLevel(uint8_t channel, uint16_t level);

  // What the input or the API asked for, as bytes (the top 8 bits of the level).
  uint8_t getValue(uint8_t channel) const;
  const uint8_t *getValues() const;
  // What the input or the API asked for, 16-bit.
  uint16_t getLevel(uint8_t channel) const;
  // What is on the DAC now, 16-bit. Differs from getLevel() during a fade.
  uint16_t getOutputLevel(uint8_t channel) const;
  // The DAC code on a channel now.
  uint16_t getCode(uint8_t channel) const;

  bool connected() const;
  IPAddress localIP() const;
  bool ready() const;
  bool dacReady() const;
  bool inputReady() const;

  // Health, for the node report and the serial console.
  DropCeilingSignal signal() const;
  uint32_t frames() const;
  uint32_t losses() const;
  uint32_t dacErrors() const;

  // Text after the status code in ArtPollReply's node report, up to 48 characters.
  void setNodeReport(const char *text);

  void setWiFiCredentials(const char *ssid, const char *password);
  void setDisplay(DropCeilingDisplay *display);
  DropCeilingConfig &config();
  DropCeilingEthernetInput &ethernet() { return _ethernetInput; }

private:
  DropCeilingMode _mode;
  DropCeilingConfig _config;
  DropCeilingDAC _dac;
  DropCeilingFader _fader;
  uint8_t _values[DROPCEILING_CHANNELS];
  uint16_t _target[DROPCEILING_CHANNELS];
  bool _ready;
  bool _dacReady;
  bool _inputReady;
  uint32_t _lastFrames;
  char _report[49];

  DropCeilingInput *_input;
  DropCeilingEthernetInput _ethernetInput;
  DropCeilingWiFiInput _wifiInput;
  DropCeilingSerialInput _serialInput;
  DropCeilingBLEInput _bleInput;
  DropCeilingStandaloneInput _standaloneInput;

  DropCeilingDisplay *_display;
  DropCeilingDisplayST7789 _builtInDisplay;
  bool _usingBuiltInDisplay;
  DropCeilingDisplayState _lastState;
  unsigned long _lastDisplayUpdate;

  DropCeilingInput *selectInput();
  void updateOutput();
  void outputBytes(uint8_t bytes[DROPCEILING_CHANNELS]) const;
  int8_t channelToIndex(uint8_t channel) const;
  void beginDisplay();
  void refreshDisplay(bool force = false);
  DropCeilingDisplayState displayState() const;
};

#endif
