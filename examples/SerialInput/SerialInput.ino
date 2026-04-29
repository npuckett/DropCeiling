#include <DropCeiling.h>

DropCeiling ceiling(DropCeilingMode_Serial);

void setup() {
  ceiling.config().serialBaud = 115200;
  if (!ceiling.setup()) {
    Serial.println("DropCeiling Serial setup failed");
  }
  Serial.println("Send CSV values like: 0,128,255,0,0,0,0,0");
}

void loop() {
  ceiling.read();
}
