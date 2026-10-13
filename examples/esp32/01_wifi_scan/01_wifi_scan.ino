#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

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

  WiFi.scanDelete();
}

void loop() {}
