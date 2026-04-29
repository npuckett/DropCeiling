#include <DropCeiling.h>

DropCeiling ceiling(DropCeilingMode_Standalone);

unsigned long lastStep = 0;
uint8_t level = 0;

void setup() {
  Serial.begin(115200);

  ceiling.config().standaloneValues[0] = 64;
  ceiling.config().standaloneValues[1] = 128;
  ceiling.config().standaloneValues[2] = 192;
  ceiling.config().standaloneValues[3] = 255;

  if (!ceiling.setup()) {
    Serial.println("DropCeiling Standalone setup failed");
  }
}

void loop() {
  ceiling.read();

  if (millis() - lastStep >= 1000) {
    lastStep = millis();
    level = level == 0 ? 255 : 0;
    ceiling.voltageWrite(1, level);
    ceiling.voltageWrite(2, level / 2);
    ceiling.voltageWrite(3, 5.0f);
  }
}
