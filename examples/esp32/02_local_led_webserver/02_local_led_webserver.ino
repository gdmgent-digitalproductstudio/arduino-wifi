#include <WiFi.h>

#include "secrets.h"

const unsigned long WIFI_TIMEOUT_MS = 20000;

WiFiServer server(80);  // 80 = standaard HTTP-poort
bool ledOn = false;

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
  WiFiClient client = server.accept();
  if (!client) return;

  // Zonder time-out blijft readStringUntil wachten op een trage browser.
  client.setTimeout(1000);
  const String requestLine = client.readStringUntil('\n');

  // Lees en negeer de overige headers tot de lege regel; pas daarna
  // mag je antwoorden.
  while (client.connected()) {
    String headerLine = client.readStringUntil('\n');
    headerLine.trim();
    if (headerLine.length() == 0) break;
  }

  // De spatie na het pad is nodig zodat bv. "/online" niet meetelt.
  if (requestLine.startsWith("GET /on ")) ledOn = true;
  if (requestLine.startsWith("GET /off ")) ledOn = false;
  digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);

  sendPage(client);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);  // wacht op de USB-seriële verbinding (115200 baud)

  if (!connectToWiFi()) {
    Serial.println("Wifi-verbinding mislukt.");
    return;
  }

  server.begin();
  Serial.print("Open http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  handleClient();
}
