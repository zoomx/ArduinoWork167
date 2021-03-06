/*
  Thermorecorder01

  NOT TESTED!!!

  Record thermocouple on a SD
  Start and stop by button

  CONNECTIONS
  Arduino MAX6675

  Arduino  MAX6675
  2       GND
  3       Vcc
  4       DO
  5       CS
  6       CLK

  Digital pins
  7 Pushbutton for start and stop logging
  2017/03/28

  Remember SS is on pin 10
  11,12 and 13 are used bi SPI and SD
  10
  11
  12
  13


  DS1307
  Pin used A4 (SDA),A5 (SDL)

  Button on PIN 7

  /Arduino Button Library by Jack Christensen
  https://github.com/JChristensen/Button

  //Arduino DS1307 librery by Peter Schmelzer & Oliver Kraus
*/


#include <SPI.h>
#include <SdFat.h>

#include <Wire.h>
#include "DS1307new.h"     //by Peter Schmelzer & Oliver Kraus
#include "Button.h"        //https://github.com/JChristensen/Button
#include "max6675.h"

const uint8_t thermoDO = 4;
const uint8_t thermoCS = 5;
const uint8_t thermoCLK = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const uint8_t vccPin = 3;
const uint8_t gndPin = 2;

// Log file base name.  Must be six characters or less.
#define FILE_BASE_NAME "Data"


#define BUTTON 7
#define PULLUP false        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
//switch is closed, this is negative logic, i.e. a high state
//means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.
#define LONG_PRESS 1000    //We define a "long press" to be 1000 milliseconds.

// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))

#define INLENGTH 5          //Needed for input with termination
#define INTERMINATOR 13     //Needed for input with termination
char inString[INLENGTH + 1]; //Needed for input with termination
uint8_t inCount;                //Needed for input with termination
char comm;

// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
const uint8_t chipSelect = SS;

// File system object.
SdFat sd;

// Log file.
SdFile file;

bool InAcq = false;

uint8_t ore, minuti, secondi;

Button pushbutton(BUTTON, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the button

//Serial input withEndMarker
//http://forum.arduino.cc/index.php?topic=288234.0
const byte numChars = 32;
char receivedChars[numChars];  // an array to store the received data
boolean newData = false;

float Temp;

uint32_t previousMillis = 0;
uint32_t intervallo = 2500;

//==============================================================================
// User functions.
//------------------------------------------------------------------------------
void TestSens() {
  Serial.println(F("Sensors Test"));
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
void GetTime() {
  Serial.println(F("GetTime"));
  RTC.getTime();
  Serial.print(F("RTC date: "));
  mon_print_date(RTC.year, RTC.month, RTC.day);
  Serial.print(F(" "));
  Serial.print(F("  time: "));
  mon_print_time(RTC.hour, RTC.minute, RTC.second);
  Serial.println();
}
//------------------------------------------------------------------------------
void mon_print_2d(uint8_t v)
{
  if ( v < 10 )
    Serial.print(F("0"));
  Serial.print(v, DEC);
}
//------------------------------------
void mon_print_3d(uint8_t v)
{
  if ( v < 10 )
    Serial.print(F(" "));
  if ( v < 100 )
    Serial.print(F(" "));
  Serial.print(v, DEC);
}
//------------------------------------
void mon_print_date(uint16_t y, uint8_t m, uint8_t d)
{
  Serial.print(y, DEC);
  Serial.print(F("-"));
  mon_print_2d(m);
  Serial.print(F("-"));
  mon_print_2d(d);
}
//------------------------------------
void mon_print_time(uint16_t h, uint8_t m, uint8_t s)
{
  mon_print_2d(h);
  Serial.print(F(":"));
  mon_print_2d(m);
  Serial.print(F(":"));
  mon_print_2d(s);
}

//------------------------------------------------------------------------------
void SD_print_2d(uint8_t v)
{
  if ( v < 10 )
    file.print(F("0"));
  file.print(v, DEC);
}
//------------------------------------
void SD_print_3d(uint8_t v)
{
  if ( v < 10 )
    file.print(F(" "));
  if ( v < 100 )
    file.print(F(" "));
  file.print(v, DEC);
}
//------------------------------------
void SD_print_date(uint16_t y, uint8_t m, uint8_t d)
{
  file.print(y, DEC);
  file.print(F("/"));
  SD_print_2d(m);
  file.print(F("/"));
  SD_print_2d(d);
}
//------------------------------------
void SD_print_time(uint16_t h, uint8_t m, uint8_t s)
{
  SD_print_2d(h);
  file.print(F(":"));
  SD_print_2d(m);
  file.print(F(":"));
  SD_print_2d(s);
}
//------------------------------------------------------------------------------
/*
  void BlinkGreen() {
  Serial.println(F("Blink Green LED"));
  digitalWrite(GREENLED, HIGH);
  delay(100);
  digitalWrite(GREENLED, LOW);
  delay(100);
  }
  //------------------------------------------------------------------------------
  void BlinkRed() {
  Serial.println(F("Blink Red LED"));
  digitalWrite(REDLED, HIGH);
  delay(100);
  digitalWrite(REDLED, LOW);
  delay(100);
  }
*/
//------------------------------------------------------------------------------
void StopAcq() {
  Serial.println(F("Stop Acquisition"));
  InAcq = false;
}

//------------------------------------------------------------------------------
void StartAcq() {
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.csv";

  Serial.println(F("Start Acquisition"));
  // Initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }

  // Find an unused file name.
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      error("Can't create file name");
    }
  }
  if (!file.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
    error("file.open");
    return;
  }
  do {
    delay(10);
  } while (Serial.read() >= 0);
  Serial.print(F("Logging to: "));
  Serial.println(fileName);
  // Write data header.
  writeHeader();
  InAcq = true;
  logData(thermocouple.readCelsius());
  previousMillis = millis();
}
//------------------------------------------------------------------------------
void PrintVersion() {
  Serial.print(F("Version 0.1 "));
  Serial.print(__DATE__);
  Serial.print(F(" "));
  Serial.println(__TIME__);
}
//------------------------------------------------------------------------------
void GetCharFromSerial() {
  //Get string from serial and put in inString
  //first letter in comm
  Serial.flush(); //flush all previous received and transmitted data
  inCount = 0;
  do
  {
    while (!Serial.available());             // wait for input
    inString[inCount] = Serial.read();       // get it
    //++inCount;
    if (inString [inCount] == INTERMINATOR) break;
  } while (++inCount < INLENGTH);
  inString[inCount] = 0;                     // null terminate the string
  //ch=inString;
  Serial.print(F("Ricevuto->"));
  Serial.println(inString);
  comm = inString[0];
}
//------------------------------------------------------------------------------
void CheckTime() {
  if ((millis() - previousMillis) > intervallo) {
    Serial.print("C = ");
    //Serial.println(thermocouple.readCelsius());
    logData(thermocouple.readCelsius());
    previousMillis = millis();
  }
}
/*
  //------------------------------------------------------------------------------
  void showNewData() {
  if (newData == true) {
    //Serial.print("This just in ... ");
    Serial.print(receivedChars);
    Serial.println();
    logData(Temp);
    newData = false;
  }
  }
*/
//------------------------------------------------------------------------------
// Write data header.
void writeHeader() {
  file.print(F("Time"));
  file.print(F(";Temperature"));
  file.println();
}

//------------------------------------------------------------------------------
// Log a data record.
void logData(float temp) {

  Serial.println(temp);
  RTC.getTime();
  delay(100);
  SD_print_date(RTC.year, RTC.month, RTC.day);
  file.print(F(" "));
  SD_print_time(RTC.hour, RTC.minute, RTC.second);
  file.print(':');

  // Write data to file.
  //RTC.getTime();
  //delay(100);
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
  //file.println();
  // Write data to file.  Start with log time in micros.


  file.write(';');
  file.print(temp);

  file.println();
  //BlinkGreen();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void PrintMenu() {
  Serial.println(F("v Print version"));
  Serial.println(F("T Get Time"));
  Serial.println(F("t Set Time"));
  Serial.println(F("B Blink green LED"));
  Serial.println(F("R Blink red LED"));
  Serial.println(F("m print Menu"));
  Serial.println(F("--------------------"));
  Serial.println(F("Type choice and press enter"));
}

//------------------------------------------------------------------------------
void ParseMenu(char Stringa) {
  Serial.println(F("Parse Menu"));
  switch (Stringa) {
    case 'm':
      PrintMenu();
      break;
    case 'B':
      //BlinkGreen();
      break;
    case 'R':
      //BlinkRed();
      break;
    case 'v':
      PrintVersion();
      break;
    case 'S':
      TestSens();
      break;
    case 'T':
      GetTime();
      break;
    case 't':
      SetTime();
      break;
    default:
      Serial.print(F("Command Unknown! ->"));
      Serial.println(Stringa, HEX);
  }
}

//------------------------------------------------------------------------------
//==============================================================================
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  /*
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
  */

  Serial.println(F("Thermorecorder01"));

  //pinMode(GREENLED, OUTPUT);
  //pinMode(REDLED, OUTPUT);
  //pinMode(BUZZER, OUTPUT);
  //digitalWrite(GREENLED, HIGH);
  //digitalWrite(BUZZER, LOW);

  //pinMode(BUTTON, INPUT);  //INPUT_PULLUP   //Done by the button library

  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  PrintMenu();
}

void loop() {
  pushbutton.read();                //Read the button

  if (InAcq == true) {
    //recvWithEndMarker();
    //receivedChars[7] = '\0';
    //showNewData();   //Maybe change in logdata!!!
    CheckTime();
    if (pushbutton.pressedFor(LONG_PRESS)) {
      // Close file and stop.
      file.close();
      Serial.println(F("Done"));
      InAcq = false;
      //digitalWrite(GREENLED, HIGH);
      delay(5000);  //Avoid to cach button again and stops!
    }
    /*
        if (Serial.available()) {  //button check for stop
          // Close file and stop.
          file.close();
          Serial.println(F("Done"));
          InAcq = false;
        }
    */
  }
  else {
    //buttoncheck for start
    if (pushbutton.pressedFor(LONG_PRESS)) {
      Serial.println("Long press!");
      StartAcq();
      InAcq = true;
      //digitalWrite(GREENLED, LOW);
      Serial.println("Release button now!");
      delay(5000);  //Avoid to cach button again and stops!
    }
  }


  if (Serial.available()) {
    GetCharFromSerial();
    Serial.print(F("Ricevuto->"));
    Serial.println(inString);
    ParseMenu(comm);
  }
}


