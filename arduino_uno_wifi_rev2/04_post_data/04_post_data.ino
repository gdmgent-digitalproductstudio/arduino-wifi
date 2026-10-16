#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;  // 443 = HTTPS
const unsigned long WIFI_TIMEOUT_MS = 20000;
const unsigned long HTTP_TIMEOUT_MS = 5000;

// Analoge pin voor de meting.
const byte SENSOR_PIN = A0;

// WiFiSSLClient = TLS-verbinding (HTTPS) via de NINA-wifimodule.
WiFiSSLClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

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
  return true;
}

// Voert een GET-request uit en geeft de statuscode en body terug.
// Een negatieve statuscode betekent dat de request zelf mislukte
// (verbinding, DNS, time-out) en dat er dus geen serverantwoord is.
String getJson(const char* path, int& statusCode) {
  const int requestError = http.get(path);
  if (requestError != 0) {
    statusCode = requestError;
    http.stop();
    return "";
  }

  statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  // Altijd stoppen na het lezen: zo is de verbinding klaar voor de
  // volgende request.
  http.stop();
  return body;
}

// Verstuurt JSON via POST en geeft statuscode en body terug.
// Een negatieve statuscode betekent dat de request zelf mislukte.
String postJson(const char* path, const String& payload,
                int& statusCode) {
  http.beginRequest();  // nodig om zelf headers en body te mogen sturen
  const int requestError = http.post(path);
  if (requestError != 0) {
    statusCode = requestError;
    http.stop();
    return "";
  }

  http.sendHeader("Content-Type", "application/json");
  // De server moet vooraf weten hoeveel bytes er volgen.
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

  // Statuscode en body worden hier getoond zodat je ziet wat de server
  // terugstuurt; foutafhandeling volgt in 05_robust_client.
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
  document["unit"] = "raw";  // ruwe ADC-waarde, 10-bit: 0-1023

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
  Serial.begin(9600);
  delay(1000);  // wacht op de USB-seriële verbinding (9600 baud)

  // De NINA-module is een aparte chip; zonder werkende module kan er
  // geen enkele wifi-functie draaien.
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return;
  }
  http.setHttpResponseTimeout(HTTP_TIMEOUT_MS);

  // Elke stap hieronder heeft de vorige nodig; stop dus bij de eerste fout.
  if (!connectToWiFi()) return;
  sendMessage();
  readCurrentMessage();
  sendTelemetry();
}

void loop() {}  // Alles gebeurt eenmalig in setup().
