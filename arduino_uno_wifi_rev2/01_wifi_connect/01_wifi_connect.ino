#include <WiFiNINA.h>

#include "secrets.h"

const unsigned long WIFI_TIMEOUT_MS = 20000;

bool connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Geen eindeloze while-lus: zonder time-out blijft het bord hangen bij
  // een fout wachtwoord of een netwerk buiten bereik.
  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi-verbinding mislukt.");
    return false;
  }

  Serial.println("Wifi verbonden.");
  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signaalsterkte: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  return true;
}

void setup() {
  Serial.begin(115200);
  // Geef de USB-seriële verbinding even de tijd om te starten, anders
  // mis je de eerste regels in de Serial Monitor. Zet de Serial Monitor
  // zelf ook op 115200 baud.
  delay(1000);

  // De NINA-module is een aparte chip; zonder werkende module kan er
  // geen enkele wifi-functie draaien.
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return;
  }

  // Informatief: String-vergelijking volstaat voor versienummers als "1.5.0".
  const String firmware = WiFi.firmwareVersion();
  if (firmware < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Er is nieuwere NINA-firmware beschikbaar.");
  }
  connectToWiFi();
}

void loop() {
  // Wifi kan wegvallen (router herstart, bereik). Controleer daarom
  // regelmatig en verbind opnieuw als dat nodig is.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Verbinding verloren; opnieuw proberen.");
    connectToWiFi();
  }
  delay(5000);
}
