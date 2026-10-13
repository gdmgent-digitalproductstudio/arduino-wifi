#include <WiFiNINA.h>

#include "secrets.h"

WiFiServer server(80);
bool ledOn = false;

bool connectToWiFi() {
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

void sendPage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println("<!doctype html><html><body>");
  client.print("<h1>Led is ");
  client.print(ledOn ? "aan" : "uit");
  client.println("</h1>");
  client.println("<p><a href='/on'>Aan</a></p>");
  client.println("<p><a href='/off'>Uit</a></p>");
  client.println("</body></html>");
}

void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  client.setTimeout(1000);
  const String requestLine = client.readStringUntil('\n');

  while (client.connected()) {
    String headerLine = client.readStringUntil('\n');
    headerLine.trim();
    if (headerLine.length() == 0) break;
  }

  if (requestLine.startsWith("GET /on ")) ledOn = true;
  if (requestLine.startsWith("GET /off ")) ledOn = false;
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

  sendPage(client);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);

  if (WiFi.status() == WL_NO_MODULE || !connectToWiFi()) {
    Serial.println("Wifi niet beschikbaar.");
    return;
  }

  server.begin();
  Serial.print("Open http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  handleClient();
}
