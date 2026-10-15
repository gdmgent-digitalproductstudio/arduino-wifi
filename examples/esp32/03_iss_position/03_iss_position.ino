#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "secrets.h"

const char SERVER_HOST[] = "api.open-notify.org";
const int SERVER_PORT = 80;  // gewone HTTP: deze dienst biedt geen HTTPS aan
const unsigned long WIFI_TIMEOUT_MS = 20000;
const unsigned long HTTP_TIMEOUT_MS = 5000;

WiFiClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < WIFI_TIMEOUT_MS) {
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

  // Eerst de status controleren: alleen bij 200 is de body geldige JSON.
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

  // Open Notify stuurt de coördinaten als tekst (\"12.3\"), niet als getal;
  // daarom lezen we ze als string.
  Serial.print("ISS latitude: ");
  Serial.println(document["iss_position"]["latitude"].as<const char*>());
  Serial.print("ISS longitude: ");
  Serial.println(document["iss_position"]["longitude"].as<const char*>());
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // wacht op de USB-seriële verbinding (115200 baud)
  http.setHttpResponseTimeout(HTTP_TIMEOUT_MS);

  if (connectToWiFi()) printIssPosition();
}

void loop() {}
