#include <WiFiNINA.h>

#include "secrets.h"

const unsigned long WIFI_TIMEOUT_MS = 20000;

bool wifiHardwareAvailable() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return false;
  }

  const String firmware = WiFi.firmwareVersion();
  if (firmware < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Er is nieuwere NINA-firmware beschikbaar.");
  }
  return true;
}

bool connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Verbinden met ");
  Serial.print(WIFI_SSID);

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
  delay(1000);

  if (wifiHardwareAvailable()) {
    connectToWiFi();
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Verbinding verloren; opnieuw proberen.");
    connectToWiFi();
  }
  delay(5000);
}
