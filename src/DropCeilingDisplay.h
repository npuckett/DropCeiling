#ifndef DROPCEILING_DISPLAY_H
#define DROPCEILING_DISPLAY_H

#include <Arduino.h>
#include <IPAddress.h>
#include "DropCeilingTypes.h"

// What the board is doing, worst first: a display shows the first that applies.
enum DropCeilingDisplayState {
  DropCeilingDisplayState_NoDac,    // the DAC did not answer at start
  DropCeilingDisplayState_NoLink,   // the Ethernet cable or switch is out
  DropCeilingDisplayState_Lost,     // frames stopped: fading out
  DropCeilingDisplayState_Waiting,  // no frames yet
  DropCeilingDisplayState_Live
};

class DropCeilingDisplay {
public:
  virtual ~DropCeilingDisplay() {}
  virtual bool begin(const DropCeilingConfig &config) = 0;
  virtual void drawStaticUI(const IPAddress &localIP, uint16_t universe, const uint8_t values[DROPCEILING_CHANNELS]) = 0;
  virtual void setConnectionStatus(bool connected) = 0;
  virtual void updateChannels(const uint8_t values[DROPCEILING_CHANNELS]) = 0;
  // Richer than setConnectionStatus. A display that does not draw it shows live or not.
  virtual void setState(DropCeilingDisplayState state) { setConnectionStatus(state == DropCeilingDisplayState_Live); }
};

#endif
