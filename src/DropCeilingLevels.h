#ifndef DROPCEILING_LEVELS_H
#define DROPCEILING_LEVELS_H

// Output levels and the fades around loss of signal. No Arduino dependencies, so the logic can be
// tested on a desktop compiler (extras/tests).
//
// Every input works in a 16-bit level, 0-65535. An 8-bit value v becomes v * 257, so 255 is 65535 and
// a byte still reaches full scale. The DAC takes the top bits: a 10-bit DAC code is level >> 6.

#include <stdint.h>

static const uint8_t DROPCEILING_LEVEL_CHANNELS = 8;

namespace DropCeilingLevels {
inline uint16_t fromByte(uint8_t value) {
  return static_cast<uint16_t>(value) * 257u;
}

inline uint8_t toByte(uint16_t level) {
  return static_cast<uint8_t>(level >> 8);
}

inline uint16_t toCode(uint16_t level, uint8_t dacBits) {
  if (dacBits >= 16) {
    return level;
  }
  return static_cast<uint16_t>(level >> (16 - dacBits));
}

// Straight line from a to b, t of span milliseconds in. t >= span, or span 0, gives b.
inline uint16_t lerp(uint16_t a, uint16_t b, uint32_t t, uint32_t span) {
  if (span == 0 || t >= span) {
    return b;
  }
  int32_t delta = static_cast<int32_t>(b) - static_cast<int32_t>(a);
  return static_cast<uint16_t>(static_cast<int32_t>(a) + static_cast<int32_t>((static_cast<int64_t>(delta) * t) / span));
}
}

enum DropCeilingSignal {
  DropCeilingSignal_Waiting,  // no frame since boot: output holds where the DAC was
  DropCeilingSignal_Live,     // frames arriving: output follows them
  DropCeilingSignal_Lost      // silence past the timeout: output fades to the loss level
};

// Follows the frames, and never jumps when they start or stop.
//  - Boot: hold the level the DAC already had (0 after power-up, the old level after a watchdog reset).
//  - First frame, or frames again after a loss: a straight fade from where the output is to the
//    frames' values over resumeFadeMs, then follow them exactly.
//  - No frame for lossTimeoutMs: a straight fade from where the output is to lossLevel over lossFadeMs.
// lossTimeoutMs 0 turns all of this off: the output is the target, and silence holds the last frame.
class DropCeilingFader {
public:
  uint32_t lossTimeoutMs = 0;
  uint32_t lossFadeMs = 0;
  uint16_t lossLevel = 0;
  uint32_t resumeFadeMs = 0;

  void begin(const uint16_t start[DROPCEILING_LEVEL_CHANNELS], uint32_t now) {
    for (uint8_t i = 0; i < DROPCEILING_LEVEL_CHANNELS; i++) {
      _out[i] = start[i];
      _from[i] = start[i];
    }
    _signal = DropCeilingSignal_Waiting;
    _since = now;
    _lastFrame = now;
    _resuming = false;
  }

  void frame(uint32_t now) {
    _lastFrame = now;
    if (_signal != DropCeilingSignal_Live) {
      _signal = DropCeilingSignal_Live;
      if (resumeFadeMs > 0) {
        snapshot();
        _since = now;
        _resuming = true;
      }
    }
  }

  const uint16_t *update(const uint16_t target[DROPCEILING_LEVEL_CHANNELS], uint32_t now) {
    if (lossTimeoutMs == 0) {
      for (uint8_t i = 0; i < DROPCEILING_LEVEL_CHANNELS; i++) {
        _out[i] = target[i];
      }
      return _out;
    }

    if (_signal != DropCeilingSignal_Lost && now - _lastFrame >= lossTimeoutMs) {
      _signal = DropCeilingSignal_Lost;
      snapshot();
      _since = now;
      _resuming = false;
      _fading = true;
      _losses++;
    }

    switch (_signal) {
      case DropCeilingSignal_Waiting:
        break;
      case DropCeilingSignal_Live:
        if (_resuming && now - _since >= resumeFadeMs) {
          _resuming = false;
        }
        for (uint8_t i = 0; i < DROPCEILING_LEVEL_CHANNELS; i++) {
          _out[i] = _resuming ? DropCeilingLevels::lerp(_from[i], target[i], now - _since, resumeFadeMs) : target[i];
        }
        break;
      case DropCeilingSignal_Lost:
        if (_fading && now - _since >= lossFadeMs) {
          _fading = false;  // settled: millis() wrapping later must not restart the fade
        }
        for (uint8_t i = 0; i < DROPCEILING_LEVEL_CHANNELS; i++) {
          _out[i] = _fading ? DropCeilingLevels::lerp(_from[i], lossLevel, now - _since, lossFadeMs) : lossLevel;
        }
        break;
    }
    return _out;
  }

  DropCeilingSignal signal() const { return _signal; }
  const uint16_t *output() const { return _out; }
  uint32_t losses() const { return _losses; }

private:
  uint16_t _out[DROPCEILING_LEVEL_CHANNELS] = {0};
  uint16_t _from[DROPCEILING_LEVEL_CHANNELS] = {0};
  DropCeilingSignal _signal = DropCeilingSignal_Waiting;
  uint32_t _since = 0;
  uint32_t _lastFrame = 0;
  uint32_t _losses = 0;
  bool _resuming = false;
  bool _fading = false;

  void snapshot() {
    for (uint8_t i = 0; i < DROPCEILING_LEVEL_CHANNELS; i++) {
      _from[i] = _out[i];
    }
  }
};

#endif
