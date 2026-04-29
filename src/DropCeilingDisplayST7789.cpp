#include "DropCeilingDisplayST7789.h"

namespace {
const uint8_t HEADER_H = 16;
const uint8_t BAR_X = 16;
const uint8_t BAR_Y_START = 22;
const uint8_t BAR_MAX_W = 184;
const uint8_t BAR_H = 11;
const uint8_t BAR_SPACING = 14;
const uint8_t VAL_X = BAR_X + BAR_MAX_W + 4;

const uint16_t COL_BG = ST77XX_BLACK;
const uint16_t COL_BAR = 0x07E0;
const uint16_t COL_BAR_BG = 0x2104;
const uint16_t COL_TEXT = ST77XX_WHITE;
const uint16_t COL_LABEL = 0xBDF7;
}

DropCeilingDisplayST7789::DropCeilingDisplayST7789(int8_t cs, int8_t dc, int8_t rst)
  : _tft(cs, dc, rst), _cs(cs), _dc(dc), _rst(rst), _ready(false) {
  memset(_displayValues, 0xFF, sizeof(_displayValues));
}

bool DropCeilingDisplayST7789::begin(const DropCeilingConfig &config) {
  (void)config;
  if (_cs < 0 || _dc < 0) {
    return false;
  }

#if defined(TFT_I2C_POWER)
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
#endif
#if defined(TFT_BACKLITE)
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
#endif

  delay(10);
  _tft.init(135, 240);
  _tft.setRotation(3);
  _tft.fillScreen(COL_BG);

  _tft.setTextSize(2);
  _tft.setTextColor(COL_BAR);
  _tft.setCursor(20, 20);
  _tft.println("Drop Ceiling");
  _tft.setCursor(20, 42);
  _tft.println("Controller");
  _tft.setTextSize(1);
  _tft.setTextColor(COL_LABEL);
  _tft.setCursor(20, 70);
  _tft.print("v");
  _tft.print(config.firmwareVersionHi);
  _tft.print(".");
  _tft.println(config.firmwareVersionLo);
  delay(500);

  _ready = true;
  return true;
}

void DropCeilingDisplayST7789::drawStaticUI(const IPAddress &localIP, uint16_t universe, const uint8_t values[DROPCEILING_CHANNELS]) {
  if (!_ready) {
    return;
  }

  _tft.fillScreen(COL_BG);
  _tft.setTextSize(1);
  _tft.setTextColor(COL_TEXT, COL_BG);
  _tft.setCursor(0, 2);
  _tft.print("IP: ");
  _tft.print(localIP);

  _tft.setCursor(190, 2);
  _tft.print("U:");
  _tft.print(universe);

  _tft.drawFastHLine(0, HEADER_H, 240, COL_LABEL);

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    int y = BAR_Y_START + channel * BAR_SPACING;
    _tft.setTextColor(COL_LABEL, COL_BG);
    _tft.setCursor(0, y + 2);
    _tft.print(channel + 1);
    _tft.fillRect(BAR_X, y, BAR_MAX_W, BAR_H, COL_BAR_BG);
    _displayValues[channel] = 0xFF;
  }

  setConnectionStatus(false);
  updateChannels(values);
}

void DropCeilingDisplayST7789::setConnectionStatus(bool connected) {
  if (!_ready) {
    return;
  }

  int statusX = 120;
  int statusY = 2;
  _tft.fillRect(statusX, 0, 65, HEADER_H - 1, COL_BG);

  if (connected) {
    _tft.fillCircle(statusX + 4, statusY + 4, 3, 0x07E0);
    _tft.setTextColor(0x07E0, COL_BG);
    _tft.setCursor(statusX + 12, statusY);
    _tft.print("LIVE");
  } else {
    _tft.fillCircle(statusX + 4, statusY + 4, 3, 0xF800);
    _tft.setTextColor(0xF800, COL_BG);
    _tft.setCursor(statusX + 12, statusY);
    _tft.print("NO SIG");
  }
}

void DropCeilingDisplayST7789::updateChannels(const uint8_t values[DROPCEILING_CHANNELS]) {
  if (!_ready) {
    return;
  }

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    if (_displayValues[channel] != values[channel]) {
      drawChannel(channel, values[channel]);
    }
  }
}

bool DropCeilingDisplayST7789::isLikelyAvailable() {
  return DROPCEILING_HAS_TFT_PIN_DEFAULTS == 1;
}

void DropCeilingDisplayST7789::drawChannel(uint8_t channel, uint8_t value) {
  int y = BAR_Y_START + channel * BAR_SPACING;

  _tft.fillRect(BAR_X, y, BAR_MAX_W, BAR_H, COL_BAR_BG);
  int barW = map(value, 0, 255, 0, BAR_MAX_W);
  if (barW > 0) {
    _tft.fillRect(BAR_X, y, barW, BAR_H, COL_BAR);
  }

  _tft.setTextColor(COL_TEXT, COL_BG);
  _tft.setCursor(VAL_X, y + 2);
  if (value < 10) {
    _tft.print("  ");
  } else if (value < 100) {
    _tft.print(" ");
  }
  _tft.print(value);

  _displayValues[channel] = value;
}
