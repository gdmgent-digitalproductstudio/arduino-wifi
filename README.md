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

Open steeds de `.ino` in de map met dezelfde naam. Vul daarna de dummywaarden in `secrets.h` in. Gebruik voor `PLAYER_KEY` een zelfgekozen, vaste waarde die je niet met anderen deelt.

## Opbouw

Elke map is een volledige, zelfstandige sketch. Je hoeft dus geen bestanden uit een vorige map te kopiëren.

| Map | Inhoud |
| --- | --- |
| `01_wifi_connect` | Verbinden met wifi en netwerkgegevens tonen |
| `02_health_check` | De game-API bereiken met GET en JSON lezen |
| `03_register_player` | JSON versturen, aanmelden en een token bewaren |
| `04_controller_input` | Joystick of D-pad uitlezen en een actiepakket maken |
| `05_send_actions` | Acties versturen, timing begrenzen en fouten opvangen |

## Extra voorbeelden

De map `examples` bevat oefeningen die losstaan van de game-API:

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
- [Game-API](https://arcade.rogerthat.be/docs/)

De game-API gebruikt voor deze les plain HTTP. Gebruik in de voorbeeldgegevens daarom nooit een persoonlijk wachtwoord of een sleutel die je elders hergebruikt.
