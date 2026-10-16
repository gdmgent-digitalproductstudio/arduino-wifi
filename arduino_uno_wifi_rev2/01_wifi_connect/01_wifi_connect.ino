#include <WiFiNINA.h>

#include "secrets.h"

const unsigned long WIFI_TIMEOUT_MS = 20000;

String getSignalStrength(int rssi) {
  if (rssi == 0) return "onbekend";
  if (rssi >= -50) return "uitstekend";
  if (rssi >= -60) return "goed";
  if (rssi >= -70) return "matig";
  if (rssi >= -80) return "zwak";
  return "zeer zwak";
}

int getSignalStrengthPercent(int rssi) {
  if (rssi == 0) return -1;
  if (rssi <= -100) return 0;
  if (rssi >= -50) return 100;
  return 2 * (rssi + 100);
}

int getRSSI() {
  return WiFi.RSSI();
}

void printWiFiStatus() {
  const int rssi = getRSSI();
  const int signalPercent = getSignalStrengthPercent(rssi);

  Serial.println("Wifi verbonden.");
  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signaalsterkte: ");
  Serial.print(getSignalStrength(rssi));
  Serial.print(" (");
  Serial.print(signalPercent);
  Serial.println("%)");
  Serial.print("dBm: ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

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

  printWiFiStatus();
  return true;
}

void initializeSerial() {
  Serial.begin(9600);
  // Geef de USB-seriële verbinding even de tijd om te starten, anders
  // mis je de eerste regels in de Serial Monitor. Zet de Serial Monitor
  // zelf ook op 9600 baud.
  delay(1000);
}

bool checkWiFiModule() {
  // De NINA-module is een aparte chip; zonder werkende module kan er
  // geen enkele wifi-functie draaien.
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return false;
  }
  return true;
}

void checkFirmwareVersion() {
  // Informatief: String-vergelijking volstaat voor versienummers als "1.5.0".
  const String firmware = WiFi.firmwareVersion();
  if (firmware < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Er is nieuwere NINA-firmware beschikbaar.");
  }
}

void setup() {
  initializeSerial();
  if (!checkWiFiModule()) return;

  checkFirmwareVersion();
  connectToWiFi();
}

void checkWifiAndReconnect() {
  // Controleer of de wifi-verbinding nog actief is. Als dat niet zo is, probeer opnieuw te verbinden.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Verbinding verloren; opnieuw proberen.");
    connectToWiFi();
  }
}

void loop() {
  // Controleer de wifi-verbinding en probeer opnieuw te verbinden indien nodig.
  checkWifiAndReconnect();

  // print signaalsterkte en IP-adres elke 5 seconden
  printWiFiStatus();
  
  delay(5000);
}
