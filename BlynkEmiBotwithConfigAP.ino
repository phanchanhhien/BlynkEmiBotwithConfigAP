/**************************************************************
 * Blynk is a platform with iOS and Android apps to control
 * Arduino, Raspberry Pi and the likes over the Internet.
 * You can easily build graphic interfaces for all your
 * projects by simply dragging and dropping widgets.
 *
 *   Downloads, docs, tutorials: http://www.blynk.cc
 *   Blynk community:            http://community.blynk.cc
 *   Social networks:            http://www.fb.com/blynkapp
 *                               http://twitter.com/blynk_app
 *
 * Blynk library is licensed under MIT license
 * This example code is in public domain.
 *
 **************************************************************
 * You can receive x and y coords for joystick movement within App.
 *
 * App project setup:
 *   Two Axis Joystick on V1 in MERGE output mode.
 *   MERGE mode means device will receive both x and y within 1 message
 *
 **************************************************************/

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include "WiFiManager.h"
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include "Ultrasonic.h"

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).

#define PWMLeft  D1
#define PWMRight D2
#define DirLeft  D3
#define DirRight D4
#define Config_Pin D5

WiFiClient client;
Ultrasonic ultrasonic(D7,D8); // (Trig PIN,Echo PIN)

char blynk_token[34];

int robotspeed = 255;
int Lspeed;
int Rspeed;


void Stop()
{
analogWrite(PWMLeft, 0);
analogWrite(PWMRight, 0);
  
}


void  Forward(int _speed)
{
analogWrite(PWMLeft, _speed);
analogWrite(PWMRight, _speed);
digitalWrite(DirLeft,LOW);
digitalWrite(DirRight,LOW);
  
}
/////////
void Backward(int Car_speed)
{
analogWrite(PWMLeft, Car_speed);
analogWrite(PWMRight, Car_speed);
digitalWrite(DirLeft,HIGH);
digitalWrite(DirRight,HIGH);  
}
////////////////////////
void Turn_Left(int Car_speed)
{
analogWrite(PWMLeft, Car_speed);
analogWrite(PWMRight, Car_speed);
digitalWrite(DirLeft,LOW);
digitalWrite(DirRight,HIGH);    
}

/////////////////
void Turn_Right(int Car_speed)
{
analogWrite(PWMLeft, Car_speed);
analogWrite(PWMRight, Car_speed);
digitalWrite(DirLeft,HIGH);
digitalWrite(DirRight,LOW);  
}
////////////////
void Init_FSconfig()
{
  Serial.println();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

         // strcpy(mqtt_server, json["mqtt_server"]);
        //  strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

}
//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
//////////////// 
void setup()
{
  ////////////////////////////////////////
  ///Robot Setup//////////
  pinMode(Config_Pin, INPUT);

  pinMode (PWMLeft,OUTPUT);
  pinMode (PWMRight,OUTPUT);
  pinMode (DirLeft,OUTPUT);
  pinMode (DirRight,OUTPUT);
  digitalWrite(PWMLeft,0);
  digitalWrite(PWMRight,0);
  digitalWrite(DirLeft,0);
  digitalWrite(DirRight,0);
  
  analogWriteRange(255);
  analogWriteFreq(500);



  /////Connection init and setup
 WiFiManager wifiManager;
 Serial.begin(115200);
   
 
 Init_FSconfig(); //init config parameters (SSID,Password,Blynk Token) from SPIFFS
 WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 34);
//WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);  //set config save notify callback
  wifiManager.addParameter(&custom_blynk_token);
   //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  if (!digitalRead(Config_Pin))
   {
    Serial.println("Go direct into AP mode for Setting");
    if (!wifiManager.startConfigPortal("emiBot")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }

   
    
   
  wifiManager.setTimeout(120);
    if (!wifiManager.autoConnect("emiBot")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  
  //read updated parameters
  strcpy(blynk_token, custom_blynk_token.getValue());
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
 //    json["mqtt_server"] = mqtt_server;
 //   json["mqtt_port"] = mqtt_port;
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  
//  Blynk.begin(blynk_token);
  Blynk.config(blynk_token);
  if (!Blynk.connect(30)) {
    Serial.println("Blynk connection to server fail! Please press and hold Config Button and reboot to go into AP config"); 
  }
  else {
    Serial.println("Blynk ready to use!");
  }


}
////////////////////
BLYNK_WRITE(V1) {
  int x = param[0].asInt();
  int y = param[1].asInt();
  int xscale;
  int yscale;
  int DIRECTION;
  int LDIRECTION;
  int RDIRECTION;
  // Do something with x and y
  Serial.print("X = ");
  Serial.print(x);
  Serial.print("; Y = ");
  Serial.println(y);
      

    if (y >=220)   //  Robot direction = forward
   
      {
       // yscale = (y-128)* 2 + 1; 
       Forward(255);
            
        Serial.println("Go Forward");
       }
   else if (y <= 60)
      {
      // yscale = (128-y)* 2 + 1; 
       Backward(255);           
       Serial.println("Go Backward");
      }
    //robotspeed = 255;  
   else if (x >= 220) // go right
          {
          //xscale = (x -128)*2 + 1;
        Turn_Right(255);
          Serial.println("turn right");
           }
    else if(x<=60) // go left
           {
          // xscale = (128 - x)*2 + 1;
            Turn_Left(255);
       
            Serial.println("turn left");
           }   
   else if ((y<220)&&(y>60)&&(x<220)&&(x>60))
     {
          Stop();
      
     }
       
   }    
///////////////////////
BLYNK_READ(V0) // Widget in the app READs Virtal Pin V5 with the certain frequency
{
  // This command writes Arduino's uptime in seconds to Virtual Pin V5
  long range = ultrasonic.Ranging(CM);
  Blynk.virtualWrite(V0, range);
}  


void loop()
{
  Blynk.run();
  
}
