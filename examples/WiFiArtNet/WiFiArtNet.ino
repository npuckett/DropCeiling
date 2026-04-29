#include <DropCeiling.h>

DropCeilingConfig config;
DropCeiling ceiling(DropCeilingMode_WiFi, config);

void setup() {
  Serial.begin(115200);

  ceiling.setWiFiCredentials("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
  if (!ceiling.setup()) {
    Serial.println("DropCeiling WiFi setup failed");
  }
}

void loop() {
  ceiling.read();
}
