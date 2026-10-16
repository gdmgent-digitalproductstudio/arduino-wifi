#include <WiFi.h>

void setup() {
  Serial.begin(9600);
  delay(1000);  // wacht op de USB-seriële verbinding (9600 baud)

  // Scannen kan alleen in station-modus; disconnect wist een eventuele
  // eerdere verbinding zodat de scan niet gehinderd wordt.
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

  // Geef het geheugen van de scanresultaten vrij.
  WiFi.scanDelete();
}

void loop() {}
