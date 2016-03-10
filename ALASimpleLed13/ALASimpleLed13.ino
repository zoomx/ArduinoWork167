///////////////////////////////////////////////////////////////////////////////////////////
//
// ALA library example: SimpleLed
//
// A very simple example about how to fade one LED.
//
// Web page: http://yaab-arduino.blogspot.com/p/ala-example-simpleled.html
//
///////////////////////////////////////////////////////////////////////////////////////////

#define LED 13

#include <AlaLed.h>

AlaLed alaLed;

void setup()
{
  Serial.begin(9600);
  Serial.println("ALASimpleLED13");

  // initialize the led attached to pin LED with PWM driver
  alaLed.initPWM(LED);

  // set a fading animation with a duration of 2 seconds
  alaLed.setAnimation(ALA_GLOW, 2000);
}

void loop()
{
  // run the animation
  alaLed.runAnimation();
}

