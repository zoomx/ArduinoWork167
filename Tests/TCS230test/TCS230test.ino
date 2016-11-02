/*
   TCS230 board test
*/

#include <SPI.h>
#include <FreqCount.h>
#include <TimerOne.h>
#define S0 13 // TCS230 board pins
#define S1 12
#define S2 11
#define S3 10
#define OUT 5
#define DELAY 1000


void inizio() {
  pinMode(S0, OUTPUT); // OUTPUT FREQUENCY
  pinMode(S1, OUTPUT); // OUTPUT FREQUENCY
  pinMode(S2, OUTPUT); // RGB Selection
  pinMode(S3, OUTPUT); // RGB Selection
  pinMode(OUT, INPUT); // Frequency reader
  digitalWrite(S0, LOW); // OUTPUT FREQUENCY SCALING 2%
  digitalWrite(S1, HIGH);
}

void setup() {
  Serial.begin(115200);
  Serial.println("TCS230 test");
  FreqCount.begin(1000);
}

void loop() {
  unsigned long countR;
  unsigned long countB;
  unsigned long countG;
  unsigned long countW;

  digitalWrite(S2, LOW); // Red
  digitalWrite(S3, LOW);
  delay(DELAY);
  if (FreqCount.available()) {
    countR = FreqCount.read();
  }

  digitalWrite(S2, LOW); // Blue
  digitalWrite(S3, HIGH);
  delay(DELAY);
  if (FreqCount.available()) {
    countB = FreqCount.read();
  }

  digitalWrite(S2, HIGH); // Green
  digitalWrite(S3, HIGH);
  delay(DELAY);
  if (FreqCount.available()) {
    countG = FreqCount.read();
  }

  digitalWrite(S2, HIGH); // White
  digitalWrite(S3, LOW);
  delay(DELAY);
  if (FreqCount.available()) {
    countW = FreqCount.read();
  }

  Serial.print("->Red Fequency =");
  Serial.println(countR);
  Serial.print("->Blue Fequency =");
  Serial.println(countB);
  Serial.print("->Green Fequency =");
  Serial.println(countG);
  Serial.print("->White Fequency =");
  Serial.println(countW);
  Serial.println("-----------------------------------------------");
  delay(100);

}


