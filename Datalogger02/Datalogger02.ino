/* #############################################################################
  #
  # Scriptname : Datalogger02.ino
  # Author     : Zoomx
  # Date       : 06.01.2017

  # Version    : 0.01
  # Date       : 06.01.2017

  # Description:
  # Datalogger program with
  # RTC
  # SDFAT
  # Acquisition every 5 minutes at 00, 05, 10, 15....
  # Record on SD
  # Start immediately


  Connections
  Vcc
  GND
  MOSI  D11
  SS    D10 Se non si collega non funziona!
  SCK   D13
  MISO  D12

  LED on PIN 13

  // #############################################################################
*/

// *********************************************
// INCLUDE
// *********************************************
//#include <Wire.h>
#include <DS1307new.h>  // version 1.24
//https://code.google.com/p/ds1307new/
//#include "RTClib.h"
#include <SPI.h>
#include "SdFat.h"
#include <DallasTemperature.h>
#include <OneWire.h>


// *********************************************
// DEFINE
// *********************************************
const uint8_t chipSelect = 10;
#define LED 13
#define PIN18B20 3
// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))

//
#define INLENGTH 5          //Needed for input with termination
#define INTERMINATOR 13     //Needed for input with termination
char inString[INLENGTH + 1]; //Needed for input with termination
int inCount;                //Needed for input with termination
char comm;

//
char FileName[12];

boolean AcqDone = false;  //It's time to acquisition!
boolean InAcq = false;    //If true=Acquisition is made

float intADC;

// File system object.
SdFat sd;
// Log file.
SdFile file;

//OneWire DS1820
#define ONE_WIRE_BUS PIN18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

uint8_t ore, minuti, secondi;

//------------------------------------------------------------------------------
void PrintVersion() {
  Serial.print(F("Version 0.01"));
  Serial.print(__DATE__);
  Serial.print(F(" "));
  Serial.println(__TIME__);
  Serial.flush();
}
//------------------------------------------------------------------------------
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
//------------------------------------------------------------------------------
void GetCharFromSerial() {
  //Get string from serial and put in inString
  //first letter in comm
  //http://forums.adafruit.com/viewtopic.php?f=8&t=9918
  inCount = 0;
  do
  {
    while (!Serial.available());             // wait for input
    inString[inCount] = Serial.read();       // get it
    //++inCount;
    if (inString [inCount] == INTERMINATOR) break;
  }
  while (++inCount < INLENGTH);
  inString[inCount] = 0;                     // null terminate the string
  //ch=inString;
  Serial.print(F("GetCharFromSerial->"));
  Serial.println(inString);
  Serial.flush();
  comm = inString[0];
  Serial.println(comm);
}
//------------------------------------------------------------------------------
void GenFileName() {
  RTC.getTime();
  char Buf[4];
  //RTC.year.toCharArray(YearBuf,4);
  itoa(RTC.year, Buf, DEC);
  FileName[0] = Buf[0];
  FileName[1] = Buf[1];
  FileName[2] = Buf[2];
  FileName[3] = Buf[3];

  itoa(RTC.month, Buf, DEC);
  FileName[4] = Buf[0];
  FileName[5] = Buf[1];
  if (Buf[1] == 0) {
    FileName[4] = '0';
    FileName[5] = Buf[0];
  }

  itoa(RTC.day, Buf, DEC);
  FileName[6] = Buf[0];
  FileName[7] = Buf[1];
  if (Buf[1] == 0) {
    FileName[6] = '0';
    FileName[7] = Buf[0];

  }
  //strcat(FileName, ".TXT");   //Curiosamente occupa più memoria!
  FileName[8] = '.';
  FileName[9] = 'T';
  FileName[10] = 'X';
  FileName[11] = 'T';
  FileName[12] = 0;
}
//------------------------------------------------------------------------------
//Date Serial print functions
//From the Monitor demo of DS1307new library
void mon_print_2d(uint8_t v)
{
  if ( v < 10 )
    Serial.print(F("0"));
  Serial.print(v, DEC);
}
//---------------------------------------------------
void mon_print_date(uint16_t y, uint8_t m, uint8_t d)
{
  Serial.print(y, DEC);
  Serial.print(F("/"));
  mon_print_2d(m);
  Serial.print(F("/"));
  mon_print_2d(d);
}
//---------------------------------------------------
void mon_print_time(uint16_t h, uint8_t m, uint8_t s)
{
  mon_print_2d(h);
  Serial.print(F(":"));
  mon_print_2d(m);
  Serial.print(F(":"));
  mon_print_2d(s);
}
//------------------------------------------------------------------------------
// SD file print functions
void SD_print_2d(uint8_t v)
{
  char cifra[2];
  cifra[1] = '0';
  if ( v < 10 ) {
    file.print("0");
    cifra[0] = char(v + 48);
    file.print(cifra);
  }
  else {
    cifra[0] = char(v + 48);
    file.print(cifra);
    cifra[0] = char(v % 10 + 48);
    file.print(cifra);
  }
}
//----------------------------------------------------
void SD_print_date(uint16_t y, uint8_t m, uint8_t d)
{
  SD_print_2d(y);
  file.print("/");
  SD_print_2d(m);
  file.print("/");
  SD_print_2d(d);
}
//----------------------------------------------------
void SD_print_time(uint16_t h, uint8_t m, uint8_t s)
{
  SD_print_2d(h);
  file.print(":");
  SD_print_2d(m);
  file.print(":");
  SD_print_2d(s);
}
//----------------------------------------------------
void SDPrintTime() {
  Serial.println(F("sdPrintTime"));
  RTC.getTime();
  file.print("20");    //RTC give you only the last 2 chars of a year, I add the first two.
  SD_print_date(RTC.year, RTC.month, RTC.day);
  file.print(" ");
  SD_print_time(RTC.hour, RTC.minute, RTC.second);
}
// End of SD print functions

//------------------------------------------------------------------------------
void TestSens() {
  Serial.println(F("Sensors Test"));
}
//------------------------------------------------------------------------------
void SDwriteTest() {
  byte res;
  Serial.println(F("SD write test1"));
  if (!file.open("BIGFILE.TXT", O_CREAT | O_WRITE)) {
    error("file.open");
  }
  Serial.println(freeRam());
  Serial.println(freeRam());
  file.print("01234567890");
  Serial.println(freeRam());
  file.close();
  Serial.println(freeRam());
  Serial.println(F("done!"));
}
//------------------------------------------------------------------------------
void GetTime() {
  Serial.println(F("GetTime"));
  RTC.getTime();
  mon_print_date(RTC.year, RTC.month, RTC.day);
  Serial.print(F(" "));
  mon_print_time(RTC.hour, RTC.minute, RTC.second);
  Serial.println();
  /*
    //Alternativa
    RTC.getTime();
    delay(100);
    Serial.print(RTC.year, DEC);
    Serial.print('/');
    Serial.print(RTC.month, DEC);
    Serial.print('/');
    Serial.print(RTC.day, DEC);
    Serial.print(' ');
    Serial.print(RTC.hour, DEC);
    Serial.print(':');
    Serial.print(RTC.minute, DEC);
    Serial.print(':');
    Serial.print(RTC.second, DEC);
    Serial.println();

  */

}
//------------------------------------------------------------------------------
void SetTime() {
  RTC.stopClock();
  Serial.println(F("SetTime"));
  Serial.print(F("Year"));
  GetCharFromSerial();
  ore = atoi(inString); //Use the same variables for Year..
  Serial.print(F("Month"));
  GetCharFromSerial();
  minuti = atoi(inString);
  Serial.print(F("Day"));
  GetCharFromSerial();
  secondi = atoi(inString);
  RTC.fillByYMD(ore, minuti, secondi);

  Serial.print(F("Hour"));
  GetCharFromSerial();
  ore = atoi(inString); //Use the same variables for Year..
  Serial.print(F("Minutes"));
  GetCharFromSerial();
  minuti = atoi(inString);
  Serial.print(F("Seconds"));
  GetCharFromSerial();
  secondi = atoi(inString);
  RTC.fillByHMS(ore, minuti, secondi);
  RTC.setTime();
  RTC.startClock();
}
//------------------------------------------------------------------------------
void StartAcq() {
  Serial.println(F("Start Acquisition"));
  //Serial.flush();
  if (InAcq == true) return;

  GenFileName();
  Serial.println(FileName);
  //Serial.flush();
  /*
    if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
      sd.initErrorHalt();
    }
    Serial.println(F("SD Begin OK"));
    if (!file.open(FileName, O_CREAT | O_WRITE | O_APPEND)) {
      error("file.open");
    }
  */
  Serial.print(F("Logging to: "));
  Serial.println(FileName);

  InAcq = true;
  Serial.println(F("Started"));
  //Serial.flush();
}
//------------------------------------------------------------------------------
void StopAcq() {
  Serial.println(F("Stop Acquisition"));
  //Serial.flush();
  InAcq = false;
  Serial.println(F("Stopped"));
  //Serial.flush();
  file.close();
}
//------------------------------------------------------------------------------
void PrintMenu() {
  Serial.println(F("1 Start Acquisition"));
  Serial.println(F("3 Stop Acquisition"));
  //Serial.println(F("4 Last data"));
  Serial.println(F("8 Info"));
  Serial.println(F("9 SD write test"));
  Serial.println(F("S Sensors Test"));
  Serial.println(F("A acqisition test"));
  Serial.println(F("v Print version"));
  Serial.println(F("T Get Time"));
  Serial.println(F("t Set Time"));
  Serial.println(F("B Blink LED"));
  Serial.println(F("--------------------"));
  Serial.println(F("Type number and press enter"));
}
//------------------------------------------------------------------------------
void blink() {
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  delay(100);
}
//------------------------------------------------------------------------------
void ParseMenu(char Stringa) {
  Serial.println(F("R3"));
  Serial.flush();
  switch (Stringa) {
    case '1':
      StartAcq();
      break;
    case '3':
      StopAcq();
      break;
    case '8':
      //InfoAcq();
      break;
    case '9':
      SDwriteTest();
      break;
    case 'B':
      blink;
      break;
    case 'v':
      PrintVersion();
      break;
    case 'S':
      TestSens();
      break;
    case 'A':
      AcqTest();
      break;
    case 'T':
      GetTime();
      break;
    case 't':
      SetTime();
      break;
    default:
      Serial.print(F("?? ->"));
      Serial.println(Stringa, HEX);
      Serial.flush();
  }
}

void acq()
{

  RTC.getTime();
  if (RTC.minute % 5 == 0) {  //Se il resto è zero i minuti finiscono per zero o 5
    if (AcqDone == false) { //Siccome il controllo viene fatto ogni 30 secondi evita
      //GetTime();        //che la routine scatti 2 volte ad es a 05 e 35 secondi
      //Acquisizione
      /*
        if (!file.exists(FileName)) {
        file.create(FileName);
        }
        else {
        file.openFile(FileName, FILEMODE_TEXT_WRITE);
        }
      */
      //file.printLn("Acquisizione!");
      AcqTest();
      Serial.println(F("Acquisition done!"));
      AcqDone = true; //Così non tento di aprire la SD subito dopo.
      return;
    }
    AcqDone = true; //forse non ci vuole
  }
  else
  {
    AcqDone = false;
  }
}

//------------------------------------------------------------------------------


double GetTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  t = 0;

  intADC = 0; //!!!!!!!! Only to evaluate coefficients!!!!

  for (int i = 0; i <= 25; i++) {

    ADCSRA |= _BV(ADSC);  // Start the ADC

    // Detect end-of-conversion
    while (bit_is_set(ADCSRA, ADSC));

    // Reading register "ADCW" takes care of how to read ADCL and ADCH.
    wADC = ADCW;

    intADC += wADC;
    // The offset of 324.31 could be wrong. It is just an indication.
    //t += (wADC - 324.31 ) / 1.22;
    //t += (wADC * 0.731 - 256.42);  //get a T too low
    //t += (wADC - 256.42 ) / 1.367989; //get a T too high
  }
  //t = t / 25;
  intADC = intADC / 25;  //!!!!!!!! Only to evaluate coefficients!!!! (but used anyway!!!)
  // The returned temperature is in degrees Celsius.
  t = intADC * 0.731 - 256.42;
  return (t);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void setup()
{
  byte res;
  Serial.begin(115200);
  Serial.println(F("Datalogger02"));

  pinMode(chipSelect, OUTPUT);


  // Initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }

  GenFileName();
  Serial.println(FileName);
  Serial.flush();


  Serial.println(F("Setup"));
  Serial.flush();
  StopAcq();

  sensors.begin();
  pinMode(LED, OUTPUT);
  StartAcq();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


void loop()
{
  /*
    //PrintMenu();
    //GetCharFromSerial();
    //Serial.print(F("R2->"));
    //Serial.println(inString);
    //Serial.flush();
    //ParseMenu(comm);
    //Serial.println();
    //Serial.flush();
    acq();
    Serial.println("cycle 10");
    delay(10000);
  */
  if (InAcq == true) {
    acq();
    if (Serial.available()) {
      // Close file and stop.
      file.close();
      Serial.println(F("Done"));
      InAcq = false;
    }

  }
  else {
    PrintMenu();
    GetCharFromSerial();
    Serial.print(F("Ricevuto->"));
    Serial.println(comm);
    ParseMenu(comm);
  }
}



//------------------------------------------------------------------------------


void AcqTest() {
  GetTime();
  Serial.print(GetTemp(), 1);
  Serial.print(";");
  sensors.requestTemperatures();     // Send the command to get temperatures
  Serial.print(sensors.getTempCByIndex(0));
  Serial.print(";");
  Serial.print(intADC);
  Serial.println();
  //Memorizzazione
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }
  GenFileName();
  if (!file.open(FileName, O_CREAT | O_WRITE | O_APPEND)) {
    //error("file.open");
    Serial.println(F("Error file open"));
    return;
  }

  //SDPrintTime();
  RTC.getTime();
  delay(100);
  file.print(RTC.year, DEC);
  file.print('/');
  file.print(RTC.month, DEC);
  file.print('/');
  file.print(RTC.day, DEC);
  file.print(' ');
  file.print(RTC.hour, DEC);
  file.print(':');
  file.print(RTC.minute, DEC);
  file.print(':');
  file.print(RTC.second, DEC);
  file.print("; ");

  file.print(GetTemp(), 1);
  file.print("; ");
  sensors.requestTemperatures();     // Send the command to get temperatures
  file.print(sensors.getTempCByIndex(0));
  file.print("; ");
  file.print(intADC);
  file.println();
  file.close();
}
