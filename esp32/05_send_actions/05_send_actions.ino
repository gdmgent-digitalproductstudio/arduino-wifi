#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "secrets.h"

const char SERVER_HOST[] = "arcade.rogerthat.be";
const int SERVER_PORT = 80;

const bool USE_JOYSTICK = true;
const byte X_PIN = A0;
const byte Y_PIN = A1;
const byte SHOOT_PIN = 27;
const byte LEFT_PIN = 13;
const byte RIGHT_PIN = 12;
const byte UP_PIN = 33;
const byte DOWN_PIN = 32;

const int X_MIN = 0;
const int X_CENTER = 2048;
const int X_MAX = 4095;
const int Y_MIN = 0;
const int Y_CENTER = 2048;
const int Y_MAX = 4095;
const int DEADZONE = 8;

const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long JOIN_RETRY_MS = 5000;
const unsigned long MIN_SEND_INTERVAL_MS = 200;

WiFiClient networkClient;
HttpClient http(networkClient, SERVER_HOST, SERVER_PORT);

String playerToken;
int currentX = 0;
int currentY = 0;
bool currentShoot = false;
unsigned long lastWifiAttemptAt = 0;
unsigned long lastJoinAttemptAt = 0;
unsigned long lastSendAt = 0;
unsigned long sendIntervalMs = MIN_SEND_INTERVAL_MS;

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

  if (WiFi.status() != WL_CONNECTED) return false;
  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());
  return true;
}

void maintainWiFi(unsigned long now) {
  if (WiFi.status() == WL_CONNECTED) return;
  if (now - lastWifiAttemptAt < WIFI_RETRY_MS) return;

  lastWifiAttemptAt = now;
  playerToken = "";
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Nieuwe wifi-poging gestart.");
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

bool checkServerHealth() {
  const int requestError = http.get("/api/v1/health");
  if (requestError != 0) {
    http.stop();
    return false;
  }

  const int statusCode = http.responseStatusCode();
  const String body = http.responseBody();
  http.stop();
  if (statusCode != 200) return false;

  JsonDocument response;
  if (deserializeJson(response, body)) return false;
  return response["ok"] | false;
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
  if (token == nullptr || token[0] == '\0') return false;

  playerToken = token;
  Serial.println("Speler aangemeld.");
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
  analogReadResolution(12);
  pinMode(SHOOT_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

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

int sendAction() {
  const String path =
    "/api/v1/players/me/act?token=" + playerToken;

  int statusCode = 0;
  const String body =
    postJson(path.c_str(), makeActionPayload(), statusCode);

  digitalWrite(LED_BUILTIN, statusCode == 200 ? HIGH : LOW);

  if (statusCode == 200) {
    if (sendIntervalMs > MIN_SEND_INTERVAL_MS) {
      sendIntervalMs -= 10;
    }
  } else if (statusCode == 401) {
    playerToken = "";
    Serial.println("Token ongeldig; opnieuw aanmelden.");
  } else if (statusCode == 429) {
    if (sendIntervalMs < 1000) sendIntervalMs += 100;
    Serial.print("Te veel requests; nieuw interval: ");
    Serial.println(sendIntervalMs);
  } else {
    Serial.print("Actiefout ");
    Serial.print(statusCode);
    Serial.print(": ");
    Serial.println(body);
  }
  return statusCode;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  setupInput();
  http.setHttpResponseTimeout(3000);

  if (connectToWiFi() && checkServerHealth()) {
    registerPlayer();
  }
}

void loop() {
  const unsigned long now = millis();
  readController();
  maintainWiFi(now);

  if (WiFi.status() != WL_CONNECTED) return;

  if (playerToken.length() == 0) {
    if (now - lastJoinAttemptAt >= JOIN_RETRY_MS) {
      lastJoinAttemptAt = now;
      registerPlayer();
    }
    return;
  }

  if (now - lastSendAt >= sendIntervalMs) {
    lastSendAt = now;
    sendAction();
  }
}
