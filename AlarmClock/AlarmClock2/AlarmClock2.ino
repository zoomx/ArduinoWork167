/**
 *    AlarmClock2 - Added set hour and set minutes
 *    An Arduino sketch that makes use of an LCD Keypad Shield and a DS1302 RTC to build an Alarm Clock.
 *    
 *    
 *    Original version
 *    https://github.com/pAIgn10/AlarmClock
 *    DS1302 version by zoomx
 *
 * Description:
 * This is an example sketch that builds an Alarm Clock out of an Arduino Uno,
 * an LCD Keypad Shield, a DS1307 Real Time Clock, and a buzzer
 * The example lets you display the date and time, and also set the alarm.
 * There are 4 modes of operation in which the buttons on the LCD have specific functions.
 * "Display Date Time" mode: Date, Time, and an Alarm indication display on the LCD (default mode)
 *      SELECT: Lets you set the alarm time
 *      LEFT: Displays the current alarm time
 *      RIGHT: Turns the alarm on and off
 * "Display Alarm Time":
 *      No active buttons. After 3 seconds the clock returns to the default mode
 * "Set Alarm" mode: Lets you set the alarm time (hours & minutes)
 *      UP: Increases hours/minutes **
 *      DOWN: Decreases hours/minutes **
 *      SELECT: Accepts the hours/minutes value
 *      LEFT: When in Set Alarm Minutes jump to Set Hours
 * "Set Time" mode: Lets you to set hours and minutes of RTC
 *      UP: Increases hours/minutes **
 *      DOWN: Decreases hours/minutes **
 *      SELECT: Accepts the hours/minutes value
 * "Buzzer Ringing" mode: When the alarm time has been met, the buzzer rings
 *      SELECT or LEFT: Turns off the buzzer and the alarm
 *      UP or DOWN: Snooze -> Adds 10 minutes to the alarm time
 * ** After 5 seconds of inactivity, the clock returns to default mode
 *
 * Note 1:
 * The development of the code was based on a Finite State Machine (FSM)
 * you can find on github: https://github.com/pAIgn10/AlarmClock
 * The code is quite simple and was made possible due to the above
 * preliminary design. You are urged to study both the FSM and the code.
 * Note 2:
 * The time on the RTC can shift over time. If you notice a deviation building up,
 * just uncomment line 91 and load the example to your Arduino. That will set the
 * computer's time on the RTC. Afterwards, make sure to reupload the code with
 * line 91 commented out. If you don't do that, the next time your Arduino resets
 * it will write the time again on the RTC... the time of the code's compilation.
 *
 * Author:
 *   Nick Lamprianidis { paign10.ln [at] gmail [dot] com }
 *
 * Modifications by Zoomx 2015
 * SLCDKeypad and DS1302 libraries are in the same sketch folder
 * Changed from DS1307 to DS1302
 * Added Set hours and minutes of RTC
 * When you set hours and minutes you start from the actual values and not from zero
 *
 * License:
 *   Copyright (c) 2014 Nick Lamprianidis
 *   This code is released under the MIT license
 *   http://www.opensource.org/licenses/mit-license.php
 */

#include <LiquidCrystal.h>  // Required by LCDKeypad
#include "LCDKeypad.h"
#include "DS1302.h"  //https://github.com/msparks/arduino-ds1302

#define TIME_OUT 5  // One of the system's FSM transitions
#define ALARM_TIME_MET 6  // One of the system's FSM transitions

#define BUZZER_PIN 3  // Output PWM pin for the buzzer
#define SNOOZE 10  // Minutes to snooze

// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. See the DS1302
// datasheet:
//
//   http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
const int kCePin   = 13;  // Chip Enable
const int kIoPin   = 11;  // Input/Output
const int kSclkPin = 12;  // Serial Clock

// The different states of the system
enum states
{
  SHOW_TIME,           // Displays the time and date
  SHOW_TIME_ALARM_ON,  // Displays the time and date, and alarm is on
  SHOW_ALARM_TIME,     // Displays the alarm time and goes back to time and date after 3 seconds
  SET_ALARM_HOUR,      // Option for setting the alarm hours. If provided, it moves on to alarm minutes.
  //   Otherwise, it times out after 5 seconds and returns to time and date
  SET_ALARM_MINUTES,   // Option for setting the alarm minutes. If provided, it finally sets the alarm time and alarm.
  //   Otherwise, it times out after 5 seconds and returns to time and date
  BUZZER_ON,            // Displays the time and date, and buzzer is on (alarm time met)
  SET_HOUR,
  SET_MINUTES
};

// Creates an LCDKeypad instance
// It handles the LCD screen and buttons on the shield
LCDKeypad lcd;

// Create a DS1302 object.
DS1302 rtc(kCePin, kIoPin, kSclkPin);

states state;  // Holds the current state of the system
int8_t button;  // Holds the current button pressed
uint8_t alarmHours = 0, alarmMinutes = 0;  // Holds the current alarm time
uint8_t tmpHours;
boolean alarm = false;  // Holds the current state of the alarm
unsigned long timeRef;
//Time DateTime; // now;  // Holds the current date and time information

/*
String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}
*/
String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "Sun";
    case Time::kMonday: return "Mon";
    case Time::kTuesday: return "Tue";
    case Time::kWednesday: return "Wed";
    case Time::kThursday: return "Thu";
    case Time::kFriday: return "Fri";
    case Time::kSaturday: return "Sat";
  }
  return "Unk";
}

void printTime() {
  // Get the current time and date from the chip.
  Time t = rtc.time();

  // Name the day of the week.
  const String day = dayAsString(t.day);

  // Format the time and date and insert into the temporary buffer.
  char buf[50];
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
           day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);

  // Print the formatted string to serial so we can see the time.
  Serial.println(buf);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("AlarmClock2");
  pinMode(BUZZER_PIN, OUTPUT);  // Buzzer pin

  // Initializes the LCD and RTC instances
  lcd.begin(16, 2);

  // Initialize a new chip by turning off write protection and clearing the
  // clock halt flag. These methods needn't always be called. See the DS1302
  // datasheet for details.
  rtc.writeProtect(false);
  rtc.halt(false);

  state = SHOW_TIME;  // Initial state of the FSM

}

// Has the main control of the FSM (1Hz refresh rate)
void loop()
{
  timeRef = millis();

  // Uses the current state to decide what to process
  switch (state)
  {
    case SHOW_TIME:
      showTime();
      break;
    case SHOW_TIME_ALARM_ON:
      showTime();
      checkAlarmTime();
      break;
    case SHOW_ALARM_TIME:
      showAlarmTime();
      break;
    case SET_ALARM_HOUR:
      setAlarmHours();
      if ( state != SET_ALARM_MINUTES ) break;
    case SET_ALARM_MINUTES:
      setAlarmMinutes();
      break;
    case SET_HOUR:
      setHours();
      if ( state != SET_MINUTES ) break;
    case SET_MINUTES:
      setMinutes();
      break;
    case BUZZER_ON:
      showTime();
      break;
  }

  // Waits about 1 sec for events (button presses)
  // If a button is pressed, it blocks until the button is released
  // and then it performs the applicable state transition
  while ( (unsigned long)(millis() - timeRef) < 970 )
  {
    if ( (button = lcd.button()) != KEYPAD_NONE )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      transition(button);
      break;
    }
  }
}

// Looks at the provided trigger (event)
// and performs the appropriate state transition
// If necessary, sets secondary variables
void transition(uint8_t trigger)
{
  switch (state)
  {
    case SHOW_TIME:
      if ( trigger == KEYPAD_LEFT ) state = SHOW_ALARM_TIME;
      else if ( trigger == KEYPAD_RIGHT ) {
        alarm = true;
        state = SHOW_TIME_ALARM_ON;
      }
      else if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_HOUR;
      break;
    case SHOW_TIME_ALARM_ON:
      if ( trigger == KEYPAD_LEFT ) state = SHOW_ALARM_TIME;
      else if ( trigger == KEYPAD_RIGHT ) {
        alarm = false;
        state = SHOW_TIME;
      }
      else if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_HOUR;
      else if ( trigger == ALARM_TIME_MET ) {
        analogWrite(BUZZER_PIN, 220);
        state = BUZZER_ON;
      }
      break;
    case SHOW_ALARM_TIME:
      if ( trigger == TIME_OUT ) {
        if ( !alarm ) state = SHOW_TIME;
        else state = SHOW_TIME_ALARM_ON;
      }
      break;
    case SET_ALARM_HOUR:
      if ( trigger == KEYPAD_SELECT ) state = SET_ALARM_MINUTES;
      else if ( trigger == TIME_OUT ) {
        if ( !alarm ) state = SHOW_TIME;
        else state = SHOW_TIME_ALARM_ON;
      }
      break;

    case SET_ALARM_MINUTES:
      if ( trigger == KEYPAD_SELECT ) {
        alarm = true;
        state = SHOW_TIME_ALARM_ON;
      }
      else if ( trigger == KEYPAD_LEFT ) {
        state = SET_HOUR;
      }
      else if ( trigger == TIME_OUT ) {
        if ( !alarm ) state = SHOW_TIME;
        else state = SHOW_TIME_ALARM_ON;
      }

      break;

    case SET_HOUR:
      if ( trigger == KEYPAD_SELECT ) state = SET_MINUTES;
      else if ( trigger == TIME_OUT ) {
        state = SHOW_TIME;
      }
      break;

    case SET_MINUTES:
      if ( trigger == KEYPAD_SELECT ) {
        state = SHOW_TIME;
      }
      else if ( trigger == TIME_OUT ) {
        state = SHOW_TIME;
      }

      break;



    case BUZZER_ON:
      if ( trigger == KEYPAD_UP || trigger == KEYPAD_DOWN ) {
        analogWrite(BUZZER_PIN, 0);
        snooze(); state = SHOW_TIME_ALARM_ON;
      }
      if ( trigger == KEYPAD_SELECT || trigger == KEYPAD_LEFT ) {
        analogWrite(BUZZER_PIN, 0);
        alarm = false; state = SHOW_TIME;
      }
      break;
  }
}

// Displays the current date and time, and also an alarm indication
// e.g. SAT 04 JAN 2014, 22:59:10  ALARM
void showTime()
{
  // Get the current time and date from the chip.
  Time t = rtc.time();
  // Name the day of the week.
  const String day = dayAsString(t.day);

  //const char* dayName[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
  const char* monthName[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
  lcd.clear();
  lcd.print(String(day.c_str()) + " " +
            (t.date < 10 ? "0" : "") + t.date + " " +
            monthName[t.mon - 1] + " " + t.yr);
  lcd.setCursor(0, 1);
  lcd.print((t.hr < 10 ? "0" : "") + String(t.hr) + ":" +
            (t.min < 10 ? "0" : "") + t.min + ":" +
            (t.sec < 10 ? "0" : "") + t.sec + (alarm ? "  ALARM" : ""));
}

// Displays the current alarm time and transitions back to show
// date and time after 2 sec (+ 1 sec delay from inside the loop function)
// e.g. Alarm Time HOUR: 08 MIN: 20
void showAlarmTime()
{
  lcd.clear();
  lcd.print("Alarm Time");
  lcd.setCursor(0, 1);
  lcd.print(String("HOUR: ") + ( alarmHours < 9 ? "0" : "" ) + alarmHours +
            " MIN: " + ( alarmMinutes < 9 ? "0" : "" ) + alarmMinutes);
  delay(2000);
  transition(TIME_OUT);
}

// Checks if the alarm time has been met,
// and if so initiates a state transition
void checkAlarmTime()
{
  // Get the current time and date from the chip.
  Time t = rtc.time();
  if ( t.yr == alarmHours && t.min == alarmMinutes ) transition(ALARM_TIME_MET);
}

// When the buzzer is ringing, by pressing the UP or DOWN buttons,
// a SNOOZE (default is 10) minutes delay on the alarm time happens
void snooze()
{
  alarmMinutes += SNOOZE;
  if ( alarmMinutes > 59 )
  {
    alarmHours += alarmMinutes / 60;
    alarmMinutes = alarmMinutes % 60;
  }
}

// The first of a 2 part process for setting the alarm time
// Receives the alarm time hour. If not provided within 5 sec,
// times out and returns to a previous (time and date) state
void setAlarmHours()
{
  unsigned long timeRef;
  boolean timeOut = true;

  lcd.clear();
  lcd.print("Alarm Time");

  tmpHours = 0;
  timeRef = millis();
  lcd.setCursor(0, 1);
  lcd.print("Set hours:  0");

  while ( (unsigned long)(millis() - timeRef) < 5000 )
  {
    uint8_t button = lcd.button();

    if ( button == KEYPAD_UP )
    {
      tmpHours = tmpHours < 23 ? tmpHours + 1 : tmpHours;
      lcd.setCursor(11, 1);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      if ( tmpHours < 10 ) lcd.print(" ");
      lcd.print(tmpHours);
      timeRef = millis();
    }
    else if ( button == KEYPAD_DOWN )
    {
      tmpHours = tmpHours > 0 ? tmpHours - 1 : tmpHours;
      lcd.setCursor(11, 1);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      if ( tmpHours < 10 ) lcd.print(" ");
      lcd.print(tmpHours);
      timeRef = millis();
    }
    else if ( button == KEYPAD_SELECT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      timeOut = false;
      break;
    }
    delay(150);
  }

  if ( !timeOut ) transition(KEYPAD_SELECT);
  else transition(TIME_OUT);
}

// The second of a 2 part process for setting the alarm time
// Receives the alarm time minutes. If not provided within 5 sec,
// times out and returns to a previous (time and date) state
// If minutes are provided, sets the alarm time and turns the alarm on
void setAlarmMinutes()
{
  unsigned long timeRef;
  boolean timeOut = true;
  uint8_t tmpMinutes = 0;
  bool LeftPressed = false;
  lcd.clear();
  lcd.print("Alarm Time");

  timeRef = millis();
  lcd.setCursor(0, 1);
  lcd.print("Set minutes:  0");

  while ( (unsigned long)(millis() - timeRef) < 5000 )
  {
    uint8_t button = lcd.button();

    if ( button == KEYPAD_UP )
    {
      tmpMinutes = tmpMinutes < 55 ? tmpMinutes + 5 : tmpMinutes;
      lcd.setCursor(13, 1);
      lcd.print("  ");
      lcd.setCursor(13, 1);
      if ( tmpMinutes < 10 ) lcd.print(" ");
      lcd.print(tmpMinutes);
      timeRef = millis();
    }
    else if ( button == KEYPAD_DOWN )
    {
      tmpMinutes = tmpMinutes > 0 ? tmpMinutes - 5 : tmpMinutes;
      lcd.setCursor(13, 1);
      lcd.print("  ");
      lcd.setCursor(13, 1);
      if ( tmpMinutes < 10 ) lcd.print(" ");
      lcd.print(tmpMinutes);
      timeRef = millis();
    }
    else if ( button == KEYPAD_SELECT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      timeOut = false;
      break;
    }
    else if ( button == KEYPAD_LEFT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      LeftPressed = true;
      timeOut = false;
      break;
    }
    delay(150);
  }

  if ( !timeOut )
  {
    alarmHours = tmpHours;
    alarmMinutes = tmpMinutes;
    if ( LeftPressed == true ) {
      transition(KEYPAD_LEFT);
    }
    else
    {
      transition(KEYPAD_SELECT);
    }
  }
  else transition(TIME_OUT);
}


void setHours()
{
  unsigned long timeRef;
  boolean timeOut = true;

  lcd.clear();
  lcd.print("Time");

  Time t = rtc.time();
  tmpHours = t.hr;

  timeRef = millis();
  lcd.setCursor(0, 1);
  lcd.print("Set hours: ");
  lcd.print(tmpHours);

  while ( (unsigned long)(millis() - timeRef) < 5000 )
  {
    uint8_t button = lcd.button();

    if ( button == KEYPAD_UP )
    {
      tmpHours = tmpHours < 23 ? tmpHours + 1 : tmpHours;
      lcd.setCursor(11, 1);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      if ( tmpHours < 10 ) lcd.print(" ");
      lcd.print(tmpHours);
      timeRef = millis();
    }
    else if ( button == KEYPAD_DOWN )
    {
      tmpHours = tmpHours > 0 ? tmpHours - 1 : tmpHours;
      lcd.setCursor(11, 1);
      lcd.print("  ");
      lcd.setCursor(11, 1);
      if ( tmpHours < 10 ) lcd.print(" ");
      lcd.print(tmpHours);
      timeRef = millis();
    }
    else if ( button == KEYPAD_SELECT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;



      timeOut = false;
      break;
    }
    delay(150);
  }

  if ( !timeOut ) {
    // Get the current time and date from the chip.
    Time t = rtc.time();

    t.hr = tmpHours;
    // Set the time and date on the chip.
    rtc.halt(false);
    rtc.time(t);
    transition(KEYPAD_SELECT);
  }
  else transition(TIME_OUT);
}




void setMinutes()
{
  unsigned long timeRef;
  boolean timeOut = true;
  uint8_t tmpMinutes = 0;

  lcd.clear();
  lcd.print("Time");

  Time t = rtc.time();
  tmpMinutes = t.min;

  timeRef = millis();
  lcd.setCursor(0, 1);
  lcd.print("Set minutes: ");
  lcd.print(tmpMinutes);

  while ( (unsigned long)(millis() - timeRef) < 5000 )
  {
    uint8_t button = lcd.button();

    if ( button == KEYPAD_UP )
    {
      tmpMinutes = tmpMinutes < 59 ? tmpMinutes + 1 : tmpMinutes;
      lcd.setCursor(13, 1);
      lcd.print("  ");
      lcd.setCursor(13, 1);
      if ( tmpMinutes < 10 ) lcd.print(" ");
      lcd.print(tmpMinutes);
      timeRef = millis();
    }
    else if ( button == KEYPAD_DOWN )
    {
      tmpMinutes = tmpMinutes > 0 ? tmpMinutes - 1 : tmpMinutes;
      lcd.setCursor(13, 1);
      lcd.print("  ");
      lcd.setCursor(13, 1);
      if ( tmpMinutes < 10 ) lcd.print(" ");
      lcd.print(tmpMinutes);
      timeRef = millis();
    }
    else if ( button == KEYPAD_SELECT )
    {
      while ( lcd.button() != KEYPAD_NONE ) ;
      timeOut = false;
      break;
    }
    delay(150);
  }

  if ( !timeOut )
  {

    // Get the current time and date from the chip.
    Time t = rtc.time();

    t.min = tmpMinutes;
    // Set the time and date on the chip.
    rtc.halt(false);
    rtc.time(t);

    transition(KEYPAD_SELECT);
  }
  else transition(TIME_OUT);
}

