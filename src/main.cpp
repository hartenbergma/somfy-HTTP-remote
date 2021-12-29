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

#define EMITTER_GPIO 2
#define CC1101_FREQUENCY 433.42

#define REMOTE1 0x65dc00
#define REMOTE2_1 0x25b5d5
#define REMOTE2_2 0x25a5ef
#define REMOTE3 0xc6c78f
#define REMOTE4 0x59714b

#define UP Command::Up
#define DOWN Command::Down
#define MY Command::My
#define PROG Command::Prog

/////////////////// CHANGE THESE VALUES //////////////////////
const char* ssid = "home.ookonzept"; // Name of your network
const char* password = "1238185828867696"; // Password for your network
const char* mdns = "somfyremotes"; // mDNS name
const int speed = 4.4; // % per sec
//////////////////////////////////////////////////////////////

EEPROMRollingCodeStorage rollingCodeStorage1(0);
EEPROMRollingCodeStorage rollingCodeStorage2_1(2);
EEPROMRollingCodeStorage rollingCodeStorage2_2(4);
EEPROMRollingCodeStorage rollingCodeStorage3(6);
EEPROMRollingCodeStorage rollingCodeStorage4(8);
SomfyRemote somfyRemote1(EMITTER_GPIO, REMOTE1, &rollingCodeStorage1);  //Rollot Martin
SomfyRemote somfyRemote2_1(EMITTER_GPIO, REMOTE2_1, &rollingCodeStorage2_1);  //Rollot Tobi links
SomfyRemote somfyRemote2_2(EMITTER_GPIO, REMOTE2_2, &rollingCodeStorage2_2);  //Rollot Tobi rechts
SomfyRemote somfyRemote3(EMITTER_GPIO, REMOTE3, &rollingCodeStorage3);  //Markise 
SomfyRemote somfyRemote4(EMITTER_GPIO, REMOTE4, &rollingCodeStorage4);  //Markise Sonnenschutz
int pos1, pos2, pos3, pos4 = 100;
bool state1, state2, state3, state4 = false;


ESP8266WebServer server(80);

void serverSetup();

void setup() {
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
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(String(++i) + " ");
  }
  Serial.println();
  Serial.println("Connected successfully");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(mdns)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS address: " + String(mdns) + ".local");

  serverSetup();
  
  // Start the server
  server.begin();
}

void move(int currentpos, int desiredpos, SomfyRemote remote){
  int cushion = 0;
  if (desiredpos > 100){
    desiredpos = 100;
    cushion = 10;
  } else if(desiredpos < 0){
    desiredpos = 0;
    cushion = 10;
  }

  if(currentpos < desiredpos){
    sendCC1101Command(UP, remote);
    while(currentpos < desiredpos){
    delay(1/(speed/1000));
    currentpos++;
    };
  } else if(currentpos > desiredpos){
    sendCC1101Command(DOWN, remote);
    while(currentpos > desiredpos){
    delay(1/(speed/1000));
    currentpos -= 1;
    };
  }

  while(cushion > 0){
      delay(1/(speed/1000));
      cushion--;
    }
  sendCC1101Command(MY, remote);

}

void sendCC1101Command(Command command, SomfyRemote remote) {
	ELECHOUSE_cc1101.SetTx();
	remote.sendCommand(command);
	ELECHOUSE_cc1101.setSidle();
}


void loop() {
	server.handleClient();
  MDNS.update();
}

void serverSetup(){
  //-----------------remote1-------------------------
  server.on("/remote1/status", []() {
    size_t capacity = JSON_OBJECT_SIZE(2) + 51;
    DynamicJsonDocument doc(capacity);

    doc["currentState"] = state1;
    doc["currentBrightness"] = pos1;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/remote1/setBrightness", []() {
    int desiredpos = server.arg("value").toInt();
    move(pos1, desiredpos, somfyRemote1);
    server.send(200);
  });

  server.on("/remote1/setState", []() {
    if (server.arg("value") == "true") {
      sendCC1101Command(UP, somfyRemote1);
    } else {
      sendCC1101Command(DOWN, somfyRemote1);
    }
    server.send(200);
  });

  server.on("/remote1/prog", []() {
    sendCC1101Command(PROG, somfyRemote1);
  });

  //-----------------remote2-------------------------

  server.on("/remote2/status", []() {
    size_t capacity = JSON_OBJECT_SIZE(2) + 51;
    DynamicJsonDocument doc(capacity);

    doc["currentState"] = state2;
    doc["currentBrightness"] = pos2;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/remote2/setBrightness", []() {
    int desiredpos = server.arg("value").toInt();
    move(pos2, desiredpos, somfyRemote2_1);
    move(pos2, desiredpos, somfyRemote2_2);
    server.send(200);
  });

  server.on("/remote2/setState", []() {
    if (server.arg("value") == "true") {
      sendCC1101Command(UP, somfyRemote2_1);
      sendCC1101Command(UP, somfyRemote2_2);

    } else {
      sendCC1101Command(DOWN, somfyRemote2_1);
      sendCC1101Command(DOWN, somfyRemote2_2);
    }
    server.send(200);
  });

  server.on("/remote2_1/prog", []() {
    sendCC1101Command(PROG, somfyRemote2_1);
  });

  server.on("/remote2_2/prog", []() {
    sendCC1101Command(PROG, somfyRemote2_2);
  });

  //-----------------remote3-------------------------

  server.on("/remote3/status", []() {
    size_t capacity = JSON_OBJECT_SIZE(2) + 51;
    DynamicJsonDocument doc(capacity);

    doc["currentState"] = state3;
    doc["currentBrightness"] = pos3;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/remote3/setBrightness", []() {
    int desiredpos = server.arg("value").toInt();
    move(pos3, desiredpos, somfyRemote3);
    server.send(200);
  });

  server.on("/remote3/setState", []() {
    if (server.arg("value") == "true") {
      move(pos3, 110, somfyRemote3);
      //sendCC1101Command(UP, somfyRemote3);
    } else {
      move(pos3, -10, somfyRemote3);
      //sendCC1101Command(DOWN, somfyRemote3);
    }
    server.send(200);
  });

  server.on("/remote3/prog", []() {
    sendCC1101Command(PROG, somfyRemote3);
  });

  //-----------------remote4-------------------------

  server.on("/remote4/status", []() {
    size_t capacity = JSON_OBJECT_SIZE(2) + 51;
    DynamicJsonDocument doc(capacity);

    doc["currentState"] = state4;
    doc["currentBrightness"] = pos4;

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/remote4/setBrightness", []() {
    int desiredpos = server.arg("value").toInt();
    move(pos4, desiredpos, somfyRemote4);
    server.send(200);
  });

  server.on("/remote4/setState", []() {
    if (server.arg("value") == "true") {
      sendCC1101Command(UP, somfyRemote4);
    } else {
      sendCC1101Command(DOWN, somfyRemote4);
    }
    server.send(200);
  });

  server.on("/remote4/prog", []() {
    sendCC1101Command(PROG, somfyRemote4);
  });

  //-----------------reset----------
  server.on("/reset", []() {
    sendCC1101Command(UP, somfyRemote1);
    sendCC1101Command(UP, somfyRemote2_1);
    sendCC1101Command(UP, somfyRemote2_2);
    sendCC1101Command(UP, somfyRemote3);
    sendCC1101Command(UP, somfyRemote4);
  });

}


