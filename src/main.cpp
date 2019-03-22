#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Servo.h>

#define API_DEBUG true
#define MAX_CLIENTS 5
#define HOTSPOT false
#define HTTP_RESPONSE_OK "OK"
#define HTTP_RESPONSE_BAD "BAD"

AsyncWebServer server(80);
DHT_Unified dht(D7, DHT11);
Servo door;

const char* ssid = "Hit2Hat";
const char* pass = "09876789000";

byte registeredClients = 0;

struct client {
  String name;
  String ip;
} clients[MAX_CLIENTS];

struct {
  struct {
    byte x = 0;
    byte y = 0;
  } coords;

  byte fuel = 0;
  byte countCharges = 0;
  byte countMissions = 0;
  float temperature = 0;
  float humidity = 0;
} metrics;

client* findClientByName(String _name) {
  for(byte i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].name == _name)
      return &clients[i];
  }
  return nullptr;
}

bool registerClient(client target) {
  if (registeredClients + 1 > MAX_CLIENTS) return false;
  registeredClients++;
  clients[registeredClients] = target;

  if (API_DEBUG) {
    Serial.println("[registerClient]: new client " + target.ip + " was registered as " + target.name);
  }

  return true;
}

void setup() {
  Serial.begin(9600);
  if (HOTSPOT) {
    WiFi.softAP(ssid, pass);
  } else {
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) { delay(100); }
  }
  
  Serial.println(WiFi.localIP());
  SPIFFS.begin();
  dht.begin();
  door.attach(D5);
  door.write(20);

  /*
    Get all metrics
    return:
      @data - metrics in json
  */
  server.on("/api/getMetrics", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument result(200);
    result["temperature"] = (int) metrics.temperature;
    result["humidity"] = (int) metrics.humidity;
    result["fuel"] = metrics.fuel;
    result["coords_x"] = metrics.coords.x;
    result["coords_y"] = metrics.coords.y;
    result["count_charges"] = metrics.countCharges;
    result["count_missions"] = metrics.countMissions;

    String result_string;
    serializeJson(result, result_string);

    request->send(200, "application/json", result_string);
  });

  /*
    Auth on server
    params:
      @name - name of connecting device
    return:
      @status - OK / BAD
  */
  server.on("/api/auth", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (findClientByName(request->getParam("name")->value()) != nullptr) {
      request->send(200, "text/plain", HTTP_RESPONSE_BAD);
    } else {
      client newClient;
      newClient.ip = request->client()->remoteIP().toString();
      newClient.name = request->getParam("name")->value();

      if(!registerClient(newClient)) {
        request->send(200, "text/plain", HTTP_RESPONSE_BAD);
      } else {
        request->send(200, "text/plain", HTTP_RESPONSE_OK);
      }
    }
  });

  /*
    Set fuel from machine
    params:
      @fuel - count of fuel (in percentage)
    return:
      @status - OK / BAD
  */
  server.on("/api/setFuel", HTTP_GET, [](AsyncWebServerRequest *request){
    String fuel_val = request->getParam("fuel")->value();

    if (fuel_val.toInt() < 0 || fuel_val.toInt() > 100) {
       request->send(200, "text/plain", HTTP_RESPONSE_BAD);
    } else {
      metrics.fuel = fuel_val.toInt();
      request->send(200, "text/plain", HTTP_RESPONSE_OK);
    }

    if (API_DEBUG) {
      Serial.println("[/api/setFuel]: new value is " + fuel_val);
    }
  });

  /*
    Set coords of machine
    params:
      @x - x coord of machine
      @y - y coord of machine
    return:
      @status - OK / BAD
  */
  server.on("/api/setCoords", HTTP_GET, [](AsyncWebServerRequest *request){
    metrics.coords.x = request->getParam("x")->value().toInt();
    metrics.coords.y = request->getParam("y")->value().toInt();

    request->send(200, "text/plain", HTTP_RESPONSE_OK);

    if (API_DEBUG) {
      Serial.println("[/api/setCoords]: new value is (" + String(metrics.coords.x) + ":" + String(metrics.coords.y) + ")");
    }
  });

  server.on("/api/toggleDoor", HTTP_GET, [](AsyncWebServerRequest *request) {
    int val = door.read();
    if(val == 20) {
      door.write(180);
    } else {
      door.write(20);
    }
    
    request->send(200, "text/plain", String(val));
  });

  server.on("/api/actions/onFuelEnd", HTTP_GET, [](AsyncWebServerRequest *request){
    //TODO: return combine to last coords
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.begin();
}

void loop() {
  delay(1000);
  sensors_event_t event;

  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    metrics.temperature = event.temperature;
  }

  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    metrics.humidity = event.relative_humidity;
  }
}