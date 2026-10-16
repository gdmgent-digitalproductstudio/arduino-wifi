#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

const unsigned long WIFI_TIMEOUT_MS = 20000;

bool connectToWiFi() {
  // Station-modus: het bord verbindt met een bestaand netwerk en speelt
  // zelf geen toegangspunt.
  WiFi.mode(WIFI_STA);
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
  Serial.begin(9600);
  // Geef de USB-seriële verbinding even de tijd om te starten, anders
  // mis je de eerste regels in de Serial Monitor. Zet de Serial Monitor
  // zelf ook op 9600 baud.
  delay(1000);

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
