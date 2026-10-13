#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secrets.h"

const char SERVER_HOST[] = "arcade.rogerthat.be";
const int SERVER_PORT = 80;
const unsigned long WIFI_TIMEOUT_MS = 20000;

WiFiClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);
String playerToken;

bool wifiHardwareAvailable() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Geen NINA-wifimodule gevonden.");
    return false;
  }
  return true;
}

bool connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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

bool checkServerHealth() {
  const int requestError = http.get("/api/v1/health");
  if (requestError != 0) {
    Serial.print("Healthcheck kon niet worden verstuurd: ");
    Serial.println(requestError);
    http.stop();
    return false;
  }

  const int statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  http.stop();

  if (statusCode != 200) {
    Serial.print("Healthcheck gaf HTTP ");
    Serial.println(statusCode);
    Serial.println(body);
    return false;
  }

  JsonDocument document;
  if (deserializeJson(document, body)) return false;

  Serial.print("Serverstatus: ");
  Serial.println(document["status"] | "onbekend");
  return document["ok"] | false;
}

String makeJoinPayload() {
  JsonDocument document;
  document["name"] = PLAYER_NAME;
  document["color"] = PLAYER_COLOR;
  document["key"] = PLAYER_KEY;

  String payload;
  serializeJson(document, payload);
  return payload;
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

bool registerPlayer() {
  int statusCode = 0;
  const String body = postJson(
    "/api/v1/players", makeJoinPayload(), statusCode);

  Serial.print("Registratiestatus: ");
  Serial.println(statusCode);
  Serial.println(body);

  if (statusCode != 201) return false;

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, body);
  if (error) {
    Serial.print("JSON-fout: ");
    Serial.println(error.c_str());
    return false;
  }

  const char* token = document["playerToken"].as<const char*>();
  if (token == nullptr || token[0] == '\0') {
    Serial.println("playerToken ontbreekt.");
    return false;
  }

  playerToken = token;
  Serial.println("Speler aangemeld; token staat in RAM.");
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  http.setHttpResponseTimeout(5000);

  if (wifiHardwareAvailable() &&
      connectToWiFi() &&
      checkServerHealth()) {
    registerPlayer();
  }
}

void loop() {}
