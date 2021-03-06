#include <Arduino.h>
#include <EEPROM.h>
#include <EEPROMRollingCodeStorage.h>
#include <SomfyRemote.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define EMITTER_GPIO 2
#define CC1101_FREQUENCY 433.42

const char *ssid = "home.ookonzept";       // Name of your network
const char *password = "1238185828867696"; // Password for your network
const char *mdns = "esphub";               // mDNS name
ESP8266WebServer server(80);

#define REMOTE1 0x65dc00
#define REMOTE2_1 0x25b5d5
#define REMOTE2_2 0x25a5ef
#define REMOTE3 0xc6c78f
#define REMOTE4 0x59714b
#define UP Command::Up
#define DOWN Command::Down
#define MY Command::My
#define PROG Command::Prog
EEPROMRollingCodeStorage rollingCodeStorage1(0);
EEPROMRollingCodeStorage rollingCodeStorage2_1(2);
EEPROMRollingCodeStorage rollingCodeStorage2_2(4);
EEPROMRollingCodeStorage rollingCodeStorage3(6);
EEPROMRollingCodeStorage rollingCodeStorage4(8);
SomfyRemote somfyRemote1(EMITTER_GPIO, REMOTE1, &rollingCodeStorage1);       //Rollot Martin
SomfyRemote somfyRemote2_1(EMITTER_GPIO, REMOTE2_1, &rollingCodeStorage2_1); //Rollot Tobi links
SomfyRemote somfyRemote2_2(EMITTER_GPIO, REMOTE2_2, &rollingCodeStorage2_2); //Rollot Tobi rechts
SomfyRemote somfyRemote3(EMITTER_GPIO, REMOTE3, &rollingCodeStorage3);       //Markise
SomfyRemote somfyRemote4(EMITTER_GPIO, REMOTE4, &rollingCodeStorage4);       //Markise Sonnenschutz

void serverSetup();

void setup()
{
  Serial.begin(9600);

  // setup RF emitter
  pinMode(EMITTER_GPIO, OUTPUT);
  digitalWrite(EMITTER_GPIO, LOW);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(CC1101_FREQUENCY);

  // connect to wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(String(++i) + " ");
  }
  Serial.println();
  Serial.println("Connected successfully");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(mdns))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS address: " + String(mdns) + ".local");

  serverSetup();

  // Start the server
  server.begin();
}

void sendCC1101Command(Command command, SomfyRemote remote)
{
  ELECHOUSE_cc1101.SetTx();
  remote.sendCommand(command);
  ELECHOUSE_cc1101.setSidle();
}

void loop()
{
  server.handleClient();
  MDNS.update();
}

void serverSetup()
{
  // somfyRemote1
  server.on("/somfyremote1/up", []()
            {
              sendCC1101Command(UP, somfyRemote1);
              server.send(200);
            });
  server.on("/somfyremote1/down", []()
            {
              sendCC1101Command(DOWN, somfyRemote1);
              server.send(200);
            });
  server.on("/somfyremote1/stop", []()
            {
              sendCC1101Command(MY, somfyRemote1);
              server.send(200);
            });
  server.on("/somfyremote1/prog", []()
            { sendCC1101Command(PROG, somfyRemote1); });

  // somfyRemote2
  server.on("/somfyremote2/up", []()
            {
              sendCC1101Command(UP, somfyRemote2_1);
              sendCC1101Command(UP, somfyRemote2_2);
              server.send(200);
            });
  server.on("/somfyremote2/down", []()
            {
              sendCC1101Command(DOWN, somfyRemote2_1);
              sendCC1101Command(DOWN, somfyRemote2_2);
              server.send(200);
            });
  server.on("/somfyremote2/stop", []()
            {
              sendCC1101Command(MY, somfyRemote2_1);
              sendCC1101Command(MY, somfyRemote2_2);
              server.send(200);
            });
  server.on("/somfyremote2/prog1", []()
            { sendCC1101Command(PROG, somfyRemote2_1); });
  server.on("/somfyremote2/prog2", []()
            { sendCC1101Command(PROG, somfyRemote2_2); });

  // somfyRemote3
  server.on("/somfyremote3/up", []()
            {
              sendCC1101Command(UP, somfyRemote3);
              server.send(200);
            });
  server.on("/somfyremote3/down", []()
            {
              sendCC1101Command(DOWN, somfyRemote3);
              server.send(200);
            });
  server.on("/somfyremote3/stop", []()
            {
              sendCC1101Command(MY, somfyRemote3);
              server.send(200);
            });
  server.on("/somfyremote3/prog", []()
            { sendCC1101Command(PROG, somfyRemote3); });

  // somfyRemote4
  server.on("/somfyremote4/up", []()
            {
              sendCC1101Command(UP, somfyRemote4);
              server.send(200);
            });
  server.on("/somfyremote4/down", []()
            {
              sendCC1101Command(DOWN, somfyRemote4);
              server.send(200);
            });
  server.on("/somfyremote4/stop", []()
            {
              sendCC1101Command(MY, somfyRemote4);
              server.send(200);
            });
  server.on("/somfyremote4/prog", []()
            { sendCC1101Command(PROG, somfyRemote4); });

  // temperature and humididy
  server.on("/getclimate", []()
            {
              size_t capacity = JSON_OBJECT_SIZE(2) + 51;
              DynamicJsonDocument doc(capacity);

              float temp = dht.readTemperature();
              float humi = dht.readHumidity();
              if (isnan(temp))
                temp = 0;
              if (isnan(humi))
                humi = 0;

              doc["temperature"] = temp;
              doc["humidity"] = humi;

              String json;
              serializeJson(doc, json);
              server.send(200, "application/json", json);
            });
}
