#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h> //Arduino's lcd library

uint32_t count=0; //Hit Count, as a long
uint32_t difcount=1;  //Differencial Count.  Makes lcd screen refresh only when there is a new hit.
LiquidCrystal lcd(2,3,5,6,7,8);  //Initializes the lcd screen, lcd.

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192,168,1, 177 };

Server server(8081);

void setup()
{
  lcd.begin(16,2); //starts the lcd.  This one is 16 characters by 2 lines.  Ajust accordingly.
  Ethernet.begin(mac, ip);
  server.begin();
  lcd.setCursor(1,0);  //Sets cursor to center text
  lcd.print("ServDuino v1.5");  //Displayed only if there is no error.  Stays on screen the entire time.
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
          client.println("your html code will go here. You cannot have any double quotes in it, however, because that will end the client print function like this");
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
    count++;  //increments the hit count
    delay(1);
    client.stop();
  }
  if(count!=difcount)  //only runs when count and difcount don't agree
  {
    difcount=count;  //sets difcount to count so function stops running
    lcd.setCursor(0,1); //new line
    lcd.print("Hits: ");  //displays Hits:
    lcd.print(count, DEC);  //Writes count to screen
  }
}