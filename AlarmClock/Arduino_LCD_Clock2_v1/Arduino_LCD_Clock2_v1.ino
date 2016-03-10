/*
Arduino_LCD_Clock2
by
Kevin Rye 2014/07/10
http://kevinrye.net/

These works are licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License

V1 - original release


*/
#include <Wire.h>
#include "DS3231.h"
#include <LiquidCrystal.h>
#include <Tone.h>

Tone tone1;
Tone notePlayer[1];

#define OCTAVE_OFFSET 0

int notes[] = { 
  0,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
  NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7
};
     
char *song = "StarWars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6";
#define isdigit(n) (n >= '0' && n <= '9')

DS3231 RTC; //Create the DS3231 object

LiquidCrystal lcd(7, 8, 12, 11, 10, 9);

int seconds; 
int minutes; 
int hours;
int adjHours;

int dayOfWeek;
int dayOfMonth;
int month;
int year;

int tempC;
int tempF;

int functionButton = 2;
int upButton = 3;
int downButton = 4;

int alarmButton = 5;
int alarmLED = 6;
bool setAlarmActive = false; //set up default condition
bool inAlarmMenu = false; //set up default condition
int alarmHour = 0; //set up default condition
int adjAlarmHour;
int alarmMinute = 0; //set up default condition
int alarmIsActive = false;
//audio
int audioOut = A3;

int buttonCount = 0; //start at zero starts clock in run mode

void setup()
{

  pinMode (functionButton, INPUT);
  pinMode (upButton, INPUT);  
  pinMode (downButton, INPUT);
  pinMode (alarmButton, INPUT);
  pinMode (alarmLED, OUTPUT);
  pinMode (audioOut, OUTPUT);

  Serial.begin(57600);
  Wire.begin();
  lcd.begin(16, 2);
  RTC.begin();

  //audio
  tone1.begin(A3);
 notePlayer[0].begin(A3);
 
  ////////////////////////////////
  // force time setting:
  /*
  seconds = 00;
   minutes = 16;
   hours = 17; //in 24 hour mode
   dayOfWeek = 3; //1 is Sunday
   dayOfMonth = 27;
   month = 05;
   year = 14;
   initChrono();//just set the time once on your RTC
   */
  ///////////////////////////////

  lcd.begin(16, 2); // tells Arduino the LCD dimensions

  //L
  byte customCharL[8] = {
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11111,
    0b11111
  };

  //C
  byte customCharC[8] = {
    0b01111,
    0b11111,
    0b11000,
    0b11000,
    0b11000,
    0b11000,
    0b11111,
    0b01111
  };

  //D
  byte customCharD[8] = {
    0b11110,
    0b11111,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11111,
    0b11110
  };

  //O
  byte customCharO[8] = {
    0b01110,
    0b11111,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11111,
    0b01110
  };

  //K
  byte customCharK[8] = {
    0b11011,
    0b11011,
    0b11110,
    0b11100,
    0b11100,
    0b11110,
    0b11011,
    0b11011
  };

  lcd.createChar(0, customCharL);
  lcd.createChar(1, customCharC);
  lcd.createChar(2, customCharD);
  lcd.createChar(3, customCharO);
  lcd.createChar(4, customCharK);

  lcd.setCursor(2,0);

  lcd.print(" "); // print space 
  lcd.write((uint8_t)0);  //L
  lcd.write((uint8_t)1);  //C
  lcd.write((uint8_t)2);  //D
  lcd.print(" "); // print space 
  lcd.write((uint8_t)1);  //C
  lcd.write((uint8_t)0);  //L
  lcd.write((uint8_t)3);  //O
  lcd.write((uint8_t)1);  //C
  lcd.write((uint8_t)4);  //K

  lcd.setCursor(0,1); //move cursor to start of next line
  lcd.print("Kevin Rye - 2014");
  delay(3000);
  lcd.clear(); // clear LCD screen

}




void loop() {

  //poll the DS3231 for the date and time
  get_time(); 
  get_date();

  tempC = RTC.getTemperature();
  tempF = (tempC * 1.8) + 32.0; // Convert Celcius to Fahrenheit

  lcd.clear(); // clear LCD screen    //do I need this? is this causing the 'blink'?
  lcd.setCursor(0,0);

  Serial.print("adj hours:");
  Serial.print(adjHours);
  Serial.println(" ");
  Serial.print("mins:");
  Serial.print(minutes);
  Serial.println(" ");

  Serial.print("alarm adj hours:");
  Serial.print(adjAlarmHour);
  Serial.println(" ");

  Serial.print("alarm minutes:");
  Serial.print(alarmMinute);
  Serial.println(" ");

  /////////////////////////////////////////////////////
  Serial.print("button count:");
  Serial.println(buttonCount);
  Serial.println(" ");

  //display function menu if functionButton is pressed
  if (digitalRead(functionButton) == HIGH) {

    inAlarmMenu = false;

    buttonCount = buttonCount + 1;

    if (buttonCount > 8) {
      buttonCount = 0;
    }

    delay(100);
  }

  //display alarm menu if alarmButton is pressed
  if (digitalRead(alarmButton) == LOW) { //LOW when pressed (wired differently than the other buttnons by accident)

    if (buttonCount == 20) {
      buttonCount = 0;
      inAlarmMenu = false;
    } 
    else {
      buttonCount = 20;
      inAlarmMenu = true;
    }
  }

  /////////////////////////////////////////////////////

  if(buttonCount == 0) {
    //first line - TIME
    //pass RTC values into a new variable -
  //this way one variable stores time in 24 hour mode and another that can be converted to 12 hour mode
  //time is monitored 'in the background' in 24 hour mode using the RTC, but is displayed in an adjusted 12-hr format
  adjHours = hours;

  //convert to 12 hr mode for display
  if (hours < 1) {
    adjHours = 12;
  }

  if ((hours > 12) && (hours < 24)) {
    adjHours = hours - 12;
  }
    
        if (adjHours <10) {
        lcd.print(" ");
        lcd.print(adjHours);
      } 
      else {
        lcd.print(adjHours);
      }

    lcd.print(":");

    //display minutes
    if(minutes < 10)
    {
      lcd.print("0");
    }
    lcd.print(minutes);
    lcd.print(":");

    //display seconds
    if(seconds < 10) {
      lcd.print("0");
    }
    lcd.print(seconds);

    //display AM/PM
    if(hours < 12) {
      lcd.print(" AM");
    } 
    else {
      lcd.print(" PM");
    }

    //temp
    lcd.print(" ");
    lcd.print(tempF);
    lcd.print("F");

    ////////////////////////////////////////////////////
    //second line - date
    //display day of week
    lcd.setCursor(0, 1);
    lcd.print("  ");
    switch(dayOfWeek){
    case 1: 
      lcd.print("Sun");
      break;
    case 2: 
      lcd.print("Mon");
      break;
    case 3: 
      lcd.print("Tue");
      break;
    case 4: 
      lcd.print("Wed");
      break;
    case 5: 
      lcd.print("Thu");
      break;
    case 6: 
      lcd.print("Fri");
      break;
    case 7: 
      lcd.print("Sat");
      break;
    }
    lcd.print(" ");

    //display month
    if(month < 10){
      lcd.print("0");
    }  

    lcd.print(month);
    lcd.print("/");
    if(dayOfMonth < 10) {
      lcd.print("0"); 
    }

    lcd.print(dayOfMonth); 
    lcd.print("/"); 

    //display year
    if(year < 10){
      lcd.print("0");
    } 
    lcd.print(year); 

    delay(200);
}

  //////////////////////////enter function menu
  //hours
  if (buttonCount == 1) {

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("HOURS:");
    lcd.setCursor(0,1);

if(adjHours > 12) {
  adjHours = adjHours - 12;
}
if(adjHours == 0) {
  adjHours = 12;
}
  lcd.print(adjHours);

Serial.print("adjustHours:");
Serial.print(adjHours);
Serial.print(" ");

      if (hours < 12) {
        lcd.print(" AM");
      } 
      else {
        lcd.print(" PM");
      }

      delay(200);
  }
  
  //mins
  if (buttonCount == 2) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MINUTES:");
    lcd.setCursor(0,1);
    if(minutes < 10) {
      lcd.print("0");
    }
    lcd.print(minutes);
    delay(200);
  }

  //day of week
  if (buttonCount == 3) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("DAY OF WEEK:");
    lcd.setCursor(0,1);
    switch(dayOfWeek){
    case 1: 
      lcd.print("Sun");
      break;
    case 2: 
      lcd.print("Mon");
      break;
    case 3: 
      lcd.print("Tue");
      break;
    case 4: 
      lcd.print("Wed");
      break;
    case 5: 
      lcd.print("Thu");
      break;
    case 6: 
      lcd.print("Fri");
      break;
    case 7: 
      lcd.print("Sat");
      break;
    }
    delay(200);
  }

  //month
  if (buttonCount == 4) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MONTH:");
    lcd.setCursor(0,1);
    if(month < 10) {
      lcd.print("0");
    }
    lcd.print(month);
    delay(200);
  }

  //day of month
  if (buttonCount == 5) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("DAY OF MONTH:");
    lcd.setCursor(0,1);
    if(dayOfMonth < 10) {
      lcd.print("0");
    }
    lcd.print(dayOfMonth);
    delay(200);
  }

  //year
  if (buttonCount == 6) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("YEAR:");
    lcd.setCursor(0,1);
    lcd.print("20");
    lcd.print(year);
    delay(200);
  }

  //alarm hours
  if (buttonCount == 7) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SET ALARM HOURS:");
    lcd.setCursor(0,1);

    adjAlarmHour = alarmHour;

  if(adjAlarmHour > 12) {
  adjAlarmHour = alarmHour - 12;
}
if(adjAlarmHour == 0) {
  adjAlarmHour = 12;
}
  lcd.print(adjAlarmHour);

Serial.print("adjAlarmHour:");
Serial.print(adjAlarmHour);
Serial.print(" ");

      if (alarmHour < 12) {
        lcd.print(" AM");
      } 
      else {
        lcd.print(" PM");
      }
      
    delay(200);
  }

  //alarm mins
  if (buttonCount == 8) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("SET ALARM MINS:");
    lcd.setCursor(0,1);
    if(alarmMinute < 10) {
      lcd.print("0");
    }
    lcd.print(alarmMinute);
    delay(200);
  }

  //alarm menu button was pressed, display toggle for alarm
  if (buttonCount == 20) {
   lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("ALARM:");
   
     if ((alarmHour > 12) && (alarmHour < 24)) {
        adjAlarmHour = alarmHour - 12;
      }
      if (alarmHour == 0) {
        adjAlarmHour = 12;
      }
       lcd.print(adjAlarmHour);
            
    lcd.print(":");

    //display alarm minutes
    if(alarmMinute < 10)    {
      lcd.print("0");
    }
    lcd.print(alarmMinute);

    //display AM/PM //herehere
    if(alarmHour < 12) {
      lcd.print(" AM");
    } 
    
    else {
      lcd.print(" PM");
    }
    
    lcd.setCursor(0,1);
    if(setAlarmActive == false) {
      lcd.print("OFF");
    } 
    else {
      lcd.print("ON");
    }
    delay(200);
  } 

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //pressing 'up' button should increment field being set when in functions menu
  //hours for time
  if ((digitalRead(upButton) == HIGH) && (buttonCount == 1)) {

    hours++; 
    if (hours > 23) hours = 0;

  adjHours = hours;
  
    //send update to RTC 
    set_time();
    
    delay(200);
  }

  //hours for alarm
  if ((digitalRead(upButton) == HIGH) && (buttonCount == 7)) {

    alarmHour++; 
    if (alarmHour > 23) alarmHour = 0;

adjAlarmHour = alarmHour;
  }

  //mins for time
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 2)) {   

    minutes++;              
    if (minutes > 59) minutes = 0;

    //reset seconds to '0'
    seconds = 0;

    //send update to RTC
    set_time();
  }

  //mins for alarm 
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 8)) {   

    alarmMinute++;              
    if (alarmMinute > 59) alarmMinute = 0;

  }

  //day of week
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 3)) {   

    dayOfWeek++;
    if(dayOfWeek > 7) dayOfWeek = 1;

    //send update to RTC
    set_date();
  }

  //month
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 4)) {

    month++;              
    if (month > 12) month = 1;

    //send update to RTC
    set_date();
  }

  //day of month
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 5)) {

    dayOfMonth++;              

    //if feb
    if (month == 2) {
      if (dayOfMonth > 28) dayOfMonth = 1;
    }

    //if leap year
    //still to do

    //if month has 30 days: Apr, Jun, Sep, Nov
    if ((month == 4) || (month == 6) || (month == 9) || (month == 11))  {
      if (dayOfMonth > 30) dayOfMonth = 1; 
    }

    //if month has 31 days: Jan, Mar, May, Jul, Aug, Oct, Dec
    if ((month == 1) || (month == 3) || (month == 5) || (month == 7) || (month == 8) || (month == 10)) {
      if (dayOfMonth > 31) dayOfMonth = 1;
    }

    //send update to RTC
    set_date();
  }

  //year
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 6)) {

    year++;              

    //send update to RTC
    set_date();
  }

  //toggle alarm when in alarm menu
  if ((digitalRead(upButton) == HIGH) && (buttonCount == 20)) {
    setAlarmActive = !setAlarmActive; 
 
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //pressing 'down' button should decrement field being set
  //hours for time
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 1)) {

    if (hours == 0) hours = 23;    
    else if ((hours > 0) && (hours <= 23)) hours--;

  adjHours = hours;

    //send update to RTC 
    set_time();
  }

  //hours for alarm 
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 7)) {

    if (alarmHour == 0) alarmHour = 23;    
    else if ((alarmHour > 0) && (alarmHour <= 23)) alarmHour--;

  adjAlarmHour = alarmHour;
 
  }

  //mins for time
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 2)) {   

    if (minutes == 0) minutes = 59;    
    else if ((minutes > 0) && (minutes <= 59)) minutes--; 

    //reset seconds to '0'
    seconds = 0;

    //send update to RTC
    set_time();
  }

  //mins for alarm 
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 8)) {   

    if (alarmMinute == 0) alarmMinute = 59;    
    else if ((alarmMinute > 0) && (alarmMinute <= 59)) alarmMinute--; 

  }

  //day of week
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 3)) {   

    dayOfWeek--;
    if(dayOfWeek < 1 ) dayOfWeek = 7;                                            

    //send update to RTC
    set_date();
  }

  //month
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 4)) {

    month--;              
    if (month < 1) month = 12;

    //send update to RTC
    set_date();
  }

  //day of month
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 5)) {

    dayOfMonth--;              

    //if feb
    if (month == 2) {
      if (dayOfMonth < 1) dayOfMonth = 28;
    }

    //if leap year
    //still to do

    //if month has 30 days: Apr, Jun, Sep, Nov
    if ((month == 4) || (month == 6) || (month == 9) || (month == 11))  {
      if (dayOfMonth < 1) dayOfMonth = 30; 
    }

    //if month has 31 days: Jan, Mar, May, Jul, Aug, Oct, Dec
    if ((month == 1) || (month == 3) || (month == 5) || (month == 7) || (month == 8) || (month == 10)) {
      if (dayOfMonth < 1) dayOfMonth = 31;
    }

    //send update to RTC
    set_date();
  }

  //year
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 6)) {

    year--;              
    if (year < 14) year = 14;

    //send update to RTC
    set_date();
  }

  //toggle alarm when in alarm menu
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 20)) {
    setAlarmActive = !setAlarmActive; 
 
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //button handlers
  //set seconds to '00' shortcut
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 0)) { //there's a bug in here. When time is in 12hr mode, pressing this knocks the clock from PM to AM.

    //reset seconds to '0'
    seconds = 0;

    //send update to RTC
    zeroSeconds();    
  }


  //up/down button should toggle alarm setting when in the alarm menu
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 12)) {

    setAlarmActive = !setAlarmActive;
  }

  if ((digitalRead(upButton) == HIGH) && (buttonCount == 12)) {

    setAlarmActive = !setAlarmActive;
  }

  //////////////////////////////////////////////////////////////
  //alarm 

  //turn on LED if alarm is set
  if (setAlarmActive == true) {
    digitalWrite(alarmLED, HIGH);
  } 
  else {
    digitalWrite(alarmLED, LOW);
  }

//////alarm
  //alarm only when in normal mode and when alarm is active
  if((adjAlarmHour == adjHours) && (alarmMinute == minutes) && (seconds < 1)) {

    if ((setAlarmActive == true) && (buttonCount == 0) ) {
 
      alarmIsActive = true;
      Serial.println("alarm active");
      play_rtttl(song);
    } 
  } else {
    alarmIsActive = false;
  }

///chime on the hour
//chime only when in normal mode and when not alarming
  if((minutes == 0) && (seconds < 1) && (buttonCount == 0) && (alarmIsActive == false) ) {

      Serial.println("chime active");
      playSimpleTone();
     
  }


} //end of loop



//from tone library for alarm
void play_rtttl(char *p)
{
  
  byte default_dur = 4;
  byte default_oct = 6;
  int bpm = 63;
  int num;
  long wholenote;
  long duration;
  byte note;
  byte scale;

  // format: d=N,o=N,b=NNN:
  // find the start (skip name, etc)

  while(*p != ':') p++;    // ignore name
  p++;                     // skip ':'

  // get default duration
  if(*p == 'd')
  {
    p++; 
    p++;              // skip "d="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    if(num > 0) default_dur = num;
    p++;                   // skip comma
  }

  Serial.print("ddur: "); 
  Serial.println(default_dur, 10);

  // get default octave
  if(*p == 'o')
  {
    p++; 
    p++;              // skip "o="
    num = *p++ - '0';
    if(num >= 3 && num <=7) default_oct = num;
    p++;                   // skip comma
  }

  Serial.print("doct: "); 
  Serial.println(default_oct, 10);

  // get BPM
  if(*p == 'b')
  {
    p++; 
    p++;              // skip "b="
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }
    bpm = num;
    p++;                   // skip colon
  }

  Serial.print("bpm: "); 
  Serial.println(bpm, 10);

  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / bpm) * 4;  // this is the time for whole note (in milliseconds)

  Serial.print("wn: "); 
  Serial.println(wholenote, 10);


  // now begin note loop
  while(*p)
  {
    // first, get note duration, if available
    num = 0;
    while(isdigit(*p))
    {
      num = (num * 10) + (*p++ - '0');
    }

    if(num) duration = wholenote / num;
    else duration = wholenote / default_dur;  // we will need to check if we are a dotted note after

      // now get the note
    note = 0;

    switch(*p)
    {
    case 'c':
      note = 1;
      break;
    case 'd':
      note = 3;
      break;
    case 'e':
      note = 5;
      break;
    case 'f':
      note = 6;
      break;
    case 'g':
      note = 8;
      break;
    case 'a':
      note = 10;
      break;
    case 'b':
      note = 12;
      break;
    case 'p':
    default:
      note = 0;
    }
    p++;

    // now, get optional '#' sharp
    if(*p == '#')
    {
      note++;
      p++;
    }

    // now, get optional '.' dotted note
    if(*p == '.')
    {
      duration += duration/2;
      p++;
    }

    // now, get scale
    if(isdigit(*p))
    {
      scale = *p - '0';
      p++;
    }
    else
    {
      scale = default_oct;
    }

    scale += OCTAVE_OFFSET;

    if(*p == ',')
      p++;       // skip comma for next note (or we may be at the end)

    // now play the note

    if(note)
    {
      Serial.print("Playing: ");
      Serial.print(scale, 10); 
      Serial.print(' ');
      Serial.print(note, 10); 
      Serial.print(" (");
      Serial.print(notes[(scale - 4) * 12 + note], 10);
      Serial.print(") ");
      Serial.println(duration, 10);
      tone1.play(notes[(scale - 4) * 12 + note]);
      delay(duration);
      tone1.stop();
    }
    else
    {
      Serial.print("Pausing: ");
      Serial.println(duration, 10);
      delay(duration);
    }
  }
}

//tone for chime on the hour
void playSimpleTone() {

        notePlayer[0].play(NOTE_A7);
        delay(300);
        notePlayer[0].stop();
        delay(100);
        notePlayer[0].play(NOTE_C8);
        delay(300);
        notePlayer[0].stop();
  delay(300); //delay to stop tones from playing twice
}

/////////////
//DS3231 RTC interface
void initChrono()
{
  set_time();
  set_date();
}

void set_date()
{
  Wire.beginTransmission(104);
  Wire.write(3);
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

void get_date()
{
  Wire.beginTransmission(104); 
  Wire.write(3);//set register to 3 (day)
  Wire.endTransmission();
  Wire.requestFrom(104, 4); //get 5 bytes(day,date,month,year,control);
  dayOfWeek   = bcdToDec(Wire.read());
  dayOfMonth  = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year  = bcdToDec(Wire.read());
}

void zeroSeconds() {
  Wire.beginTransmission(104);
  Wire.write(0);
  Wire.write(decToBcd(seconds));
  Wire.endTransmission();
}

void set_time()
{
  Wire.beginTransmission(104);
  Wire.write(0);
  Wire.write(decToBcd(seconds));
  Wire.write(decToBcd(minutes));
  Wire.write(decToBcd(hours));
  Wire.endTransmission();
}

void get_time()
{
  Wire.beginTransmission(104); 
  Wire.write(0);//set register to 0
  Wire.endTransmission();
  Wire.requestFrom(104, 3);//get 3 bytes (seconds,minutes,hours);
  seconds = bcdToDec(Wire.read() & 0x7f);
  minutes = bcdToDec(Wire.read());
  hours = bcdToDec(Wire.read() & 0x3f);
}





///////////////////////////////////////////////////////////////////////

byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}




//show alarm time in the alarm menu
//push time to cock if RTC not running



