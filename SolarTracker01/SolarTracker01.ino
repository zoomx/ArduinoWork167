
/*
  SolarTracker01
  by ELECTRONICS HUB FEBRUARY 29, 2016
  http://www.electronicshub.org/arduino-solar-tracker/
  modifications by Zoomx

  LDRs are used as the main light sensors. Two servo motors are fixed to the structure that holds
  the solar panel. The program for Arduino is uploaded to the microcontroller. The working of the
  project is as follows.

  LDRs sense the amount of sunlight falling on them. Four LDRs are divided into top, bottom,
  left and right.

  For east – west tracking, the analog values from two top LDRs and two bottom LDRs are compared
  and if the top set of LDRs receive more light, the vertical servo will move in that direction.

  If the bottom LDRs receive more light, the servo moves in that direction.

  For angular deflection of the solar panel, the analog values from two left LDRs and two right
  LDRs are compared. If the left set of LDRs receive more light than the right set, the horizontal
  servo will move in that direction.

  If the right set of LDRs receive more light, the servo moves in that direction.


*/



#include <Servo.h>
//defining Servos
Servo servohori;
const byte servohpin = 10;
int servoh = 0;
const int servohLimitHigh = 160;
const int servohLimitLow = 20;

Servo servoverti;
const byte servovpin = 10;
int servov = 0;
const int servovLimitHigh = 160;
const int servovLimitLow = 20;
//Assigning LDRs
const int ldrtopl = 2; //top left LDR green
const int ldrtopr = 1; //top right LDR yellow
const int ldrbotl = 3; // bottom left LDR blue
const int ldrbotr = 0; // bottom right LDR orange

void setup ()
{
  Serial.begin(115200);
  Serial.println("SolarTracker01");
  servohori.attach(servohpin);
  servohori.write(servohLimitLow);
  servoverti.attach(servovpin);
  servoverti.write(servovLimitLow);
  delay(500);
}

void loop()
{
  servoh = servohori.read();
  servov = servoverti.read();
  //capturing analog values of each LDR
  int topl = analogRead(ldrtopl);
  int topr = analogRead(ldrtopr);
  int botl = analogRead(ldrbotl);
  int botr = analogRead(ldrbotr);
  
  Serial.print(ldrtopl);
  Serial.print(" ");
  Serial.print(ldrtopr);
  Serial.print(" ");
  Serial.print(ldrbotl);
  Serial.print(" ");
  Serial.println(ldrbotr);
  //Serial.println("");
  
  // calculating average
  int avgtop = (topl + topr) / 2; //average of top LDRs
  int avgbot = (botl + botr) / 2; //average of bottom LDRs
  int avgleft = (topl + botl) / 2; //average of left LDRs
  int avgright = (topr + botr) / 2; //average of right LDRs

  if (avgtop < avgbot)
  {
    servoverti.write(servov + 1);
    if (servov > servovLimitHigh)
    {
      servov = servovLimitHigh;
    }
    delay(10);
  }
  else if (avgbot < avgtop)
  {
    servoverti.write(servov - 1);
    if (servov < servovLimitLow)
    {
      servov = servovLimitLow;
    }
    delay(10);
  }
  else
  {
    servoverti.write(servov);
  }

  if (avgleft > avgright)
  {
    servohori.write(servoh + 1);
    if (servoh > servohLimitHigh)
    {
      servoh = servohLimitHigh;
    }
    delay(10);
  }
  else if (avgright > avgleft)
  {
    servohori.write(servoh - 1);
    if (servoh < servohLimitLow)
    {
      servoh = servohLimitLow;
    }
    delay(10);
  }
  else
  {
    servohori.write(servoh);
  }
  delay(50);
}
