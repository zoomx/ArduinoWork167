/*
   RAMWebServer_Control
    by Teslaling
    http://www.instructables.com/id/ServDuino-Arduino-Webserver/?ALLSTEPS
    2011

   Modifications by Zoomx 2016
   Added DHCP and debug on serial
   Updated to the new Ethernet library
   Changed LED pin to 13
   Compile on IDE 1.6.7

*/

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
//byte ip[] = { 192,168,1, 177 };
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(80);

#define LED 13

void setup() {
  Serial.begin(9600);
  // this check is only needed on the Leonardo:
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("RAMWebServer_Control");


  // start the Ethernet connection:
  Serial.println("Trying to get an IP address using DHCP");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // initialize the Ethernet device not using DHCP:
    Ethernet.begin(mac, ip, myDns, gateway, subnet);
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  ip = Ethernet.localIP();
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(ip[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();

  server.begin();
  pinMode(LED, OUTPUT);
}
#define BUFSIZ 100  //Buffer size for getting data
char clientline[BUFSIZ];  //string that will contain command data
int index = 0;  //clientline index
void loop()
{
  index = 0; //reset the clientline index
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (index < BUFSIZ) //Only add data if the buffer isn't full.
        {
          clientline[index] = c;
          index++;
        }
        if (c == '\n' && currentLineIsBlank)
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h1><center>LED Control</h1></center><br><center><form method=get action=/?><input type=radio name=L1 value=1>On<br><input type=radio name=L1 value=0>Off<br><input type=submit value=submit></form></center>");
          Serial.println("Print page");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
        if (strstr(clientline, "/?L1=1") != 0) { //look for the command to turn the led on
          digitalWrite(LED, HIGH);  //turn the led on
          Serial.println("digitalWrite LED HIGH");
        } else if (strstr(clientline, "/?L1=0") != 0) { //look for command to turn led off. Note: If led is on, it will stay on until command is given to turn it off, even if multiple computers are on the site
          digitalWrite(LED, LOW);  //turn led off
          Serial.println("digitalWrite LED LOW");
        }
      }
    }
    delay(1);
    client.stop();
  }
}
