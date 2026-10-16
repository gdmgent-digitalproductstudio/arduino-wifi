#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;  // 443 = HTTPS
const unsigned long WIFI_TIMEOUT_MS = 20000;
const unsigned long HTTP_TIMEOUT_MS = 5000;

// WiFiClientSecure = TLS-verbinding (HTTPS).
WiFiClientSecure networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

bool connectToWiFi() {
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

// Haalt de body op, maar alleen bij een geslaagde request (HTTP 200).
// Deze ene functie doet alle foutcontrole voor het ophalen, zodat de
// aanroepers hieronder enkel nog de JSON hoeven te verwerken.
bool getBody(const char* path, String& body) {
  const int requestError = http.get(path);
  if (requestError != 0) {
    Serial.print("Request mislukt, foutcode ");
    Serial.println(requestError);
    http.stop();
    return false;
  }

  const int statusCode = http.responseStatusCode();
  body = http.responseBody();
  http.stop();

  if (statusCode != 200) {
    Serial.print(path);
    Serial.print(" gaf HTTP ");
    Serial.println(statusCode);
    return false;
  }
  return true;
}

bool printHealth() {
  String body;
  if (!getBody("/api/v1/health", body)) return false;

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, body);
  if (error) {
    Serial.println(error.c_str());
    return false;
  }

  // `waarde | standaard` leest de waarde en valt terug op de standaard
  // als de sleutel ontbreekt of een ander type heeft.
  Serial.print("Service: ");
  Serial.println(document["service"] | "onbekend");
  Serial.print("ServerTime: ");
  Serial.println(document["serverTime"] | "onbekend");
  return document["ok"] | false;
}

void printJoke() {
  String body;
  if (!getBody("/api/v1/joke", body)) return;

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, body);
  if (error) {
    Serial.println(error.c_str());
    return;
  }

  Serial.println(document["setup"] | "Geen setup");
  Serial.println(document["punchline"] | "Geen punchline");
}

void showWeatherForGhent() {
  String body;
  if (!getBody("/api/v1/weather?latitude=51.05&longitude=3.72", body)) {
    return;
  }

  // Het filter bepaalt welke velden bewaard blijven. Het weerbericht is
  // groot en het geheugen van het bord klein: zo parsen we alleen wat
  // we echt nodig hebben.
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

  // Geneste velden lees je door de sleutels na elkaar te kiezen.
  // Ontbreekt er een, dan krijg je de standaardwaarde na de `|`.
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
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);  // wacht op de USB-seriële verbinding (9600 baud)

  // HTTPS is versleuteld, maar het servercertificaat wordt in deze
  // oefenopstelling niet gecontroleerd. Doe dit niet in een echt product.
  networkClient.setInsecure();
  http.setHttpResponseTimeout(HTTP_TIMEOUT_MS);

  // Elke stap hieronder heeft de vorige nodig; stop dus bij de eerste fout.
  if (!connectToWiFi()) return;
  if (!printHealth()) return;
  printJoke();
  showWeatherForGhent();
}

void loop() {}  // Alles gebeurt eenmalig in setup().
