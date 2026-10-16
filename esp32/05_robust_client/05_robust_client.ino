#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;  // 443 = HTTPS
const unsigned long WIFI_TIMEOUT_MS = 20000;
const unsigned long HTTP_TIMEOUT_MS = 5000;
const unsigned long SEND_INTERVAL_MS = 5000;  // pauze tussen metingen
const unsigned long WIFI_RETRY_MS = 5000;    // pauze tussen wifi-pogingen

// Analoge pin voor de meting. A2 (ADC1) en niet A0: A0 en A1 (ADC2) kunnen
// op de ESP32 niet gelezen worden zolang wifi actief is.
const byte SENSOR_PIN = A2;
// De led gaat aan boven de helft van het meetbereik (12-bit: 0-4095).
const int LED_THRESHOLD = 2048;

// WiFiClientSecure = TLS-verbinding (HTTPS).
WiFiClientSecure networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

int currentSensorValue = 0;
bool wifiModuleOk = false;  // pas true na een geslaagde hardwarecontrole
// Tijden bijhouden met millis() in plaats van delay(): zo blijft loop()
// draaien en lezen we de sensor en de led altijd tijdig uit.
unsigned long lastSendAt = 0;
unsigned long lastWifiAttemptAt = 0;

// Wacht (begrensd) tot de eerste verbinding er is. Alleen gebruikt in
// setup(); in loop() mag wachten niet, daar gebruiken we maintainWiFi().
bool connectToWiFiBlocking() {
  // Station-modus: het bord verbindt met een bestaand netwerk en speelt
  // zelf geen toegangspunt.
  WiFi.mode(WIFI_STA);
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

// Niet-blokkerend: start hooguit om de WIFI_RETRY_MS een nieuwe poging
// en keert meteen terug. De verbinding komt later vanzelf tot stand.
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
    currentSensorValue > LED_THRESHOLD ? HIGH : LOW);
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

void sendTelemetry() {
  JsonDocument document;
  document["device"] = DEVICE_NAME;
  document["sensor"] = "analog-A2";
  document["value"] = currentSensorValue;
  document["unit"] = "raw";

  String payload;
  serializeJson(document, payload);

  int statusCode = 0;
  const String body =
    postJson("/api/v1/telemetry", payload, statusCode);

  // Drie uitkomsten: gelukt (201 Created), geen serverantwoord
  // (negatief) of een foutstatus van de server.
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
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);  // wacht op de USB-seriële verbinding (9600 baud)

  // HTTPS is versleuteld, maar het servercertificaat wordt in deze
  // oefenopstelling niet gecontroleerd. Doe dit niet in een echt product.
  networkClient.setInsecure();
  http.setHttpResponseTimeout(HTTP_TIMEOUT_MS);

  wifiModuleOk = true;
  // Mislukt de eerste verbinding, dan is dat geen eindpunt: loop() blijft
  // via maintainWiFi() opnieuw proberen.
  connectToWiFiBlocking();
}

void loop() {
  const unsigned long now = millis();
  readLocalSensor();  // gebeurt altijd, ook zonder wifi
  if (!wifiModuleOk) return;  // geen wifichip: niets om te herstellen
  maintainWiFi(now);

  if (WiFi.status() != WL_CONNECTED) return;
  if (now - lastSendAt < SEND_INTERVAL_MS) return;

  lastSendAt = now;
  sendTelemetry();
}
