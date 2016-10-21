//*****************************************************
#define WEBDUINO_AUTH_REALM "Authentication Required"

#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

static uint8_t mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 
  192, 168, 1, 111 };
#define PREFIX "/control"
WebServer webserver(PREFIX, 85);

// Set the number of relays you want to be active.
// Only active relays show on the web page
// Inactive = 1  Active = 0
int Relay1active = 0; 
int Relay2active = 0;  
int Relay3active = 0;   
int Relay4active = 0;   

// Set the power up state of each relay  
//OFF = 1  ON = 0
int Relay1state = 0; 
int Relay2state = 0;  
int Relay3state = 0;   
int Relay4state = 1;   

//Button Labels
String R1text = "Server Power";
String R2text = "Relay2";
String R3text = "PC Power";
String R4text = "Garage Door";

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{


   /* username = admin
   * password = admin
   * "YWRtaW46YWRtaW4=" is the Base64 representation of "admin:admin" */
    if (server.checkCredentials("YWRtaW46YWRtaW4="))
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
          digitalWrite(2, HIGH);    
        }
        else if (strcmp(value, "1") == 0)
        {
          digitalWrite(2, LOW);    
        }
      }

      if (strcmp(name, "Relay2") == 0)
      {
        if (strcmp(value, "0") == 0)
        {
          digitalWrite(3, HIGH);    
        }
        else if (strcmp(value, "1") == 0)
        {
          digitalWrite(3, LOW);    
        }
      }

      if (strcmp(name, "Relay3") == 0)
      {
        if (strcmp(value, "0") == 0)
        {
          digitalWrite(4, HIGH);    
        }
        else if (strcmp(value, "1") == 0)
        {
          digitalWrite(4, LOW);    
        }
      }

      if (strcmp(name, "Relay4") == 0)
      {
        if (strcmp(value, "0") == 0)
        {
          digitalWrite(5, HIGH);    
        }
        else if (strcmp(value, "1") == 0)
        {
          digitalWrite(5, LOW);    
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

      if (digitalRead(2) == 0){  
        server.print("<p><button name='Relay1' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
        server.print(R1text);
        server.print(" - is On</button></p>");      
      }
      else{  
        server.print("<p><button name='Relay1' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
        server.print(R1text);
        server.print("- is Off</button></p>");
      }
    }


    if (Relay2active == 0)
    {
      if (digitalRead(3) == 0){  
        server.print("<p><button name='Relay2' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
        server.print(R2text);
        server.print(" - is On</button></p>");      
      }
      else{  
        server.print("<p><button name='Relay2' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
        server.print(R2text);
        server.print("- is Off</button></p>");
      }    
    }
    if (Relay3active == 0)
    {
      if (digitalRead(4) == 0){  
        server.print("<p><button name='Relay3' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
        server.print(R3text);
        server.print(" - is On</button></p>");      
      }
      else{  
        server.print("<p><button name='Relay3' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
        server.print(R3text);
        server.print("- is Off</button></p>");
      }    
    }
    if (Relay4active == 0)
    {
      if (digitalRead(5) == 0){  
        server.print("<p><button name='Relay4' style='background-color:#088A08; color:#FFFFFF;' value='0'>");
        server.print(R4text);
        server.print(" - is On</button></p>");      
      }
      else{  
        server.print("<p><button name='Relay4' style='background-color:#CC0000; color:#FFFFFF;' value='1'>");
        server.print(R4text);
        server.print("- is Off</button></p>");
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

void logoutCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{

  server.httpUnauthorized(); 

  P(logoutMsg) = "<h1>You have been logged out!</h1> <p>Goodbye</p>";

  server.printP(logoutMsg);


}


void setup()
{




  if (Relay1active == 0){
    pinMode(2, OUTPUT); //Set pin as output 
    if (Relay1state == 0){
      digitalWrite(2, LOW);  
    }
    else{
      digitalWrite(2, HIGH);    
    }  
  }

  if (Relay2active == 0){  
    pinMode(3, OUTPUT); //Set pin as output
    if (Relay2state == 0){
      digitalWrite(3, LOW);  
    }
    else{
      digitalWrite(3, HIGH);    
    }
  }  

  if (Relay3active == 0){  
    pinMode(4, OUTPUT); //Set pin as output 
    if (Relay3state == 0){
      digitalWrite(4, LOW);  
    }
    else{
      digitalWrite(4, HIGH);    
    }  
  }

  if (Relay4active == 0){   
    pinMode(5, OUTPUT); //Set pin as output
    if (Relay4state == 0){
      digitalWrite(5, LOW);  
    }
    else{
      digitalWrite(5, HIGH);    
    }  
  }

  Ethernet.begin(mac, ip);
  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("index.html", &defaultCmd);
  webserver.addCommand("logout.html", &logoutCmd);
  webserver.begin();

}

void loop()
{

  webserver.processConnection();

}
//*****************************************************