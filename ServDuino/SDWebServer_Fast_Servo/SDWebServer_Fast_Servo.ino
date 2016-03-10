#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Ethernet.h>
#include <Servo.h>  //Arduino's Servo Library

Servo servo;
long prev=0;  //saves last time (in millisec) the servo arm was moved down
long current=0;  //current time in millisec
unsigned char servoangle=0;  //current servo angle
#define COUNTTIME 2000 //defines time that the arm waits between tick-downs (in millisec)
#define MOV 12 //defines how much the arm moves up when a page is loaded

uint8_t bufindex;
const uint8_t maxbyte=255;
uint8_t buf[maxbyte];

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
  servo.attach(9);  //sets servo to pin 9.  Choose your favorite PWM pin (except 10 or 11)
  servo.write(0);
  Serial.begin(256000);
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
            servoangle+=MOV;  //Moves servo amount specified by MOV when html file is opened
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
