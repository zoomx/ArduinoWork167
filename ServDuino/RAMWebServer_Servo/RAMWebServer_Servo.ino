#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h>  //Arduino's Servo Library

Servo servo;
long prev=0;  //saves last time (in millisec) the servo arm was moved down
long current=0;  //current time in millisec
unsigned char servoangle=0;  //current servo angle
#define COUNTTIME 2000 //defines time that the arm waits between tick-downs (in millisec)
#define MOV 12 //defines how much the arm moves up when a page is loaded

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192,168,1, 177 };

Server server(80);

void setup()
{
  servo.attach(9);  //sets servo to pin 9.  Choose your favorite PWM pin (except 10 or 11)
  servo.write(0);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop()
{
  Client client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("your html code will go here. You cannot have any double quotes in it, however, because that will end the client print function "like this");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    servoangle+=MOV;  //Moves servo amount specified by MOV when html file is opened
    delay(1);
    client.stop();
  }
  if(servoangle>180)  //keeps servo angle from going above maximum angle
    servoangle=180;
  servo.write(servoangle);  //sets new servo angle
  current=millis();
  if(current-prev>COUNTTIME)  //when more time than defined by COUNTTIME has gone by, the servo arm ticks down 1
  {
    prev=current;
    if(servoangle>0)  //keeps servoangle from rolling over to 255
      servoangle--;
  }
}