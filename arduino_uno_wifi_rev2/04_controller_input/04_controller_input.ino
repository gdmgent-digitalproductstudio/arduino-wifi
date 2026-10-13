#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secrets.h"

const char SERVER_HOST[] = "arcade.rogerthat.be";
const int SERVER_PORT = 80;

const bool USE_JOYSTICK = true;
const byte X_PIN = A0;
const byte Y_PIN = A1;
const byte SHOOT_PIN = 6;
const byte LEFT_PIN = 2;
const byte RIGHT_PIN = 3;
const byte UP_PIN = 4;
const byte DOWN_PIN = 5;

const int X_MIN = 0;
const int X_CENTER = 512;
const int X_MAX = 1023;
const int Y_MIN = 0;
const int Y_CENTER = 512;
const int Y_MAX = 1023;
const int DEADZONE = 8;

WiFiClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);
String playerToken;
int currentX = 0;
int currentY = 0;
bool currentShoot = false;

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
         millis() - startedAt < 20000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) return false;
  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
  return true;
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
  JsonDocument request;
  request["name"] = PLAYER_NAME;
  request["color"] = PLAYER_COLOR;
  request["key"] = PLAYER_KEY;

  String payload;
  serializeJson(request, payload);

  int statusCode = 0;
  const String body =
    postJson("/api/v1/players", payload, statusCode);
  if (statusCode != 201) {
    Serial.print("Registratiefout ");
    Serial.print(statusCode);
    Serial.print(": ");
    Serial.println(body);
    return false;
  }

  JsonDocument response;
  if (deserializeJson(response, body)) return false;

  const char* token = response["playerToken"].as<const char*>();
  if (token == nullptr) return false;
  playerToken = token;
  return true;
}

int normalizeAxis(int raw, int minimum,
                  int center, int maximum) {
  int value = raw < center
    ? map(raw, minimum, center, -100, 0)
    : map(raw, center, maximum, 0, 100);

  value = constrain(value, -100, 100);
  return abs(value) <= DEADZONE ? 0 : value;
}

int buttonAxis(byte negativePin, byte positivePin) {
  const bool negative = digitalRead(negativePin) == LOW;
  const bool positive = digitalRead(positivePin) == LOW;
  if (negative == positive) return 0;
  return negative ? -100 : 100;
}

void setupInput() {
  pinMode(SHOOT_PIN, INPUT_PULLUP);

  if (!USE_JOYSTICK) {
    pinMode(LEFT_PIN, INPUT_PULLUP);
    pinMode(RIGHT_PIN, INPUT_PULLUP);
    pinMode(UP_PIN, INPUT_PULLUP);
    pinMode(DOWN_PIN, INPUT_PULLUP);
  }
}

void readController() {
  if (USE_JOYSTICK) {
    currentX = normalizeAxis(
      analogRead(X_PIN), X_MIN, X_CENTER, X_MAX);
    currentY = normalizeAxis(
      analogRead(Y_PIN), Y_MIN, Y_CENTER, Y_MAX);
  } else {
    currentX = buttonAxis(LEFT_PIN, RIGHT_PIN);
    currentY = buttonAxis(UP_PIN, DOWN_PIN);
  }
  currentShoot = digitalRead(SHOOT_PIN) == LOW;
}

String makeActionPayload() {
  JsonDocument document;
  document["x"] = currentX;
  document["y"] = currentY;
  document["shoot"] = currentShoot;

  String payload;
  serializeJson(document, payload);
  return payload;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setupInput();
  http.setHttpResponseTimeout(5000);

  if (wifiHardwareAvailable() && connectToWiFi()) {
    registerPlayer();
  }
}

void loop() {
  readController();
  Serial.println(makeActionPayload());
  delay(200);
}
