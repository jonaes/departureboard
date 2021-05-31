//--------------------------------------------------
// https://github.com/jonaes/departureboard
// 
// Lizenzen der Libaries beachten!
//
//--------------------------------------------------

#include <Regexp.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <WiFiClientSecure.h>


LiquidCrystal_I2C disp(0x27, 20, 4);

File dbf;

// WiFi Parameters
const char* ssid = "******";
const char* password = "******";

// Zeitzone
const long utcOffsetInSeconds = 3600 * 2;


// API definieren
String station = "Frankfurt(Main)Hbf";  //Aktuell ohne Funktion, TODO URL aus dieser Var erzeugen
//const char* host = "https://marudor.de/api/iris/v2/abfahrten/8004596?lookahead=15";
const char* host = "https://dbf.finalrewind.org/Frankfurt(Main)Hbf.json?version=3&limit=8&admode=dep";
//const char* host = "https://jsonplaceholder.typicode.com/todos/1";



void setup() {
  
  disp.init();
  disp.clear();
  disp.backlight();
  disp.setCursor(0, 0);
  Serial.begin(115200);
  
  
  connectWiFi();

  SD.begin(15);
}

void loop() {
  
	displayLCD(dlParse());
	delay(60000);
}

DynamicJsonDocument dlParse() {
  String payload;

  SD.remove("dbf.json");
  dbf = SD.open("dbf.json", FILE_WRITE);

  // Check WiFi Status
  if (WiFi.status() == WL_CONNECTED) {
    if (dbf) {

      HTTPClient http;
      WiFiClientSecure client;
      client.setInsecure();
      client.connect(host, 80);

      Serial.println("host connected");
      http.begin(client, host);

      Serial.println("Lade url...");

      int httpCode = http.GET();
      Serial.println(httpCode);
  
      Serial.println("Erfolgreich geladen");
      //disp.setCursor(0, 3); disp.print("Geladen. Parse...");
      Serial.println("Schreibe in Textdatei...");
      dbf.print(http.getString());

      dbf.close();
      Serial.println("Abgeschlossen.");
      Serial.println();
      http.end();
    }
  }

  dbf = SD.open("dbf.json");
  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = 12288;
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError err = deserializeJson(doc, dbf);
  Serial.println(err.c_str());

  return doc;
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  disp.setCursor(0, 0); disp.print("Verbinde...");

  WiFiClientSecure client;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  //disp.print("connected");

  Serial.println("Initialisierung abgeschlossen");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  disp.setCursor(0, 1); disp.print(WiFi.localIP());
  disp.setCursor(0,2); disp.print("API von @derfnull; GNU AGPL 3");
  disp.setCursor(0,3); disp.print("dbf.finalrewind.org");
}

void displayLCD(DynamicJsonDocument doc) { //LCD-Anzeige aufbauen

  /*Serial.println(doc["departures"][0]["train"]["name"].as<char*>());
  Serial.println(doc["departures"][0]["train"].as<char*>());

  Serial.print("xx  ");
  Serial.print(doc["departures"][0]["destination"].as<char*>());
  Serial.print(" xx ");
  Serial.print( doc["departures"][0]["scheduledDeparture"].as<char*>());*/
  

  disp.clear();
  disp.backlight();
  int j = 0;
  int d = 0;
  while ( d < 4) {
    if (doc["departures"][j]["destination"].as<String>() == station) {
      j++;
      continue;
    }

    disp.setCursor(0, d);
    disp.print(shortenTrain(doc["departures"][j]["train"].as<char*>()));				//TODO Überstehende Zugnummern verarbeiten
    disp.setCursor(4, d);
    disp.print(shortenDest(doc["departures"][j]["destination"].as<char*>(), 13));
    disp.setCursor(17, d);
    if ( doc["departures"][j]["delayDeparture"].as<int>() > 3) {
      disp.print("*");
    }
    else {
      disp.print(" ");
    }
    if ( timeToMin(doc["departures"][j]["scheduledDeparture"].as<String>()) +  doc["departures"][j]["delayDeparture"].as<int>() - currentTimeInMin() > 9) {
      disp.print( timeToMin(doc["departures"][j]["scheduledDeparture"].as<String>()) +  doc["departures"][j]["delayDeparture"].as<int>() - currentTimeInMin());
    }
    else {
      disp.print(" ");
      disp.print( timeToMin(doc["departures"][j]["scheduledDeparture"].as<String>()) +  doc["departures"][j]["delayDeparture"].as<int>() - currentTimeInMin());
    }
    j++;
    d++;
  }
}

String shortenDest(String input, int mgl) { //Bahnhofsnamen kürzen
  char ipt[30];
  char res;

  if (station == input) { //Legacy (bevor ich daran gedacht habe, dass API-Parameter existieren
    input = "Zug endet";
    return input;
  }
 
  //Umlaute für LCD umwandeln
  input.replace("ü", "\xF5"); input.replace("ä", "\xE1"); input.replace("ß", "\xE2"); input.replace("ö", "\xEF");

  input.toCharArray(ipt, 30);
  MatchState ms (ipt);


  ms.GlobalReplace( "%(.*%)", " ");             //Klammerzusätze entfernen
 // ms.GlobalReplace("Bad%s", "B.");              //"Bad" zu "B" kürzen
  ms.GlobalReplace("bahnhof", "bf");
  ms.GlobalReplace("Bahnhof", "Bf");

  input = ipt;
  if (input.length() > mgl) {		//Länger als mögliche Anzeige?
    int cut = input.length() - mgl;
    Serial.println(cut);
    Serial.println(input.length());
    res = ms.Match(".+%s.+");		//enthält mehrere Wörter?
    if (res) {						//vom ersten Wort die überstehenden Zeichen wegnehmen (regexp.h kann keine {}-Quantifier
      String reg = "";
      for (int r = 0; r < cut; r++) {
        reg += "[a-z]";
      }
      reg += "%s";
      Serial.println(reg.c_str());
      ms.GlobalReplace(reg.c_str(), " "); //          //Erstes Wort kürzen
 }
     res = ms.Match(".+%-.+");		//enthält mehrere Wörter mit Bindestrich?
    if (res) {						//vom ersten Wort die überstehenden Zeichen wegnehmen (regexp.h kann keine {}-Quantifier
      String reg = "";
      for (int r = 0; r < cut; r++) {
        reg += "[a-z]";
      }
      reg += "%-";
      Serial.println(reg.c_str());
      ms.GlobalReplace(reg.c_str(), "-"); //          //Erstes Wort kürzen
 }
    input = ipt;
    input = input.substring(0, mgl);
    return input;
  }
}

String shortenTrain(String input) {
  if (input.indexOf("ICE") >= 0) {
    return "ICE";
  } else if (input.indexOf("IC") >= 0) {
    return "IC";
  }
  input.replace(" ", "");
  input = input.substring(0,5);
  return input;
}

int timeToMin(String timeStr) {
  int hr = timeStr.substring(0, 2).toInt();
  int hrMins = timeStr.substring(3).toInt();
  int mins = (hr * 60) + hrMins;
  return mins;
}

int currentTimeInMin() {
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
  timeClient.begin();
  timeClient.update();
  int mins = (timeClient.getHours() * 60) + timeClient.getMinutes();
  Serial.println("akt zeit:");
  Serial.println(mins);
  return mins;
}
