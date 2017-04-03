/*
   SerialThermoMillis
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


unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
unsigned long intervallo = 2500;
bool NewData;
//******************************************************
void CheckTime() {
  if ((millis() - previousMillis) > intervallo) {
    Serial.print("C = ");
    Serial.println(thermocouple.readCelsius());
    previousMillis = millis();
  }
}
//******************************************************
void setup() {
  Serial.begin(115200);
  Serial.println("SerialThermoMillis");
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
  Serial.print("C = ");
  Serial.println(thermocouple.readCelsius());
  previousMillis = millis();
}
//******************************************************
void loop() {
  CheckTime();
}
