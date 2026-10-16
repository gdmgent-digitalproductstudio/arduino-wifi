#include <WiFiNINA.h>

void setup() {
  Serial.begin(9600);
  delay(1000);  // wacht op de USB-seriële verbinding (9600 baud)

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return;
  }

  Serial.println("Wifi-netwerken zoeken...");
  const int count = WiFi.scanNetworks();

  if (count <= 0) {
    Serial.println("Geen netwerken gevonden.");
    return;
  }

  for (int index = 0; index < count; index++) {
    Serial.print(index + 1);
    Serial.print(". ");
    Serial.print(WiFi.SSID(index));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(index));
    Serial.println(" dBm)");
  }
}

void loop() {}
