/*
   lcdthermocouple
   Original by Adafruit
   this example is public domain. enjoy!
   https://learn.adafruit.com/thermocouple/
*/
#include <max6675.h>
#include <LiquidCrystal.h>
#include <Wire.h>

const int thermoDO = 4;
const int thermoCS = 5;
const int thermoCLK = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

const int vccPin = 3;
const int gndPin = 2;

LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// make a cute degree symbol
uint8_t degree[8]  = {140, 146, 146, 140, 128, 128, 128, 128};

void setup() {
  Serial.begin(115200);
  Serial.println("lcdthermocouple");
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);

  lcd.begin(16, 2);
  lcd.createChar(0, degree);

  // wait for MAX chip to stabilize
  delay(500);
}

void loop() {
  // basic readout test, just print the current temp
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MAX6675 test");

  // go to line #1
  lcd.setCursor(0, 1);
  lcd.print(thermocouple.readCelsius());
#if ARDUINO >= 100
  lcd.write((byte)0);
#else
  lcd.print(0, BYTE);
#endif
  lcd.print("C ");
  delay(1000);
}
