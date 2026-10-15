# Extra voorbeelden

Deze sketches staan los van de opeenvolgende cursusvoorbeelden en tonen andere toepassingen van wifi en HTTP.

## 01_wifi_scan

Scant de 2,4GHz-netwerken in de omgeving en toont SSID en signaalsterkte. Hiervoor zijn geen wifi-inloggegevens nodig.

## 02_local_led_webserver

Maakt van het Arduino-bord een kleine webserver. Open het getoonde lokale IP-adres in een browser op hetzelfde netwerk en bedien de ingebouwde led via `/on` en `/off`.

## 03_iss_position

Haalt via [Open Notify](http://open-notify.org/Open-Notify-API/ISS-Location-Now/) de actuele breedte- en lengtegraad van het ISS op en verwerkt de JSON-response.

Publieke diensten kunnen wijzigen of tijdelijk onbereikbaar zijn. Controleer bij problemen altijd eerst de documentatie en de ruwe HTTP-response.
