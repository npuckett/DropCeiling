#include "DropCeilingDisplayST7789.h"

namespace {
// Adafruit GFX's built-in font is 6 x 8 pixels at size 1.
const int16_t W = 240;
const int16_t CHAR_W = 6;
const int16_t CHAR_H = 8;

const int16_t NAME_Y = 4;          // the name, up to 32 px tall
const int16_t STATE_Y = 42;        // state and address, size 2
const int16_t INFO_Y = 62;         // universe, firmware, resolution, size 1
const int16_t BARS_Y = 76;         // the top of the bars
const int16_t BARS_H = 44;
const int16_t SLOT_W = W / DROPCEILING_CHANNELS;
const int16_t BAR_W = 20;
const int16_t LABEL_Y = BARS_Y + BARS_H + 4;

const uint16_t COL_BG = ST77XX_BLACK;
const uint16_t COL_TEXT = ST77XX_WHITE;
const uint16_t COL_LABEL = 0xBDF7;
const uint16_t COL_BAR = 0xFFFF;
const uint16_t COL_BAR_BG = 0x2104;
const uint16_t COL_GREEN = 0x07E0;
const uint16_t COL_AMBER = 0xFD20;
const uint16_t COL_RED = 0xF800;

uint8_t largestSize(const char *text, int16_t width, uint8_t most) {
  int16_t chars = static_cast<int16_t>(strlen(text));
  for (uint8_t size = most; size > 1; size--) {
    if (chars * CHAR_W * size <= width) {
      return size;
    }
  }
  return 1;
}
}

DropCeilingDisplayST7789::DropCeilingDisplayST7789(int8_t cs, int8_t dc, int8_t rst)
  : _tft(cs, dc, rst), _cs(cs), _dc(dc), _rst(rst), _ready(false), _fwHi(0), _fwLo(0), _resolution(8) {
  memset(_displayValues, 0xFF, sizeof(_displayValues));
  _name[0] = '\0';
}

bool DropCeilingDisplayST7789::begin(const DropCeilingConfig &config) {
  if (_cs < 0 || _dc < 0) {
    return false;
  }
  strncpy(_name, config.nodeShortName != nullptr ? config.nodeShortName : "", sizeof(_name) - 1);
  _name[sizeof(_name) - 1] = '\0';
  _fwHi = config.firmwareVersionHi;
  _fwLo = config.firmwareVersionLo;
  _resolution = config.artnetResolution;

  // The same pin powers the I2C connector, so the DAC, if it is fed from there: on before the DAC starts.
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
  // The name from the first moment, while the network starts (DHCP can take seconds).
  drawName("starting");

  _ready = true;
  return true;
}

void DropCeilingDisplayST7789::drawName(const char *below) {
  uint8_t size = largestSize(_name, W - 8, 4);
  int16_t x = (W - static_cast<int16_t>(strlen(_name)) * CHAR_W * size) / 2;
  int16_t y = NAME_Y + (32 - CHAR_H * size) / 2;
  _tft.fillRect(0, 0, W, STATE_Y - 2, COL_BG);
  _tft.setTextWrap(false);
  _tft.setTextSize(size);
  _tft.setTextColor(COL_TEXT, COL_BG);
  _tft.setCursor(x < 0 ? 0 : x, y);
  _tft.print(_name);
  if (below != nullptr) {
    _tft.setTextSize(2);
    _tft.setTextColor(COL_LABEL, COL_BG);
    _tft.setCursor(4, STATE_Y);
    _tft.print(below);
  }
}

void DropCeilingDisplayST7789::drawStaticUI(const IPAddress &localIP, uint16_t universe, const uint8_t values[DROPCEILING_CHANNELS]) {
  if (!_ready) {
    return;
  }

  _tft.fillScreen(COL_BG);
  drawName(nullptr);

  // The address, right-aligned on the state's line: large when it fits beside the state (10.42.0.x does).
  char ip[16];
  snprintf(ip, sizeof(ip), "%u.%u.%u.%u", localIP[0], localIP[1], localIP[2], localIP[3]);
  uint8_t ipSize = strlen(ip) <= 11 ? 2 : 1;
  _tft.setTextSize(ipSize);
  _tft.setTextColor(COL_TEXT, COL_BG);
  _tft.setCursor(W - 4 - static_cast<int16_t>(strlen(ip)) * CHAR_W * ipSize, ipSize == 2 ? STATE_Y : STATE_Y + 4);
  _tft.print(ip);

  char info[40];
  snprintf(info, sizeof(info), "universe %u   fw %u.%u   %u-bit", universe, _fwHi, _fwLo, _resolution);
  _tft.setTextSize(1);
  _tft.setTextColor(COL_LABEL, COL_BG);
  _tft.setCursor(4, INFO_Y);
  _tft.print(info);

  for (uint8_t channel = 0; channel < DROPCEILING_CHANNELS; channel++) {
    int16_t x = channel * SLOT_W + (SLOT_W - BAR_W) / 2;
    _tft.fillRect(x, BARS_Y, BAR_W, BARS_H, COL_BAR_BG);
    _tft.setTextSize(1);
    _tft.setTextColor(COL_LABEL, COL_BG);
    _tft.setCursor(channel * SLOT_W + (SLOT_W - CHAR_W) / 2, LABEL_Y);
    _tft.print(channel + 1);
    _displayValues[channel] = 0xFF;
  }

  setState(DropCeilingDisplayState_Waiting);
  updateChannels(values);
}

void DropCeilingDisplayST7789::setConnectionStatus(bool connected) {
  setState(connected ? DropCeilingDisplayState_Live : DropCeilingDisplayState_Waiting);
}

void DropCeilingDisplayST7789::setState(DropCeilingDisplayState state) {
  if (!_ready) {
    return;
  }
  const char *text = "WAIT";
  uint16_t colour = COL_AMBER;
  switch (state) {
    case DropCeilingDisplayState_Live: text = "LIVE"; colour = COL_GREEN; break;
    case DropCeilingDisplayState_Lost: text = "LOST"; colour = COL_RED; break;
    case DropCeilingDisplayState_NoLink: text = "NO LINK"; colour = COL_RED; break;
    case DropCeilingDisplayState_NoDac: text = "NO DAC"; colour = COL_RED; break;
    default: break;
  }
  // Up to 98 px: a dot and seven characters at size 2. The address on this line starts at 104 or later.
  _tft.fillRect(0, STATE_Y - 1, 98, 17, COL_BG);
  _tft.fillCircle(8, STATE_Y + 7, 5, colour);
  _tft.setTextSize(2);
  _tft.setTextColor(colour, COL_BG);
  _tft.setCursor(18, STATE_Y);
  _tft.print(text);
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
  int16_t x = channel * SLOT_W + (SLOT_W - BAR_W) / 2;
  int16_t h = static_cast<int16_t>((static_cast<int32_t>(value) * BARS_H + 127) / 255);
  // Only the two parts, so the bar never flickers through empty.
  if (h < BARS_H) {
    _tft.fillRect(x, BARS_Y, BAR_W, BARS_H - h, COL_BAR_BG);
  }
  if (h > 0) {
    _tft.fillRect(x, BARS_Y + BARS_H - h, BAR_W, h, COL_BAR);
  }
  _displayValues[channel] = value;
}
