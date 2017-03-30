/*
   serialthermocouple
   Original by Adafruit
   this example is public domain. enjoy!
   https://learn.adafruit.com/thermocouple/
*/
#include "max6675.h"

const int thermoDO = 4;
const int thermoCS = 5;
const int thermoCLK = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const int vccPin = 3;
const int gndPin = 2;
//******************************************************
void setup() {
  Serial.begin(115200);
  Serial.println("serialthermocouple");
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
}
//******************************************************
void loop() {
  // basic readout test, just print the current temp
  Serial.print("C = ");
  Serial.println(thermocouple.readCelsius());
  delay(1000);
}
