#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Ethernet.h>
#include <LiquidCrystal.h> //Arduino's lcd library

uint8_t bufindex;
const uint8_t maxbyte=255;
uint8_t buf[maxbyte];

uint32_t count=0; //Hit Count, as a long
uint32_t difcount=1;  //Differencial Count.  Makes lcd screen refresh only when there is a new hit.
LiquidCrystal lcd(2,3,5,6,7,8);  //Initializes the lcd screen, lcd.

byte mac[] = {
  0x90,0xA2,0xDA,0x00,0x26,0xEB};
byte ip[] = {
  192,168,1,177};
char rootFileName[] = "index.htm";
Server server(8081);

Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

#define error(s) error_P(PSTR(s))
void error_P(const char* str) {
  lcd.print("SD Error");  //If there is a card error, the lcd displays: SD Error
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  while(1);
}

void setup() {
  Serial.begin(256000);
  lcd.begin(16,2); //starts the lcd.  This one is 16 characters by 2 lines.  Ajust accordingly.
  PgmPrint("Free RAM: ");
  Serial.println(FreeRam());
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  if (!card.init(SPI_FULL_SPEED, 4)) error("card.init failed!");
  if (!volume.init(&card)) error("vol.init failed!");
  PgmPrint("Volume is FAT");
  Serial.println(volume.fatType(),DEC);
  Serial.println();
  if (!root.openRoot(&volume)) error("openRoot failed");
  PgmPrintln("Files found in root:");
  root.ls(LS_DATE | LS_SIZE);
  Serial.println();
  PgmPrintln("Files found in all dirs:");
  root.ls(LS_R);
  Serial.println();
  PgmPrintln("Done");
  Ethernet.begin(mac, ip);
  server.begin();
  lcd.setCursor(1,0);  //Sets cursor to center text
  lcd.print("ServDuino v1.5");  //Displayed only if there is no error.  Stays on screen the entire time.
}
#define BUFSIZ 100

void loop()
{
  char clientline[BUFSIZ];
  char *filename;
  int index = 0;
  int image = 0;
  Client client = server.available();
  if (client) {
    boolean current_line_is_blank = true;
    index = 0;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          if (index >= BUFSIZ)
            index = BUFSIZ -1;
          continue;
        }
        clientline[index] = 0;
        filename = 0;
        Serial.println(clientline);
        if (strstr(clientline, "GET / ") != 0) {
          filename = rootFileName;
        }
        if (strstr(clientline, "GET /") != 0) {
          if (!filename) filename = clientline + 5;
          (strstr(clientline, " HTTP"))[0] = 0;
          Serial.println(filename);
          if (! file.open(&root, filename, O_READ)) {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<h2>File Not Found!</h2>");
            break;
          }
          Serial.println("Opened!");
          client.println("HTTP/1.1 200 OK");
          if (strstr(filename, ".htm") != 0){
            client.println("Content-Type: text/html");
            count++;  //the hit count only increments when an html document loads
          }
          else if (strstr(filename, ".css") != 0)
            client.println("Content-Type: text/css");
          else if (strstr(filename, ".png") != 0)
            client.println("Content-Type: image/png");
          else if (strstr(filename, ".jpg") != 0)
            client.println("Content-Type: image/jpeg");
          else if (strstr(filename, ".gif") != 0)
            client.println("Content-Type: image/gif");
          else if (strstr(filename, ".3gp") != 0)
            client.println("Content-Type: video/mpeg");
          else if (strstr(filename, ".pdf") != 0)
            client.println("Content-Type: application/pdf");
          else if (strstr(filename, ".js") != 0)
            client.println("Content-Type: application/x-javascript");
          else if (strstr(filename, ".xml") != 0)
            client.println("Content-Type: application/xml");
          else
            client.println("Content-Type: text");
          client.println();
          int16_t c;
          bufindex=0;
          while ((c = file.read()) >= 0) {
            buf[bufindex++]=((char)c);
            if(bufindex==maxbyte)
            {
              client.write(buf, maxbyte);
              bufindex=0;
            }
          }
          file.close();
          if(bufindex>0)
          {
            client.write(buf, bufindex);
          }
          bufindex=0;
        } 
        else {
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>File Not Found!</h2>");
        }
        break;
      }
    }
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