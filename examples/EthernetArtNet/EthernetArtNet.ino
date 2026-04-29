#include <DropCeiling.h>

DropCeiling ceiling(DropCeilingMode_Ethernet);

void setup() {
  Serial.begin(115200);
  if (!ceiling.setup()) {
    Serial.println("DropCeiling Ethernet setup failed");
  }
}

void loop() {
  ceiling.read();
}
