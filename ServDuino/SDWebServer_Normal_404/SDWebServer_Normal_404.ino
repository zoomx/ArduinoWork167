#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Ethernet.h>

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
            file.open(&root,"404error.htm",O_READ);  //Opens the file 404error.htm from the root directory.  This is if the file doesn't exist
            int16_t c;
            while((c=file.read())>=0){  //sends file (yes it is the slow way, but if you don't have a picture, thats ok
              client.write((char)c);
            }
            file.close();
            break;
          }
          Serial.println("Opened!");
          client.println("HTTP/1.1 200 OK");
          if (strstr(filename, ".htm") != 0)
            client.println("Content-Type: text/html");
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
          while ((c = file.read()) >= 0) {
            client.write((char)c);
          }
          file.close();
        } 
        else {
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          file.open(&root,"404error.htm",O_READ);  //this is if the "GET /" fails to retrieve data
          int16_t c;
          while((c=file.read())>=0){  //sends file.  still not fast.
            client.write((char)c);
          }
          file.close();
        }
        break;
      }
    }
    delay(1);
    client.stop();
  }
}