# departureboard
**Arduino-Sketch zur Anzeige einer Bahnhofstafel auf einem 20x4-LCD-Display.**

## Aktuelle Features

* Anzeige der nächsten 4 Abfahrten eines Bahnhofes mit Daten der dbf-API von [@derf](https://github.com/derf) unter [https://dbf.finalrewind.org/]
  *  Liniennr., Ziel, Minuten bis zur realen Abfahrt
  * Anzeige von Verspätungen >3 Min durch einen Stern *
* Zugziele werden für eine bessere Darstellung etwas gekürzt
* Aktualisierung ca. alle 1:20 Minuten

* weitere sind geplant

## Voraussetzungen
(das ist jedenfalls meine Hardware, auf der sollte der Sketch ohne Anpassungen laufen)
* NodeMCU CH340 ESP-8266 ([dieses hier](https://de.aliexpress.com/item/1005001636634198.html?spm=a2g0s.9042311.0.0.5ed64c4dZIBFtu))
* 20 x 4 I2C LCD-Display ([jenes dort](https://de.aliexpress.com/item/1005001636301479.html?spm=a2g0s.9042311.0.0.5ed64c4dZIBFtu))
* SD-Card-Leser ([das da](https://de.aliexpress.com/item/2046499166.html?spm=a2g0s.9042311.0.0.5ed64c4dZIBFtu))
  * eine SD-Karte (eine alte 2 GB-Karte, die hier noch rumflog)
* WLAN

Bibliotheken:
* ESP8266WiFi
* ESP8266HTTPClient
* SPI
* SD

* Regexp von Nick Gammon  nickgammon/Regexp 
* LiquidCrystal I2C von Frank de Brabander  johnrickman/LiquidCrystal_I2C 
* NTPClient von Fabrice Weinberg   arduino-libraries/NTPClient 
* ArduinoJSON von Benoit Blanchon   bblanchon/ArduinoJson 

### Aufbau:
**NodeMCU -> SD**
* 3V -> 3V
* G -> GND
* D5 -> SCK
* D6 -> MISO
* D7 -> MOSI
* D8 -> CS

**NodeMCU -> LCD**
* G -> GND
* VU -> VCC
* D1 -> SCL
* D2 -> SDA


TODO: Hier noch Bilder hin

## Bekannte Probleme
* Immer mal wird eine falsche Stundenzahl für die aktuelle Uhrezeit abgerufen.

* im Allgemeinen ist der Code vermutlich unglaublich unelegant. Mein Wissen über C++ beschränkt sich auf "das ist im Prinzip so ähnlich wie Java (dassen Wissen sich wiederum auf 1 Jahr Schule beschränkt)" und Codeschnipseln aus dem Internet. Verbesserungsvorschläge willkommen.

## Lizenz
habe ich mich noch nicht mit beschäftigt. Einige der Libraries stehen unter GNU GPL. Bitte entsprechend beachten.
