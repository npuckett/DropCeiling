#include <DropCeiling.h>

DropCeiling ceiling(DropCeilingMode_Bluetooth);

void setup() {
  Serial.begin(115200);
  if (!ceiling.setup()) {
    Serial.println("DropCeiling BLE setup failed");
  }
}

void loop() {
  ceiling.read();
}
