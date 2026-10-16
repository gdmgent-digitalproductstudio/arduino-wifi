# Arduino & internet

Volledige voorbeeldsketches voor de les Arduino & Internet van Digital Product Studio.

De voorbeelden bestaan voor twee boards:

- `esp32`: Adafruit ESP32 Feather V2;
- `arduino_uno_wifi_rev2`: Arduino Uno WiFi Rev2.

Beide varianten gebruiken dezelfde HTTP-library, [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient). Alleen de wifi-driver verschilt: `WiFi.h` voor de ESP32 en `WiFiNINA.h` voor de Uno WiFi Rev2.

## Installeren

Installeer via de Library Manager van Arduino IDE:

- ArduinoHttpClient;
- ArduinoJson, versie 7 of recenter;
- WiFiNINA voor de Uno WiFi Rev2.

Installeer daarnaast het juiste boardpakket:

- `esp32 by Espressif Systems` voor de ESP32 Feather V2;
- `Arduino megaAVR Boards` voor de Uno WiFi Rev2.

### Uno WiFi Rev2: firmware en HTTPS-certificaat

Werk vóór de HTTPS-voorbeelden ook de NINA-W102-module bij:

1. sluit de Serial Monitor;
2. open **Tools > Firmware Updater** in Arduino IDE 2;
3. selecteer de Uno WiFi Rev2, controleer op updates en installeer de nieuwste firmware;
4. upload daarna via **Tools > Upload Root Certificates** het certificaat voor `https://iot.rogerthat.be`.

Controleer de firmware met **File > Examples > WiFiNINA > Tools > CheckFirmwareVersion**. Firmware en certificaten zijn afzonderlijk: actuele firmware alleen volstaat niet om de server bij een TLS-handshake te vertrouwen.

Open steeds de `.ino` in de map met dezelfde naam. Vul daarna de dummywaarden in `secrets.h` in en kies voor `DEVICE_NAME` een pseudoniem voor je team of bord.

## Voor je begint

- Zet de Serial Monitor op **9600 baud**. Alle sketches gebruiken `Serial.begin(9600)`; een andere waarde geeft onleesbare tekens.
- `secrets.h` bevat je wifi-gegevens. Commit die niet met echte waarden.
- Het bord werkt alleen op **2,4GHz-wifi**.
- De sketches zijn bewust zelfstandig: sommige hulpfuncties (zoals `connectToWiFi`) staan daarom in elke map opnieuw.

## Opbouw

| Map | Inhoud |
| --- | --- |
| `01_wifi_connect` | Verbinden met wifi en netwerkgegevens tonen |
| `02_get_requests` | De IoT API via HTTPS bereiken en GET-responses bekijken |
| `03_json_reading` | JSON lezen, geneste weerdata selecteren en een led aansturen |
| `04_post_data` | Een bericht en een echte analoge meting versturen |
| `05_robust_client` | Metingen begrensd versturen en wifi-uitval opvangen |

## Stapsgewijs

### 1. `01_wifi_connect`: verbinden

- `WiFi.begin(...)` start de verbinding; die komt asynchroon tot stand, dus we wachten in een lus.
- De lus heeft een **time-out** (`WIFI_TIMEOUT_MS`). Zonder time-out hangt het bord eeuwig bij een fout wachtwoord.
- `loop()` controleert elke 5 seconden of de verbinding er nog is en herstelt ze indien nodig.
- Uno WiFi Rev2: de wifi zit op een aparte NINA-chip. `WL_NO_MODULE` betekent dat die niet bereikbaar is; dan heeft verder gaan geen zin.
- ESP32: `WiFi.mode(WIFI_STA)` zet het bord in station-modus (client).

### 2. `02_get_requests`: een GET-request

- Gebruikt HTTPS (poort 443) via `WiFiSSLClient` (Uno) of `WiFiClientSecure` (ESP32).
- ESP32: `setInsecure()` slaat de certificaatcontrole over. Dat is alleen aanvaardbaar voor deze oefening.
- `getJson` geeft de statuscode apart terug. Een **negatieve statuscode** betekent: de request zelf mislukte (geen serverantwoord). `200` betekent geslaagd, `404` niet gevonden enz.
- Deze sketch toont bewust elke status en body, zodat je ziet wat de server teruggeeft.
- `http.stop()` na elke request sluit de verbinding zodat de volgende request schoon start.

### 3. `03_json_reading`: JSON verwerken

- `getBody` controleert op **één plaats** of de request gelukt is (transport én HTTP 200). De functies erna verwerken enkel nog JSON.
- `deserializeJson` geeft een `DeserializationError` terug. In deze sketches testen we dat altijd expliciet (`if (error)`); de verkorte vorm `if (deserializeJson(...)) return;` werkt ook, maar is verwarrend: de functie geeft bij **succes** een lege fout terug (evalueert naar `false`) en bij **mislukking** een waarde die als `true` telt.
- `document["sleutel"] | standaard` leest een waarde en valt terug op de standaardwaarde als de sleutel ontbreekt. Zo crasht de sketch niet op onverwachte data.
- Een **filter** (`DeserializationOption::Filter`) bewaart alleen de velden die je nodig hebt. Het geheugen van een Uno is klein, dus dat is nodig bij grote antwoorden.
- De ingebouwde led volgt het veld `daylight`.

### 4. `04_post_data`: data versturen

- Een POST heeft `Content-Type: application/json` en `Content-Length` nodig; de server moet weten wat en hoeveel er komt.
- JSON bouw je met `JsonDocument` en `serializeJson`, niet met handmatige tekstconcatenatie.
- De analoge meting is een ruwe ADC-waarde: 0-1023 op de Uno (10-bit), 0-4095 op de ESP32 (12-bit).
- **ESP32:** de sketch leest pin `A2`, niet `A0`. A0 en A1 horen bij ADC2, die je niet kunt lezen zolang wifi aan staat. A2 hoort bij ADC1.

### 5. `05_robust_client`: betrouwbaar blijven werken

- Geen `delay()` in `loop()`: tijden worden met `millis()` bijgehouden, zodat de sensor en led altijd blijven reageren.
- `maintainWiFi()` start hooguit elke `WIFI_RETRY_MS` een nieuwe poging en blokkeert niet.
- Een mislukte eerste verbinding is geen eindpunt; `loop()` herstelt ze later.
- Het resultaat van elke POST is één van drie: `201` (gelukt), negatief (geen antwoord) of een andere HTTP-status (serverfout).
- De led-drempel is de helft van het meetbereik: 512 (Uno) of 2048 (ESP32).

## Veelvoorkomende problemen

| Symptoom | Oorzaak |
| --- | --- |
| Onleesbare tekens | Serial Monitor niet op 9600 baud |
| Alleen puntjes, dan "mislukt" | Fout SSID/wachtwoord, of 5GHz-netwerk |
| Negatieve statuscode | Geen verbinding met de server (netwerk, DNS, time-out) |
| ESP32 leest altijd 0 of ruis op A0 | ADC2 werkt niet met wifi; gebruik A2 |
| Uno: "Geen NINA-wifimodule" | Firmware of verbinding met de wifichip; update via *Tools > WiFi101 / WiFiNINA Firmware Updater* |

## Extra voorbeelden

De map `examples` bevat oefeningen die losstaan van de opeenvolgende cursusvoorbeelden:

- wifi-netwerken scannen;
- een kleine webserver bouwen waarmee je de ingebouwde led bedient;
- de actuele positie van het ISS ophalen en als JSON verwerken.

## Documentatie

- [Adafruit ESP32 Feather V2 pinouts](https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/pinouts)
- [Arduino Uno WiFi Rev2](https://docs.arduino.cc/hardware/uno-wifi-rev2/)
- [Arduino-ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [WiFiNINA](https://docs.arduino.cc/libraries/wifinina/)
- [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient)
- [ArduinoJson 7](https://arduinojson.org/v7/tutorial/)
- [IoT Field Station API](https://iot.rogerthat.be/docs/)

De IoT API gebruikt HTTPS en vraagt geen token. Gebruik in berichten en telemetry alleen een team- of apparaatnaam, nooit persoonsgegevens.
