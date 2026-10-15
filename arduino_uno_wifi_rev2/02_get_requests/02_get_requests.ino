#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>

#include "secrets.h"

const char SERVER_HOST[] = "iot.rogerthat.be";
const int SERVER_PORT = 443;  // 443 = HTTPS
const unsigned long WIFI_TIMEOUT_MS = 20000;
const unsigned long HTTP_TIMEOUT_MS = 5000;

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

  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
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

void printRequest(const char* path) {
  int statusCode = 0;
  const String body = getJson(path, statusCode);

  // Hier tonen we bewust elke statuscode en body: in deze stap leer je
  // wat de server terugstuurt, ook bij fouten (404, 500, ...).
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
  delay(1000);  // wacht op de USB-seriële verbinding (115200 baud)

  // De NINA-module is een aparte chip; zonder werkende module kan er
  // geen enkele wifi-functie draaien.
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return;
  }
  http.setHttpResponseTimeout(HTTP_TIMEOUT_MS);

  // Elke stap hieronder heeft de vorige nodig; stop dus bij de eerste fout.
  if (!connectToWiFi()) return;
  printRequest("/api/v1/health");
  printRequest("/api/v1/time");
}

void loop() {}  // Alles gebeurt eenmalig in setup().
