/**
 * debug_network_rev2.ino
 *
 * HTTPS-client voor Arduino Uno WiFi Rev2 (u-blox NINA-W102).
 * Verbindt met WiFi, controleert de verbinding laag per laag en
 * haalt periodiek JSON op van https://iot.rogerthat.be/api/v1/...
 *
 * Fasen:
 *   1. WiFi-module controleren + firmwareversie vergelijken
 *   2. Netwerken scannen (signaalsterkte, kanaal, beveiliging)
 *   3. Verbinden met retry en volledige module-reset tussen pogingen
 *   4. Diagnose: DNS, ping naar gateway, TLS-test naar twee hosts
 *   5. Periodiek GET-verzoeken via ArduinoHttpClient
 *
 * Zie README.md voor installatie en probleemoplossing.
 */

#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>

#include "secrets.h"  // WIFI_SSID en WIFI_PASSWORD

// ---------------------------------------------------------------------------
// Configuratie
// ---------------------------------------------------------------------------
namespace cfg {
const char     HOST[]            = "iot.rogerthat.be";
const uint16_t PORT              = 443;

const uint8_t  WIFI_ATTEMPTS     = 5;        // aantal verbindingspogingen
const uint32_t WIFI_TIMEOUT_MS   = 15000;    // wachttijd per poging
const uint32_t DHCP_TIMEOUT_MS   = 5000;     // wachttijd op een IP-adres

const uint8_t  HTTP_ATTEMPTS     = 3;        // pogingen per GET (incl. TLS-handshake)
const uint32_t HTTP_TIMEOUT_MS   = 10000;    // wachttijd op antwoord
const size_t   MAX_BODY          = 256;      // buffergrootte; Uno WiFi Rev2 heeft maar 6 KB RAM

const uint32_t POLL_INTERVAL_MS  = 30000;    // tijd tussen twee rondes
const bool     RUN_DIAGNOSTICS   = true;     // false = sneller starten, minder uitvoer
}  // namespace cfg

// ---------------------------------------------------------------------------
// Globale objecten
// Eén SSL-client voor alles: de NINA-module heeft een beperkt aantal sockets.
// ---------------------------------------------------------------------------
WiFiSSLClient sslClient;
HttpClient    http(sslClient, cfg::HOST, cfg::PORT);

// ---------------------------------------------------------------------------
// Tekstuele beschrijvingen van codes (in flash, niet in RAM)
// ---------------------------------------------------------------------------
const __FlashStringHelper* wifiStatusText(uint8_t s) {
  switch (s) {
    case WL_NO_MODULE:       return F("geen module gevonden");
    case WL_IDLE_STATUS:     return F("idle (wacht op verbinding)");
    case WL_NO_SSID_AVAIL:   return F("SSID niet gevonden (signaal te zwak, verkeerde band of verborgen SSID)");
    case WL_SCAN_COMPLETED:  return F("scan voltooid");
    case WL_CONNECTED:       return F("verbonden");
    case WL_CONNECT_FAILED:  return F("verbinden mislukt (wachtwoord of beveiligingstype)");
    case WL_CONNECTION_LOST: return F("verbinding verbroken");
    case WL_DISCONNECTED:    return F("niet verbonden");
    default:                 return F("onbekende status");
  }
}

const __FlashStringHelper* encryptionText(int t) {
  switch (t) {
    case ENC_TYPE_TKIP: return F("WPA (TKIP)");
    case ENC_TYPE_CCMP: return F("WPA2 (AES)");
    case ENC_TYPE_WEP:  return F("WEP");
    case ENC_TYPE_NONE: return F("open");
    case ENC_TYPE_AUTO: return F("auto");
    default:            return F("onbekend");
  }
}

const __FlashStringHelper* rssiText(int32_t rssi) {
  if (rssi > -60) return F("goed");
  if (rssi > -70) return F("redelijk");
  if (rssi > -80) return F("zwak");
  return F("te zwak voor stabiele TLS");
}

const __FlashStringHelper* httpErrorText(int e) {
  switch (e) {
    case HTTP_ERROR_CONNECTION_FAILED: return F("verbinding/TLS-handshake mislukt");
    case HTTP_ERROR_API:               return F("API-fout");
    case HTTP_ERROR_TIMED_OUT:         return F("time-out");
    case HTTP_ERROR_INVALID_RESPONSE:  return F("ongeldig antwoord");
    default:                           return F("onbekende fout");
  }
}

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------

/** Controleert of de NINA-module reageert en toont de firmwareversie. */
bool checkWifiModule() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(F("[module] FOUT: geen NINA-wifimodule gevonden."));
    return false;
  }
  const char* fw = WiFi.firmwareVersion();
  Serial.print(F("[module] firmware: "));
  Serial.print(fw);
  Serial.print(F(" (bibliotheek verwacht "));
  Serial.print(WIFI_FIRMWARE_LATEST_VERSION);
  Serial.println(')');
  if (strcmp(fw, WIFI_FIRMWARE_LATEST_VERSION) != 0) {
    Serial.println(F("[module] WAARSCHUWING: firmware en bibliotheek komen niet overeen."));
  }
  return true;
}

/** Toont alle zichtbare netwerken en markeert het doelnetwerk. */
void scanNetworks() {
  Serial.println(F("[scan] netwerken zoeken..."));
  const int n = WiFi.scanNetworks();
  if (n <= 0) {
    Serial.println(F("[scan] geen netwerken gevonden: radio of antenne probleem?"));
    return;
  }

  bool found = false;
  for (int i = 0; i < n; i++) {
    const bool match = strcmp(WiFi.SSID(i), WIFI_SSID) == 0;
    found = found || match;
    Serial.print(match ? F("  > ") : F("    "));
    Serial.print(WiFi.SSID(i));
    Serial.print(F("  kanaal "));
    Serial.print(WiFi.channel(i));
    Serial.print(F("  "));
    Serial.print(WiFi.RSSI(i));
    Serial.print(F(" dBm ("));
    Serial.print(rssiText(WiFi.RSSI(i)));
    Serial.print(F(")  "));
    Serial.println(encryptionText(WiFi.encryptionType(i)));
  }
  if (!found) {
    Serial.print(F("[scan] WAARSCHUWING: \""));
    Serial.print(WIFI_SSID);
    Serial.println(F("\" is niet zichtbaar. Controleer SSID, band (2,4 GHz) en afstand."));
  }
}

/** Wacht tot DHCP een IP-adres heeft toegekend. */
bool waitForIp(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && millis() - start < timeoutMs) {
    delay(100);
  }
  return WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

void printLinkInfo() {
  Serial.print(F("[wifi] verbonden met "));
  Serial.println(WiFi.SSID());
  Serial.print(F("[wifi] IP: "));
  Serial.print(WiFi.localIP());
  Serial.print(F("  gateway: "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("[wifi] signaal: "));
  Serial.print(WiFi.RSSI());
  Serial.print(F(" dBm ("));
  Serial.print(rssiText(WiFi.RSSI()));
  Serial.println(')');
}

/**
 * Verbindt met WiFi. Bij elke mislukte poging wordt de module volledig
 * gereset (WiFi.end), omdat een vastgelopen module anders mislukt blijft.
 */
bool connectToWiFi() {
  for (uint8_t attempt = 1; attempt <= cfg::WIFI_ATTEMPTS; attempt++) {
    Serial.print(F("[wifi] poging "));
    Serial.print(attempt);
    Serial.print('/');
    Serial.println(cfg::WIFI_ATTEMPTS);

    WiFi.noLowPowerMode();  // geen powersave: stabieler bij zwak signaal
    WiFi.disconnect();
    delay(300);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < cfg::WIFI_TIMEOUT_MS) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      if (waitForIp(cfg::DHCP_TIMEOUT_MS)) {
        printLinkInfo();
        return true;
      }
      Serial.println(F("[wifi] verbonden, maar geen IP-adres via DHCP gekregen."));
    } else {
      Serial.print(F("[wifi] mislukt, status "));
      Serial.print(WiFi.status());
      Serial.print(F(": "));
      Serial.println(wifiStatusText(WiFi.status()));
    }

    WiFi.end();  // module volledig resetten
    delay(1000);
  }
  Serial.println(F("[wifi] opgegeven na alle pogingen."));
  return false;
}

// ---------------------------------------------------------------------------
// Diagnose
// ---------------------------------------------------------------------------

/** Test enkel de TLS-handshake naar een host (stuurt geen HTTP-verzoek). */
bool probeTls(const char* host) {
  Serial.print(F("[tls] "));
  Serial.print(host);
  Serial.print(F(": "));
  sslClient.stop();
  const uint32_t t0 = millis();
  const bool ok = sslClient.connect(host, 443);
  Serial.print(ok ? F("OK") : F("MISLUKT"));
  Serial.print(F(" ("));
  Serial.print(millis() - t0);
  Serial.println(F(" ms)"));
  sslClient.stop();
  return ok;
}

/**
 * Test lagen van onder naar boven. De eerste laag die faalt is de oorzaak:
 *   ping gateway faalt  -> lokaal wifi-probleem (signaal, router)
 *   DNS faalt           -> DNS/internet
 *   TLS arduino.cc OK, eigen server faalt -> certificaat van jouw server
 *   TLS beide falen     -> signaal, voeding of firmware
 */
void runDiagnostics() {
  Serial.println(F("[diag] --- start ---"));

  const int pingMs = WiFi.ping(WiFi.gatewayIP());
  Serial.print(F("[diag] ping gateway: "));
  if (pingMs >= 0) {
    Serial.print(pingMs);
    Serial.println(F(" ms"));
  } else {
    Serial.print(F("MISLUKT (code "));
    Serial.print(pingMs);
    Serial.println(')');
  }

  IPAddress ip;
  Serial.print(F("[diag] DNS "));
  Serial.print(cfg::HOST);
  Serial.print(F(": "));
  if (WiFi.hostByName(cfg::HOST, ip)) {
    Serial.println(ip);
  } else {
    Serial.println(F("MISLUKT"));
  }

  probeTls("arduino.cc");
  probeTls(cfg::HOST);
  Serial.println(F("[diag] --- einde ---"));
}

// ---------------------------------------------------------------------------
// HTTP
// ---------------------------------------------------------------------------

/**
 * Voert een GET uit met retry.
 * @return HTTP-statuscode (>0) of een negatieve ArduinoHttpClient-fout.
 * @param body      buffer voor de response body (altijd 0-terminated)
 * @param truncated wordt true als de body groter was dan de buffer
 */
int httpGet(const char* path, char* body, size_t bodySize, bool& truncated) {
  body[0] = '\0';
  truncated = false;
  http.setHttpResponseTimeout(cfg::HTTP_TIMEOUT_MS);

  int err = HTTP_ERROR_CONNECTION_FAILED;
  for (uint8_t attempt = 1; attempt <= cfg::HTTP_ATTEMPTS; attempt++) {
    err = http.get(path);
    if (err == HTTP_SUCCESS) break;

    Serial.print(F("[http] poging "));
    Serial.print(attempt);
    Serial.print('/');
    Serial.print(cfg::HTTP_ATTEMPTS);
    Serial.print(F(" mislukt: "));
    Serial.println(httpErrorText(err));
    http.stop();
    delay(1000UL * attempt);  // oplopende wachttijd
  }
  if (err != HTTP_SUCCESS) return err;

  const int status = http.responseStatusCode();
  if (status < 0) {
    http.stop();
    return status;
  }

  http.skipResponseHeaders();

  size_t len = 0;
  uint32_t lastData = millis();
  while ((http.connected() || http.available()) &&
         millis() - lastData < cfg::HTTP_TIMEOUT_MS) {
    if (http.available()) {
      const int c = http.read();
      if (c < 0) continue;
      if (len < bodySize - 1) body[len++] = static_cast<char>(c);
      else truncated = true;
      lastData = millis();
    } else {
      delay(5);
    }
  }
  body[len] = '\0';
  http.stop();
  return status;
}

void pollEndpoint(const char* path) {
  char body[cfg::MAX_BODY];
  bool truncated;
  const int status = httpGet(path, body, sizeof(body), truncated);

  Serial.print(F("[api] GET "));
  Serial.print(path);
  Serial.print(F(" -> "));
  if (status > 0) {
    Serial.println(status);
    Serial.print(F("[api] body: "));
    Serial.print(body);
    if (truncated) Serial.print(F(" ...(ingekort)"));
    Serial.println();
  } else {
    Serial.print(F("fout "));
    Serial.print(status);
    Serial.print(F(" ("));
    Serial.print(httpErrorText(status));
    Serial.println(')');
  }
}

// ---------------------------------------------------------------------------
// Arduino-ingangspunten
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 2000) {}  // max 2 s wachten op seriële monitor
  Serial.println(F("\n=== iot_client gestart ==="));

  if (!checkWifiModule()) while (true) delay(1000);
  if (cfg::RUN_DIAGNOSTICS) scanNetworks();
}

void loop() {
  static bool firstRun = true;
  static uint32_t lastRun = 0;
  if (!firstRun && millis() - lastRun < cfg::POLL_INTERVAL_MS) return;
  firstRun = false;
  lastRun = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (!connectToWiFi()) return;  // volgende ronde opnieuw proberen
    if (cfg::RUN_DIAGNOSTICS) runDiagnostics();
  }

  pollEndpoint("/api/v1/health");
  pollEndpoint("/api/v1/time");
}
