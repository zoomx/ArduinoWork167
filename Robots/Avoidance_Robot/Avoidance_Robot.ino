/*
   Avoidance_Robot
   By Robimek - 2015
   http://make.robimek.com/arduino-obstacle-robot/
   Traslation from Turkish by zoomx and Google!

   Connections
   Motors and L293D
   9,10,11,12
   Ultrasonic sensor
   6 trigger
   7 echo
   Servo
   4
   
*/

#include <Servo.h> // servo motor kütüphanesi
#include <NewPing.h>
//motor pins
#define OnMotor_left 9
#define OnMotor_right 10
#define ArkaMotor_forward 11
#define ArkaMotor_back 12
//sensor pins
#define USTrigger 6
#define USEcho 7
#define Maximum_distance 100
Servo servo; //servo motor definition
NewPing sonar(USTrigger, USEcho, Maximum_distance);//Ultrasonic sensor definition
//kullanılacak eleman tanımı
unsigned int distance;
unsigned int on_distance;
unsigned int left_distance;
unsigned int right_distance;
unsigned int time;


void setup()
{
  //motor outputs
  pinMode(OnMotor_left, OUTPUT);
  pinMode(OnMotor_right, OUTPUT);
  pinMode(ArkaMotor_forward, OUTPUT);
  pinMode(ArkaMotor_back, OUTPUT);
  servo.attach(4); //servo pin definition
}


void loop()
{
  delay(500);
  servo.write(90);
  sensor_measurement();
  on_distance = distance;
  if (on_distance > 35 || on_distance == 0)
  {
    forward();
  }
  else
  {
    stop();
    servo.write(180);
    delay(500);
    sensor_measurement();
    left_distance = distance;
    servo.write(0);
    delay(500);
    sensor_measurement();
    right_distance = distance;
    servo.write(90);
    delay(500);
    if (right_distance < left_distance)
    {
      right_back();
      delay(500);
      left_forward();
      delay(500);
      forward();
    }
    else if (left_distance < right_distance)
    {
      left_back();
      delay(500);
      right_forward();
      delay(500);
      forward();
    }
    else
    {
      back();
      delay(500);
    }
  }
}

// direction commands for the robot
void forward()
{
  digitalWrite(OnMotor_right, LOW);
  digitalWrite(OnMotor_left, LOW);
  digitalWrite(ArkaMotor_back, LOW);
  digitalWrite(ArkaMotor_forward, HIGH);
}
void back()
{
  digitalWrite(OnMotor_right, LOW);
  digitalWrite(OnMotor_left, LOW);
  digitalWrite(ArkaMotor_back, HIGH);
  digitalWrite(ArkaMotor_forward, LOW);
}
void left_forward()
{
  digitalWrite(OnMotor_right, LOW);
  digitalWrite(OnMotor_left, HIGH);
  digitalWrite(ArkaMotor_back, LOW);
  digitalWrite(ArkaMotor_forward, HIGH);
}
void left_back()
{
  digitalWrite(OnMotor_right, LOW);
  digitalWrite(OnMotor_left, HIGH);
  digitalWrite(ArkaMotor_back, HIGH);
  digitalWrite(ArkaMotor_forward, LOW);
}

void right_forward()
{
  digitalWrite(OnMotor_left, LOW);
  digitalWrite(OnMotor_right, HIGH);
  digitalWrite(ArkaMotor_forward, HIGH);
  digitalWrite(ArkaMotor_back, LOW);
}
void right_back()
{
  digitalWrite(OnMotor_left, LOW);
  digitalWrite(OnMotor_right, HIGH);
  digitalWrite(ArkaMotor_forward, LOW);
  digitalWrite(ArkaMotor_back, HIGH);
}

void stop()
{
  digitalWrite(OnMotor_right, LOW);
  digitalWrite(OnMotor_left, LOW);
  digitalWrite(ArkaMotor_forward, LOW);
  digitalWrite(ArkaMotor_back, LOW);
}
// distance measurement sensor
void sensor_measurement()
{
  delay(50);
  time = sonar.ping();
  distance = time / US_ROUNDTRIP_CM;
}
