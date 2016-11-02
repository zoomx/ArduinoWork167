/*****************************************************

  EthernetQuadRelayAuthentication

  Quad relay ethernet control with authentication

  was RudeBastardWebRelayControl
  http://www.rudebastard.com/article.php?story=20130310155336385
  "started this Arduino project with the objective to be able to login to an ethernet server on my home network and turn a computers power on and off. I have an old server that I seldom need and I don't want to leave running all of the time but when I do need it I want to be able to remotely power it up. I also run this website from a server on my network and it occasionally hangs and needs to be powered down and back up. This project, when complete will allow me to control up to four devices. I did my testing with a relay board but I will switch to solid state switches to do the actual power switching. Once I have completed the packaging I will post another article documenting what I did.
  Below is the Arduino/Webduino code. Webduino https://github.com/sirleech/Webduino
  I tested it in Chrome and it worked fine. When I tried it in IE9 I had an issue where IE set the Document Mode to quirks. When I set it to IE9 standards it worked fine.
  I was also able to test it on my Android phone using the web browser"

  Based on Web_Authentication.ino
  https://github.com/sirleech/Webduino/blob/master/examples/Web_Authentication/Web_Authentication.ino

  Added
  Serial debug but it eats memory!
  DHCP
  Shortened some text
  Pins are in define

  works with RelayShield by Catalex catalex.taobao.com V1.0 12/02/2013

  server is at port 80
  You have to connect to http://IPofArduino/control to get the control page
  Without /control you will get the EPIC FAIL page.
  This sketch uses DHCP so you need to set the router (the DHCP server) to
  assing the same IP to this Arduino.
  You can also assign the IP on the sketch but you have to code it.


  Connect to ArduinoIP:80/control
  admin:admin
  Change username and password as they are too much common
  Choose username and password, for example  dante and alighieri
  Go to https://www.base64encode.org/
  Insert dante:alighieri in the upper text box and leave UTF8
  Click on encode
  You will get un the bottom box a string like ZGFudGU6YWxpZ2hpZXJp (for dante:alighieri)
  Put int into CREDENTIALS define between the two "

  All serial instructions where added only for debug.
  They get a lot of memory so if you need memory
  remove them, for example to have longer strings.

  The initial relay state is defined in setup.
  It means that on reset the relays are resetted!
  If you need that they are not resetted you have to put
  their state on EEPROM and read again from there upon reset
******************************************************/
//*********DEFINE***************
#define WEBDUINO_AUTH_REALM "Authentication Required"
#define WEBPORT 80

// these are pins definition for the Catalex Quad Relays shield
#define RELAY1 7
#define RELAY2 6
#define RELAY3 5
#define RELAY4 4

#define CREDENTIALS "YWRtaW46YWRtaW4="      //this is admi:admin encoded in base64

//*********INCLUDE***************

#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"   //From Webduino https://github.com/sirleech/Webduino

//*******************************

static uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
static uint8_t ip[] = {192, 168, 1, 111};
// the router's gateway address:
static uint8_t  gateway[] = { 192, 168, 1, 1 };
// the subnet:
static uint8_t subnet[] = { 255, 255, 255, 0 };
#define PREFIX "/control"
WebServer webserver(PREFIX, WEBPORT);

//*******************************

/*
  Set the number of relays you want to be active.
  Only active relays show on the web page
  Inactive = 1  Active = 0
*/

int Relay1active = 0;
int Relay2active = 0;
int Relay3active = 0;
int Relay4active = 0;

byte Relay1pin = RELAY1;
byte Relay2pin = RELAY2;
byte Relay3pin = RELAY3;
byte Relay4pin = RELAY4;

// Set the power up state of each relay
//OFF = 0  ON = 1
int Relay1state = 0;
int Relay2state = 0;
int Relay3state = 0;
int Relay4state = 0;

/*
  Button Labels
  If you use longer labels you can get problems in the last one,
  it became unreadable due to memory problems.
  If this happens remove all serial instructions
*/

String R1text = "Relay1";
String R2text = "Relay2";
String R3text = "Relay3";
String R4text = "Relay4";

//********************************************************************
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  if (server.checkCredentials(CREDENTIALS))   //if username and password are ok!
  {


    //**************************
    // process post data here
    //**************************
    if (type == WebServer::POST)
    {
      bool repeat;
      char name[16], value[16];

      do
      {
        repeat = server.readPOSTparam(name, 16, value, 16);

        if (strcmp(name, "Relay1") == 0)
        {
          if (strcmp(value, "0") == 0)
          {
            digitalWrite(Relay1pin, HIGH);
          }
          else if (strcmp(value, "1") == 0)
          {
            digitalWrite(Relay1pin, LOW);
          }
        }

        if (strcmp(name, "Relay2") == 0)
        {
          if (strcmp(value, "0") == 0)
          {
            digitalWrite(Relay2pin, HIGH);
          }
          else if (strcmp(value, "1") == 0)
          {
            digitalWrite(Relay2pin, LOW);
          }
        }

        if (strcmp(name, "Relay3") == 0)
        {
          if (strcmp(value, "0") == 0)
          {
            digitalWrite(Relay3pin, HIGH);
          }
          else if (strcmp(value, "1") == 0)
          {
            digitalWrite(Relay3pin, LOW);
          }
        }

        if (strcmp(name, "Relay4") == 0)
        {
          if (strcmp(value, "0") == 0)
          {
            digitalWrite(Relay4pin, HIGH);
          }
          else if (strcmp(value, "1") == 0)
          {
            digitalWrite(Relay4pin, LOW);
          }
        }
      }

      while (repeat);

      server.httpSeeOther(PREFIX);
      return;
    }

    server.httpSuccess();

    if (type == WebServer::GET)
    {

      P(messagestart) =
        "<!DOCTYPE HTML><html><head><title>Power Control Page</title>"
        "<body>"
        "<h1>Power Control</h1>"
        "<form action='/control' method='POST'>";

      P(messageend) =
        "<p><a href='/control/logout.html'>LOGOUT</a></p>"
        "</form></body></html>";

      server.printP(messagestart);

      if (Relay1active == 0)
      {

        if (digitalRead(Relay1pin) == 0) {
          server.print("<p><button name='Relay1' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
          server.print(R1text);
          server.print(" On</button></p>");
        }
        else {
          server.print("<p><button name='Relay1' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
          server.print(R1text);
          server.print(" Off</button></p>");
        }
      }


      if (Relay2active == 0)
      {
        if (digitalRead(Relay2pin) == 0) {
          server.print("<p><button name='Relay2' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
          server.print(R2text);
          server.print(" On</button></p>");
        }
        else {
          server.print("<p><button name='Relay2' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
          server.print(R2text);
          server.print(" Off</button></p>");
        }
      }
      if (Relay3active == 0)
      {
        if (digitalRead(Relay3pin) == 0) {
          server.print("<p><button name='Relay3' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
          server.print(R3text);
          server.print(" On</button></p>");
        }
        else {
          server.print("<p><button name='Relay3' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
          server.print(R3text);
          server.print(" Off</button></p>");
        }
      }
      if (Relay4active == 0)
      {
        if (digitalRead(Relay4pin) == 0) {
          server.print("<p><button name='Relay4' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
          server.print(R4text);
          server.print(" On</button></p>");
        }
        else {
          server.print("<p><button name='Relay4' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
          server.print(R4text);
          server.print(" Off</button></p>");
        }
      }

      server.printP(messageend);
    }

  }
  else
  {

    server.httpUnauthorized();
  }
}

//********************************************************************
void logoutCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{

  server.httpUnauthorized();

  P(logoutMsg) = "<h1>You have been logged out!</h1> <p>Goodbye</p>";

  server.printP(logoutMsg);
}

//**************************SETUP*************************************
void setup()
{
  if (Relay1active == 0) {
    pinMode(Relay1pin, OUTPUT); //Set pin as output
    if (Relay1state == 0) {
      digitalWrite(Relay1pin, LOW);
    }
    else {
      digitalWrite(Relay1pin, HIGH);
    }
  }

  if (Relay2active == 0) {
    pinMode(Relay2pin, OUTPUT); //Set pin as output
    if (Relay2state == 0) {
      digitalWrite(Relay2pin, LOW);
    }
    else {
      digitalWrite(Relay2pin, HIGH);
    }
  }

  if (Relay3active == 0) {
    pinMode(Relay3pin, OUTPUT); //Set pin as output
    if (Relay3state == 0) {
      digitalWrite(Relay3pin, LOW);
    }
    else {
      digitalWrite(Relay3pin, HIGH);
    }
  }

  if (Relay4active == 0) {
    pinMode(Relay4pin, OUTPUT); //Set pin as output
    if (Relay4state == 0) {
      digitalWrite(Relay4pin, LOW);
    }
    else {
      digitalWrite(Relay4pin, HIGH);
    }
  }

  Serial.begin(115200);
  Serial.println(F("EthernetQuadRelayAuthentication"));
  Ethernet.begin(mac);
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());

  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("index.html", &defaultCmd);
  webserver.addCommand("logout.html", &logoutCmd);
  webserver.begin();
}

//**************************LOOP**************************************
void loop()
{

  webserver.processConnection();

}
//********************************************************************

