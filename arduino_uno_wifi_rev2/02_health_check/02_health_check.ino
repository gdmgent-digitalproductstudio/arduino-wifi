#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secrets.h"

const char SERVER_HOST[] = "arcade.rogerthat.be";
const int SERVER_PORT = 80;
const unsigned long WIFI_TIMEOUT_MS = 20000;

WiFiClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

bool wifiHardwareAvailable() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return false;
  }
  return true;
}

bool connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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

  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool checkServerHealth() {
  http.setHttpResponseTimeout(5000);

  const int requestError = http.get("/api/v1/health");
  if (requestError != 0) {
    Serial.print("Request kon niet worden verstuurd: ");
    Serial.println(requestError);
    http.stop();
    return false;
  }

  const int statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  http.stop();

  Serial.print("HTTP-status: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(body);

  if (statusCode != 200) return false;

  JsonDocument document;
  const DeserializationError jsonError =
    deserializeJson(document, body);
  if (jsonError) {
    Serial.print("JSON-fout: ");
    Serial.println(jsonError.c_str());
    return false;
  }

  const bool ok = document["ok"] | false;
  const char* status = document["status"] | "onbekend";
  Serial.print("Serverstatus: ");
  Serial.println(status);
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (wifiHardwareAvailable() && connectToWiFi()) {
    checkServerHealth();
  }
}

void loop() {}
