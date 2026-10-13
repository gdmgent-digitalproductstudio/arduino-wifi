#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;
const byte SENSOR_PIN = A0;

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
  return WiFi.status() == WL_CONNECTED;
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

String postJson(const char* path, const String& payload,
                int& statusCode) {
  http.beginRequest();
  const int requestError = http.post(path);
  if (requestError != 0) {
    statusCode = requestError;
    http.stop();
    return "";
  }

  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", payload.length());
  http.beginBody();
  http.print(payload);
  http.endRequest();

  statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  http.stop();
  return body;
}

void sendMessage() {
  JsonDocument document;
  document["text"] = "Mijn Arduino is online";
  document["sender"] = DEVICE_NAME;

  String payload;
  serializeJson(document, payload);

  int statusCode = 0;
  const String body =
    postJson("/api/v1/message", payload, statusCode);

  Serial.print("POST message: ");
  Serial.println(statusCode);
  Serial.println(body);
}

void readCurrentMessage() {
  int statusCode = 0;
  const String body = getJson("/api/v1/message", statusCode);

  Serial.print("GET message: ");
  Serial.println(statusCode);
  Serial.println(body);
}

void sendTelemetry() {
  JsonDocument document;
  document["device"] = DEVICE_NAME;
  document["sensor"] = "analog-A0";
  document["value"] = analogRead(SENSOR_PIN);
  document["unit"] = "raw";

  String payload;
  serializeJson(document, payload);

  int statusCode = 0;
  const String body =
    postJson("/api/v1/telemetry", payload, statusCode);

  Serial.print("POST telemetry: ");
  Serial.println(statusCode);
  Serial.println(body);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  networkClient.setInsecure();
  http.setHttpResponseTimeout(5000);

  if (!connectToWiFi()) return;
  sendMessage();
  readCurrentMessage();
  sendTelemetry();
}

void loop() {}
