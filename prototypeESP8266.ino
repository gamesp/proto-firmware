/*
ESP8266 MQTT conection
Use of the pubsub library
*/

/*
 * Copyright 2016 Damian Nogueiras
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
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
#define TOPIC_COMM commands

WiFiClient gamespClient;
PubSubClient client(gamespClient);
long lastMsg = 0;
// number of movement
#define MOVEMENTS 10
// total of movements
char msg[MOVEMENTS];
int value=0;

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
  memcpy(copy_commands,payload,length);
  for (int i = 0; i < length; i++) {
      Serial.print((char)copy_commands[i]);
      if ((char)copy_commands[i]=='F') {      
        mov_up();     
      } else if ((char)copy_commands[i]=='R') {
        mov_right();
      } else if ((char)copy_commands[i]=='L') {
        mov_left();
      } else if ((char)copy_commands[i]=='B') {
        mov_down();
      }
  }
  //finish execute
  free(copy_commands);
  client.publish("/gamesp/protoAlfaESP8266/executing", "SLEEP");  
}

void mov_up(){
  client.publish("/gamesp/protoAlfaESP8266/executing", "Forward");
  analogWrite(LED_UP, 125);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 0);
  delay(1000);
  
}
void mov_down(){
  client.publish("/gamesp/protoAlfaESP8266/executing", "Back");
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 125);
  delay(1000);
  //
}
void mov_right(){
  client.publish("/gamesp/protoAlfaESP8266/executing", "Right");
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 125);
  analogWrite(LED_LEFT, 0);
  analogWrite(LED_DOWN, 0);
  delay(1000);
  //
  
}
void mov_left(){
  client.publish("/gamesp/protoAlfaESP8266/executing", "Left");
  analogWrite(LED_UP, 0);
  analogWrite(LED_RIGHT, 0);
  analogWrite(LED_LEFT, 125);
  analogWrite(LED_DOWN, 0);
  delay(1000);
}
  
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect, with will message state:OFF case disconnect
    if (client.connect("protoAlfaESP8266",NULL,NULL,"/gamesp/protoAlfaESP8266/state",2,false,"OFF")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      Serial.println("ON");
      client.publish("/gamesp/protoAlfaESP8266/state", "ON");
      // publish not executing anything
      client.publish("/gamesp/protoAlfaESP8266/executing", "SLEEP");
      Serial.println("SLEEP");
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
  // publish every 2sg ON
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "ON #%ld", value);
    client.publish("/gamesp/protoAlfaESP8266/state", msg);
  }
}
