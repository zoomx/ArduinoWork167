/*
    SerialRelayControl

    Control some relays and some sensors and one fan

    Pins are referred to the Relay Shield by Arduino.org


*/

#define FAN 1

#include "SerialCommand.h"   //https://github.com/kroimon/Arduino-SerialCommand

const byte RelayPin1 = 4;
const byte RelayPin2 = 7;
const byte RelayPin3 = 8;
const byte RelayPin4 = 12;

byte Relays[5];

SerialCommand sCmd;     // The SerialCommand object

unsigned long previousMillis = 0;
const long interval = 1000;     //CoolingMode routine interval

//*******************************************
#ifdef FAN

#define COLDTEMP 25
#include <DallasTemperature.h>
#include <OneWire.h>

const byte RelayFan = 12;
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress CoolerThermometer;

float ColdTempSetpoint = COLDTEMP;                 // Target temperature of the cooler in degrees C
const float ColdTempHysteresis = .125;               // +/- Switchpoint hysteresis of the cooler  in degrees C

// Variables:
float tempCP;                                     // Current cooler temperature measurement


//**********************************
void RunCoolingMode(float temp)
{
  if (temp = -127) {
    return;
  }
  //CoolingPeriod++;
  if (temp > ColdTempSetpoint + ColdTempHysteresis )           //If Temp>TempSet+Hysteresis
  {
    digitalWrite(RelayFan, HIGH);                          // Turn ON cooler
    //CoolingActive++;
    Serial.print("; RelayFan-HIGH");
    //Serial.print(" ; ");
    //Serial.print(ColdTempSetpoint + ColdTempHysteresis, DEC);
  }
  if (temp < ColdTempSetpoint - ColdTempHysteresis )           //If Temp<TempSet+Hysteresis
  {
    digitalWrite(RelayFan, LOW);                           // Turn OFF cooler
    Serial.print("; RelayFan-LOW");
    //Serial.print(" ; ");
    //Serial.print(ColdTempSetpoint - ColdTempHysteresis,DEC);
  }
}
//**********************************
void discoverOneWireDevices(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];

  Serial.print(F("Looking for 1-Wire devices...\n\r"));
  while (oneWire.search(addr)) {
    Serial.print(F("\n\rFound \'1-Wire\' device with address:\n\r"));
    for ( i = 0; i < 8; i++) {
      Serial.print(F("0x"));
      if (addr[i] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(F(", "));
      }
    }
    if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print(F("CRC is not valid!\n"));
      return;
    }
  }
  Serial.print(F("\n\r\n\rThat's it.\r\n"));
  oneWire.reset_search();
  return;
}
//**********************************
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print(F("0"));
    Serial.print(deviceAddress[i], HEX);
  }
}
#endif

//*********************Serial Commands**************************
void LED_on() {
  Serial.println("LED on");
  digitalWrite(LED_BUILTIN, HIGH);
}
//********************************
void LED_off() {
  Serial.println("LED off");
  digitalWrite(LED_BUILTIN, LOW);
}
//********************************
void CycleRelay() {
  Serial.println("CycleRelay");
  digitalWrite(RelayPin1, HIGH);
  delay(1000);
  digitalWrite(RelayPin1, LOW);
}
//********************************
void process_command()
{
  int aNumber;
  char *arg;

  Serial.println("Cycle relays");
  arg = sCmd.next();
  if (arg != NULL)
  {
    aNumber = atoi(arg);  // Converts a char string to an integer
    Serial.print("Relay: ");
    Serial.println(aNumber);
    if (aNumber < 5 && aNumber > 0)
    {
      digitalWrite(Relays[aNumber], HIGH);
      delay(1000);
      digitalWrite(Relays[aNumber], LOW);
    }
  }
  else {
    Serial.println("No relay number");
  }
}
//********************************
// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("What?");
}
//**************************SETUP*******************************
//**************************************************************
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(LED_BUILTIN, LOW);    // default to LED off

  pinMode(RelayPin1, OUTPUT);      // Configure RelayPin for output
  digitalWrite(RelayPin1, LOW);    // default off
  pinMode(RelayPin2, OUTPUT);      // Configure RelayPin for output
  digitalWrite(RelayPin2, LOW);    // default off
  pinMode(RelayPin3, OUTPUT);      // Configure RelayPin for output
  digitalWrite(RelayPin3, LOW);    // default off
  pinMode(RelayPin4, OUTPUT);      // Configure RelayPin for output
  digitalWrite(RelayPin4, LOW);    // default off

  Serial.begin(115200);
  Serial.println("\nSerialRelayControl");
  Serial.println("Version 0.1");
  Serial.print(__DATE__);
  Serial.print(F(" "));
  Serial.println(__TIME__);

  Relays[1] = 4;
  Relays[2] = 7;
  Relays[3] = 8;
  Relays[4] = 12;

  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("LEDON",    LED_on);          // Turns LED on
  sCmd.addCommand("LEDOFF",   LED_off);         // Turns LED off
  sCmd.addCommand("CREL", CycleRelay);        // Turn On Relay for one second
  sCmd.addCommand("R", process_command); // Turn On Relay for one second
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")

#ifdef FAN
  discoverOneWireDevices();
  sensors.getAddress(CoolerThermometer, 0);
  printAddress(CoolerThermometer);
  Serial.println("");
  sensors.setResolution(CoolerThermometer, 12);
  sensors.begin();
#endif

  Serial.println("Ready");
}
//**************************LOOP********************************
//**************************************************************
void loop() {
  sCmd.readSerial();     // We don't do much, just process serial commands
#ifdef FAN


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    sensors.requestTemperatures();

    tempCP = sensors.getTempCByIndex(0);
    Serial.print(sensors.getTempCByIndex(0));
    Serial.println();

    previousMillis = currentMillis;
    RunCoolingMode(tempCP);

  }



#endif
}

