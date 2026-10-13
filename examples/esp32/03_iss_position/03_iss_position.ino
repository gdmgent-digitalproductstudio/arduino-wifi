#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "secrets.h"

const char SERVER_HOST[] = "api.open-notify.org";
const int SERVER_PORT = 80;

WiFiClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < 20000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

void printIssPosition() {
  const int requestError = http.get("/iss-now.json");
  if (requestError != 0) {
    Serial.print("Requestfout: ");
    Serial.println(requestError);
    http.stop();
    return;
  }

  const int statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  http.stop();

  if (statusCode != 200) {
    Serial.print("HTTP-status: ");
    Serial.println(statusCode);
    Serial.println(body);
    return;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, body);
  if (error) {
    Serial.println(error.c_str());
    return;
  }

  Serial.print("ISS latitude: ");
  Serial.println(document["iss_position"]["latitude"].as<const char*>());
  Serial.print("ISS longitude: ");
  Serial.println(document["iss_position"]["longitude"].as<const char*>());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  http.setHttpResponseTimeout(5000);

  if (connectToWiFi()) printIssPosition();
}

void loop() {}
