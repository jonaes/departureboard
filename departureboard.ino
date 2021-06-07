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
#include <ESP8266WebServer.h>

WiFiServer server(80);

LiquidCrystal_I2C disp(0x27, 20, 4);

File dbf;
File stationF;

int loopCount = 0;
String request;

// WiFi Parameters
const char* ssid = "";
const char* password = "";

// Zeitzone
const long utcOffsetInSeconds = 3600 * 2;


// API definieren
char* station;
//char* station = "Friedrichsdorf(Taunus)";
//const char* host = "https://marudor.de/api/iris/v2/abfahrten/8004596?lookahead=15";
char* host = "https://dbf.finalrewind.org/Frankfurt(Main)Hbf.json?version=3&limit=8&admode=dep";
//const char* host = "https://jsonplaceholder.typicode.com/todos/1";



void setup() {

  disp.init();
  disp.clear();
  disp.backlight();
  disp.setCursor(0, 0);
  Serial.begin(115200);


  connectWiFi();

  // server.on("/", webRequestRoot);

  SD.begin(15);
  server.begin();

  station = "";
  stationF = SD.open("station.txt");
  while (stationF.available()) {
    station += (char)stationF.read();
  }
  stationF.close();
}

void loop() {
  /* if (loopCount > 30) {
     ESP.restart();
    }
  */
  Serial.println("");
  loopCount++; Serial.print("Loop-Count: "); Serial.println(loopCount);
  Serial.println(station);
  displayLCD(dlParse());
  // delay(60000);
  Serial.println("Darstellung abgeschlossen. Warte auf Webanfrage...");
  waitForWebRequest();
}

DynamicJsonDocument dlParse() {
  String payload;
  char dbfHost[110];
  char* dbfHost1 = "https://dbf.finalrewind.org/";
  char* dbfHost2 = ".json?version=3&limit=8&admode=dep";
  strcpy(dbfHost, dbfHost1);
  strcat(dbfHost, station);
  strcat(dbfHost, dbfHost2);
  SD.remove("dbf.json");
  dbf = SD.open("dbf.json", FILE_WRITE);

  // Check WiFi Status
  if (WiFi.status() == WL_CONNECTED) {
    if (dbf) {

      HTTPClient http;
      WiFiClientSecure client;
      client.setInsecure();
      client.connect(dbfHost, 80);

      Serial.println("host connected");
      http.begin(client, dbfHost);

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
  disp.setCursor(0, 0); disp.print(WiFi.localIP());
  disp.setCursor(0, 1); disp.print("API von @derfnull; GNU AGPL 3");
  disp.setCursor(0, 2); disp.print("GNU AGPL 3");
  disp.setCursor(0, 3); disp.print("dbf.finalrewind.org");
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
  int time = currentTimeInMin();
  while ( d < 4) {
    if (doc["departures"][j]["destination"].as<String>() == station) {
      j++;
      continue;
    }

    disp.setCursor(0, d);
    disp.print(shortenTrain(doc["departures"][j]["train"].as<char*>()));        //TODO Überstehende Zugnummern verarbeiten
    disp.setCursor(4, d);
    disp.print(shortenDest(doc["departures"][j]["destination"].as<char*>(), 13));
    disp.setCursor(17, d);
    if ( doc["departures"][j]["delayDeparture"].as<int>() > 3) {
      disp.print("*");
    }
    else {
      disp.print(" ");
    }
    if ( timeToMin(doc["departures"][j]["scheduledDeparture"].as<String>()) +  doc["departures"][j]["delayDeparture"].as<int>() - time > 9) {
      disp.print( timeToMin(doc["departures"][j]["scheduledDeparture"].as<String>()) +  doc["departures"][j]["delayDeparture"].as<int>() - time);
    }
    else {
      disp.print(" ");
      disp.print( timeToMin(doc["departures"][j]["scheduledDeparture"].as<String>()) +  doc["departures"][j]["delayDeparture"].as<int>() - time);
    }
    j++;
    d++;
  }
}

String shortenDest(String input, int mgl) { //Bahnhofsnamen kürzen
  char ipt[30];
  char res;

  /* if (String(station) == input) { //Legacy (bevor ich daran gedacht habe, dass API-Parameter existieren
      input = "Zug endet";
      return input;
    }
  */

  //Umlaute für LCD umwandeln
  input.replace("ü", "\xF5"); input.replace("ä", "\xE1"); input.replace("ß", "\xE2"); input.replace("ö", "\xEF");

  input.toCharArray(ipt, 30);
  MatchState ms (ipt);


  ms.GlobalReplace( "%(.*%)", " ");             //Klammerzusätze entfernen
  // ms.GlobalReplace("Bad%s", "B.");              //"Bad" zu "B" kürzen
  ms.GlobalReplace("bahnhof", "bf");
  ms.GlobalReplace("Bahnhof", "Bf");

  input = ipt;
  if (input.length() > mgl) {   //Länger als mögliche Anzeige?
    int cut = input.length() - mgl;
    Serial.println(cut);
    Serial.println(input.length());
    res = ms.Match(".+%s.+");   //enthält mehrere Wörter?
    if (res) {            //vom ersten Wort die überstehenden Zeichen wegnehmen (regexp.h kann keine {}-Quantifier
      String reg = "";
      for (int r = 0; r < cut; r++) {
        reg += "[a-z]";
      }
      reg += "%s";
      Serial.println(reg.c_str());
      ms.GlobalReplace(reg.c_str(), " "); //          //Erstes Wort kürzen
    }
    res = ms.Match(".+%-.+");    //enthält mehrere Wörter mit Bindestrich?
    if (res) {            //vom ersten Wort die überstehenden Zeichen wegnehmen (regexp.h kann keine {}-Quantifier
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
  input = input.substring(0, 5);
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
  Serial.println("akt zeit:"); Serial.println(timeClient.getHours());
  Serial.println(mins);
  return mins;
}

void waitForWebRequest() {
  WiFiClient client ;
  int s = 0;
  for (s; s < 55; s++) {
    client = server.available();
    while (!client) {
      if (s > 55) {
        return;
      }
      s++;
      delay(1000);
      Serial.print("delayed1000 . "); Serial.print(s);
      client = server.available();
    }
    // Wait until the client sends some data
    Serial.println("new client");
    while (!client.available()) {
      delay(1);
      Serial.print(".");
    }
    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();
    boolean newReq =  handleRequest(request);
    webpage(client);//Return webpage
    //  boolean newReq = server.handleClient();
    if (newReq == 1) {
      Serial.println("got request, restarting loop");
      return;
    }
    s++;
    delay(2000);
    Serial.print(s); Serial.println("..delayed 2000");
  }
  return;
}


void webpage(WiFiClient client) { /* function webpage */
  ////Send webpage to client
  Serial.println("sending page");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("</head>");
  client.println("</body>");
  client.println("<h1>Abfahrtstafel</h1>");
  client.println("<form id=&quot;zza&quot;> <label class=&quot;h2&quot; form=&quot;zza&quot;>&Auml;nderung der Abfahrtstafelparameter</label><br> <label for=&quot;bahnhof&quot;>Bahnhof (vollst&auml;ndiger Name (bspw. &quot;Frankfurt(Main)Hbf&quot;) oder DS-100)</label> <input name=bahnhof id=&quot;bahnhof&quot; maxlength=&quot;30&quot;> <button name=mode value=abfahrtstafel>Abfahrtstafel</button> </form><br><br><br>Aktueller Bahnhof: ");
  client.println(station);
  client.println("</body></html>");
  client.println();
  delay(1);
  Serial.println("sent page");
}

bool handleRequest(String request) { /* function handleRequest */
  ////Handle web client request
  String bhf;
  bool ret = 0;
  if (request.indexOf("?") > 0) {
    if (request.indexOf("bahnhof=&") > 0)  { //bahnhof leer
    } else {
      bhf = request.substring(request.indexOf("bahnhof=") + 8, request.indexOf("&mode"));
      bhf.toCharArray(station, bhf.length() + 1);
      disp.setCursor(0, 3); disp.print(station); disp.print("  ");
      ret = 1;
      SD.remove("station.txt");
      stationF = SD.open("station.txt", FILE_WRITE);
      stationF.print(station);
      stationF.close();
    }

  }
  return ret;
}