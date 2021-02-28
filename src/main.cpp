#include <Arduino.h>

#include <Vault.h>

#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

#define USER_MQTT_CLIENT_NAME     "lux1"  

WiFiClient espClient;
PubSubClient mqtt(espClient);
SimpleTimer timer;

const byte TEMT_PIN = 0;
const byte BUFFER_SIZE = 50;

bool boot = true;
char messageBuffer[BUFFER_SIZE];

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME; 

float runningSum2 = 0;
word runningCount2 = 0; 


void wifi_setup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_connect() {
  int retries = 0;
  while (!mqtt.connected()) {
    if(retries < 150) {
      Serial.print("Attempting MQTT connection...");
      if (mqtt.connect(mqtt_client_name, mqtt_user, mqtt_pass)) {
        Serial.println("connected");
        mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn", boot ? "Rebooted" : "Reconnected"); 
      } 
      else {
        Serial.print("failed, rc=");
        Serial.print(mqtt.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        delay(5000);
      }
    }
    
    if(retries > 149) {
      ESP.restart();
    }
  }
}

// TEMT6000
//
// void measureLight1() {
//   runningSum1 += analogRead( TEMT_PIN );
//   runningCount1 += 1;
// }
//

// BH1750

void measureLight2() {
  runningSum2 += lightMeter.readLightLevel();
  runningCount2 += 1;
}

void reportLight2() {
    word avg = runningSum2 / runningCount2;
    Serial.println(avg); 
    
    String temp_str = String( avg );
    temp_str.toCharArray(messageBuffer, BUFFER_SIZE);
    mqtt.publish(USER_MQTT_CLIENT_NAME"/lux", messageBuffer); 

    runningSum2 = 0;
    runningCount2 = 0;
}

void checkIn() {
  mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn","OK"); 
}

//Run once setup
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  wifi_setup();
  
  mqtt.setServer(mqtt_server, mqtt_port);
  
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  delay(10);

  Wire.begin();
  lightMeter.begin();
  
  timer.setInterval(200, measureLight2);   
  timer.setInterval(10000, reportLight2);   
  
  timer.setInterval(60000, checkIn);
}

void loop() {
  if (!mqtt.connected()) {
    mqtt_connect();
  }
  mqtt.loop();
  
  ArduinoOTA.handle();

  timer.run();
}
