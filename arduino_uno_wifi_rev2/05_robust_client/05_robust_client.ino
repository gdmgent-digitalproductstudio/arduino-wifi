#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;
const byte SENSOR_PIN = A0;
const unsigned long SEND_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_MS = 5000;

WiFiSSLClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

int currentSensorValue = 0;
unsigned long lastSendAt = 0;
unsigned long lastWifiAttemptAt = 0;

bool connectToWiFiBlocking() {
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

void maintainWiFi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) return;
  if (now - lastWifiAttemptAt < WIFI_RETRY_MS) return;

  lastWifiAttemptAt = now;
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Nieuwe wifi-poging gestart.");
}

void readLocalSensor() {
  currentSensorValue = analogRead(SENSOR_PIN);
  digitalWrite(
    LED_BUILTIN,
    currentSensorValue > 512 ? HIGH : LOW);
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

void sendTelemetry() {
  JsonDocument document;
  document["device"] = DEVICE_NAME;
  document["sensor"] = "analog-A0";
  document["value"] = currentSensorValue;
  document["unit"] = "raw";

  String payload;
  serializeJson(document, payload);

  int statusCode = 0;
  const String body =
    postJson("/api/v1/telemetry", payload, statusCode);

  if (statusCode == 201) {
    Serial.println("Meting ontvangen door de server.");
  } else if (statusCode < 0) {
    Serial.print("Transportfout: ");
    Serial.println(statusCode);
  } else {
    Serial.print("HTTP-fout ");
    Serial.print(statusCode);
    Serial.print(": ");
    Serial.println(body);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return;
  }

  http.setHttpResponseTimeout(5000);
  connectToWiFiBlocking();
}

void loop() {
  const unsigned long now = millis();
  readLocalSensor();
  maintainWiFi(now);

  if (WiFi.status() != WL_CONNECTED) return;
  if (now - lastSendAt < SEND_INTERVAL_MS) return;

  lastSendAt = now;
  sendTelemetry();
}
