#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
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

bool printHealth() {
  int statusCode = 0;
  const String body = getJson("/api/v1/health", statusCode);
  if (statusCode != 200) return false;

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, body);
  if (error) {
    Serial.println(error.c_str());
    return false;
  }

  Serial.print("Service: ");
  Serial.println(document["service"] | "onbekend");
  Serial.print("ServerTime: ");
  Serial.println(document["serverTime"] | "onbekend");
  return document["ok"] | false;
}

void printJoke() {
  int statusCode = 0;
  const String body = getJson("/api/v1/joke", statusCode);
  if (statusCode != 200) {
    Serial.print("Joke-request gaf HTTP ");
    Serial.println(statusCode);
    return;
  }

  JsonDocument document;
  if (deserializeJson(document, body)) return;

  Serial.println(document["setup"] | "Geen setup");
  Serial.println(document["punchline"] | "Geen punchline");
}

void showWeatherForGhent() {
  int statusCode = 0;
  const String body = getJson(
    "/api/v1/weather?latitude=51.05&longitude=3.72",
    statusCode);
  if (statusCode != 200) {
    Serial.print("Weather-request gaf HTTP ");
    Serial.println(statusCode);
    return;
  }

  JsonDocument filter;
  filter["current"]["temperature"]["value"] = true;
  filter["current"]["temperature"]["unit"] = true;
  filter["current"]["daylight"] = true;

  JsonDocument document;
  const DeserializationError error = deserializeJson(
    document,
    body,
    DeserializationOption::Filter(filter));
  if (error) {
    Serial.println(error.c_str());
    return;
  }

  const float temperature =
    document["current"]["temperature"]["value"] | NAN;
  const char* unit =
    document["current"]["temperature"]["unit"] | "?";
  const bool daylight = document["current"]["daylight"] | false;

  Serial.print("Temperatuur: ");
  Serial.print(temperature);
  Serial.println(unit);
  digitalWrite(LED_BUILTIN, daylight ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);

  networkClient.setInsecure();
  http.setHttpResponseTimeout(5000);

  if (!connectToWiFi()) return;
  if (!printHealth()) return;
  printJoke();
  showWeatherForGhent();
}

void loop() {}
