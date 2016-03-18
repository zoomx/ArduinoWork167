/*

  ArduinoVoiceAlarm4

  Wait for an alarm
  by zoomx

  based on
  arduino_with_ethernet_shield
  Created by Rui Santos
  Visit: http://randomnerdtutorials.com for more arduino projects
  http://www.instructables.com/id/Arduino-Webserver-Control-Lights-Relays-Servos-etc/?ALLSTEPS
  Arduino with Ethernet Shield

  Talkie by Peter Knight m odified by zoomx for 1.6.7 IDE (added some const!)
  Talkie::Talkie() {
  mute();
  pinMode(5, OUTPUT); // OC3A == PC6 == Arduino digital pin 5
*/

//#include <Arduino.h>
//#include "SD.h"

#include <SPI.h>
#include <Ethernet.h>
//#include <Servo.h>
#include "talkie.h"
#include "WAV_alarm.h"

#define LEDPIN 13
//#define SERVOPIN 7

Talkie  talkie;

//Servo microservo;
int pos = 0;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
byte ip[] = { 192, 168, 1, 178 };                      // ip in lan (that's what you need to use in your browser. (F("192.168.1.178"))
byte gateway[] = { 192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 255, 255, 255, 0 };                  //subnet mask
EthernetServer server(80);                             //server port
String readString;
boolean alarm = false;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println(F("ArduinoVoiceAlarm"));
  pinMode(LEDPIN, OUTPUT);
  //microservo.attach(SERVOPIN);
  // start the Ethernet connection and the server:
  if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  server.begin();
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());
}


void loop() {
  if (alarm == true) {
    talkie.say(WAV_alarm);
  }
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
        }

        //if HTTP request has ended
        if (c == '\n') {
          Serial.println(readString); //print to serial monitor for debuging

          client.println(F("HTTP/1.1 200 OK")); //send new page
          client.println(F("Content-Type: text/html"));
          client.println();
          client.println(F("<HTML>"));
          client.println(F("<HEAD>"));
          client.println(F("<meta name='apple-mobile-web-app-capable' content='yes' />"));
          client.println(F("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />"));
          client.println(F("<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />"));
          client.println(F("<TITLE>Random Nerd Tutorials Project</TITLE>"));
          client.println(F("</HEAD>"));
          client.println(F("<BODY>"));
          client.println(F("<H1>Random Nerd Tutorials Project</H1>"));
          client.println(F("<hr />"));
          client.println(F("<br />"));
          client.println(F("<H2>Arduino with Ethernet Shield</H2>"));
          client.println(F("<br />"));
          client.println(F("<a href=\"/?button1on\"\">Turn On LED</a>"));
          client.println(F("<a href=\"/?button1off\"\">Turn Off LED</a><br />"));
          client.println(F("<br />"));
          client.println(F("<br />"));
          client.println(F("<a href=\"/?button2on\"\">Rotate Left</a>"));
          client.println(F("<a href=\"/?button2off\"\">Rotate Right</a><br />"));
          client.println(F("<br />"));
          client.println(F("<br />"));
          client.println(F("<a href=\"/?alarmon\"\">Alarm ON</a>"));
          client.println(F("<a href=\"/?alarmoff\"\">Alarm OFF</a>"));
          client.println(F("<p>Created by Rui Santos. Visit http://randomnerdtutorials.com for more projects!</p>"));
          client.println(F("<br />"));
          client.println(F("</BODY>"));
          client.println(F("</HTML>"));

          delay(1);
          //stopping client
          client.stop();
          //controls the Arduino if you press the buttons
          if (readString.indexOf("?button1on") > 0) {
            digitalWrite(LEDPIN, HIGH);
          }
          if (readString.indexOf("?button1off") > 0) {
            digitalWrite(LEDPIN, LOW);
          }
          if (readString.indexOf("?alarmon") > 0) {
            Serial.println(F("Alarm ON!!"));
            //talkie.say(WAV_alarm);
            alarm = true;
          }
          if (readString.indexOf("?alarmoff") > 0) {
            Serial.println(F("Alarm OFF!!"));
            alarm = false;
          }
          if (readString.indexOf("?button2on") > 0) {
            for (pos = 0; pos < 180; pos += 3) // goes from 0 degrees to 180 degrees
            { // in steps of 1 degree
              //microservo.write(pos);              // tell servo to go to position in variable 'pos'
              delay(15);                       // waits 15ms for the servo to reach the position
            }
          }
          if (readString.indexOf("?button2off") > 0) {
            for (pos = 180; pos >= 1; pos -= 3) // goes from 180 degrees to 0 degrees
            {
              //microservo.write(pos);              // tell servo to go to position in variable 'pos'
              delay(15);                       // waits 15ms for the servo to reach the position
            }
          }
          //clearing string for next read
          readString = "";

        }
      }
    }
  }
}

