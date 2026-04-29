#ifndef DROPCEILING_DISPLAY_ST7789_H
#define DROPCEILING_DISPLAY_ST7789_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "DropCeilingDisplay.h"

#if defined(TFT_CS) && defined(TFT_DC) && defined(TFT_RST)
#define DROPCEILING_HAS_TFT_PIN_DEFAULTS 1
#define DROPCEILING_TFT_CS TFT_CS
#define DROPCEILING_TFT_DC TFT_DC
#define DROPCEILING_TFT_RST TFT_RST
#else
#define DROPCEILING_HAS_TFT_PIN_DEFAULTS 0
#define DROPCEILING_TFT_CS -1
#define DROPCEILING_TFT_DC -1
#define DROPCEILING_TFT_RST -1
#endif

class DropCeilingDisplayST7789 : public DropCeilingDisplay {
public:
  DropCeilingDisplayST7789(int8_t cs = DROPCEILING_TFT_CS, int8_t dc = DROPCEILING_TFT_DC, int8_t rst = DROPCEILING_TFT_RST);

  bool begin(const DropCeilingConfig &config) override;
  void drawStaticUI(const IPAddress &localIP, uint16_t universe, const uint8_t values[DROPCEILING_CHANNELS]) override;
  void setConnectionStatus(bool connected) override;
  void updateChannels(const uint8_t values[DROPCEILING_CHANNELS]) override;

  static bool isLikelyAvailable();

private:
  Adafruit_ST7789 _tft;
  int8_t _cs;
  int8_t _dc;
  int8_t _rst;
  bool _ready;
  uint8_t _displayValues[DROPCEILING_CHANNELS];

  void drawChannel(uint8_t channel, uint8_t value);
};

#endif
