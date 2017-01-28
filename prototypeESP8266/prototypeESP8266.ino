#include <Arduino.h>

/*
  ESP8266 MQTT conection
  Use of the pubsub library
*/

/*
   Copyright 2016 Damian Nogueiras

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// wifi credentials
#include "WifiConfig.h"

// values conection wifi local
// defined in WifiConfig.h
/*
  const char* ssid = ".....";
  const char* password = ".....";
*/
// mqtt free broker for test
const char* mqtt_broker = "test.mosquitto.org";
// broker port
#define PORT 1883
// CLient Id
#define IDCLIENT protoAlfaESP8266
#define TOPIC_ROOT gamesp
#define TOPIC_STATE state
#define TOPIC_EXEC executing
#define TOPIC_COMM commandss

// the address for the motors at the bus i2c
#define MOTORS 1

const static byte step_patternFB[] = {
    B00010001, B00110011, B00100010, B01100110, B01000100, B11001100, B10001000, B10011001
};
const static byte step_patternL[] = {
    B00010000, B00110000, B00100000, B01100000, B01000000, B11000000, B10000000, B10010000
};
const static byte step_patternR[] = {
    B00000001, B00000011, B00000010, B00000110, B00000100, B00001100, B00001000, B00001001
};


WiFiClient gamespClient;
PubSubClient client(gamespClient);
long lastMsg = 0;
// number of movement
#define MOVEMENTS 10
// total of movements
char msg[MOVEMENTS];
int value = 0;

//actual position on the board
//init my position, right inf corner
int myPosition[2] = {9, 9};
// index of the cardinal point of my compass
int myCompass;
//cardinal points
char cardinal[4] = {'N', 'E', 'S', 'W'};
// msg to publish
String msgPub;
char charMsg[32];

// led movement
#define LED_UP D1
#define LED_RIGHT D2
#define LED_LEFT D3
#define LED_DOWN D4

void setup() {
  //debug
  Serial.begin(115200);
  //Initialize the rgb led pin as an output
  pinMode(LED_UP, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_DOWN, OUTPUT);
  // join the i2c bus like a master
  //Wire.begin();
  
  //wifi connectin
  setup_wifi();
  //mqtt client configuratin
  client.setServer(mqtt_broker, PORT);
  client.setCallback(callback);
}

//Config the wifi
void setup_wifi() {
  delay(50);
  // Connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //init compass see to North
  myCompass = 0;
}

//Arrived messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println();
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]:");

  // Necessary because de publisher buffer it's the same of subscribe
  // Allocate memory for the payload copy
  byte* copy_commands = (byte*)malloc(length);
  // Copy the payload to the new buffer
  memcpy(copy_commands, payload, length);
  for (int i = 0; i < length; i++) {
    Serial.print((char)copy_commands[i]);
    // action for different commands 
    switch ((char)copy_commands[i]) {
      case 'F':
        mov_up();
        break;
      case 'R':
        mov_right();
        break;
      case 'L':
        mov_left();
        break;
      case 'B':
        mov_down();
        break;
      // stop  
      case 'S':
        mov_stop();
        break;    
    }
  }
  //finish execute
  free(copy_commands);
  // stop movement
  mov_stop();
}

void mov_up() {
  // X position
  myPosition[0] = myPosition[0] + steepX(myCompass);
  // Y position
  myPosition[1] = myPosition[1] + steepY(myCompass);
  //Create the message to publish
  createMsg('F');
  client.publish("/gamesp/protoAlfaESP8266/executing", charMsg);
  // send movement forward to i2c
  i2c('F',2);
  analogWrite(LED_UP, 125);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 0);
  delay(1000);

}
void mov_down() {
  // X position
  myPosition[0] = myPosition[0] - steepX(myCompass);
  // Y position
  myPosition[1] = myPosition[1] - steepY(myCompass);
  createMsg('B');
  client.publish("/gamesp/protoAlfaESP8266/executing", charMsg);
  // send movement back to i2c
  i2c('B',2);
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 125);
  delay(1000);
}
void mov_right() {
  // change compass 90 degrees right
  if (myCompass == 3) {
    //West to North
    myCompass = 0;
  } else {
    myCompass = myCompass + 1;
  }

  createMsg('R');
  client.publish("/gamesp/protoAlfaESP8266/executing", charMsg);
  // send movement right to i2c
  i2c('R',2);
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 125);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 0);
  delay(1000);
}
void mov_left() {
  // change compass 90 degrees left
  if (myCompass == 0) {
    //West to North to West
    myCompass = 3;
  } else {
    myCompass = myCompass - 1;
  }
  createMsg('L');
  client.publish("/gamesp/protoAlfaESP8266/executing", charMsg);
  // send movement left to i2c
  i2c('L',2);
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 125);
  analogWrite(LED_DOWN, 0);
  delay(1000);
}

//movement X axis
int steepX(int myCompass) {
  switch (myCompass) {
    //North
    case 0:
      return -1;
    //East
    case 1:
      return 0;
    //South
    case 2:
      return +1;
    //West
    case 3:
      return 0;
    default:
      return 0;
  }
}

//movement Y axis
int steepY(int myCompass) {
  switch (myCompass) {
    //North
    case 0:
      return 0;
    //East
    case 1:
      return +1;
    //South
    case 2:
      return 0;
    //West
    case 3:
      return -1;
    default:
      return 0;
  }
}

void mov_stop() {
  createMsg('S');
  client.publish("/gamesp/protoAlfaESP8266/executing", charMsg);
  //Wire.send();
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 0);
}

// create a msg to publish
void createMsg(char myMov) {
  snprintf(charMsg, 32, "{Mov:%c,X:%d,Y:%d,Compass:%c}", myMov, myPosition[0], myPosition[1], cardinal[myCompass]);
}

// send to bus i2c de pattern
void i2c(char direction, int steeps){
  Serial.println();
  switch (direction) {
    case 'F':
      for (int i=0; i<steeps; i++) {
        for (int index=0; index<8; index++){
          Serial.println(step_patternFB[index],BIN);
          //Wire.send();
          delay(5);
        }
      };
      break;
    case 'B':
      for (int i=0; i<steeps; i++) {
        int index=7;
        do {
          Serial.println(step_patternFB[index],BIN);
          //Wire.send();
          delay(5);
          index--;
        } while (index>-1);
      };
      break;
      case 'L':
      for (int i=0; i<steeps; i++) {
        for (int index=0; index<8; index++){
          Serial.println(step_patternL[index],BIN);
          //Wire.send();
          delay(5);
        }
      };
      break;
      case 'R':   
      for (int i=0; i<steeps; i++) {
        for (int index=0; index<8; index++){
          Serial.println(step_patternR[index],BIN);
          //Wire.send();
          delay(5);
        }
      };
      break;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, with will message state:OFF case disconnect
    if (client.connect("protoAlfaESP8266", NULL, NULL, "/gamesp/protoAlfaESP8266/state", 2, false, "OFF")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      Serial.println("ON");
      client.publish("/gamesp/protoAlfaESP8266/state", "ON");
      // publish not executing anything
      createMsg('S');
      client.publish("/gamesp/protoAlfaESP8266/executing", msg);
      Serial.println(msg);
      // ... and resubscribe
      client.subscribe("/gamesp/protoAlfaESP8266/commands");
      // ... and resubscribe my state for debug
      // client.subscribe("/gamesp/protoAlfaESP8266/state");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // publish every 5sg ON
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "ON #%ld", value);
    client.publish("/gamesp/protoAlfaESP8266/state", msg);
  }
}
