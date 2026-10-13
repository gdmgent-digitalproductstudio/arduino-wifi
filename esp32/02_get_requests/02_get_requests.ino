#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;

WiFiClientSecure networkClient;
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

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi-verbinding mislukt.");
    return false;
  }

  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
  return true;
}

String getJson(const char* path, int& statusCode) {
  const int requestError = http.get(path);
  if (requestError != 0) {
    statusCode = requestError;
    http.stop();
    return "";
  }

  statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  http.stop();
  return body;
}

void printRequest(const char* path) {
  int statusCode = 0;
  const String body = getJson(path, statusCode);

  Serial.print("GET ");
  Serial.println(path);
  Serial.print("HTTP-status: ");
  Serial.println(statusCode);
  Serial.print("Body: ");
  Serial.println(body);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // HTTPS is versleuteld, maar het certificaat wordt in deze
  // oefenopstelling niet gecontroleerd.
  networkClient.setInsecure();
  http.setHttpResponseTimeout(5000);

  if (!connectToWiFi()) return;
  printRequest("/api/v1/health");
  printRequest("/api/v1/time");
}

void loop() {}
