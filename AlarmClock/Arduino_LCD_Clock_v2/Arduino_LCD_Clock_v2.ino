/*

ArduinoLCDclockV2
by
Kevin Rye 2013/03/10
http://kevinrye.net/

These works are licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License

V1 - original release
V2 - added new setting menu/gui
     added 12/24 hour operation
     changed minimum settable year to 2013
     added feature to quickly reset seconds to '00' by pressing the down button
*/


#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

int seconds; 
int minutes; 
int hours;
int adjHours;
int dayOfWeek;
int dayOfMonth;
int month;
int year;

int nextButton = 2;
int upButton = 3;
int downButton = 4;

int buttonCount = 0; //start at zero starts clock in run mode

bool display12HrMode;
int address = 0;
byte savedMode;

void setup()
{
  pinMode (nextButton, INPUT);
  pinMode (upButton, INPUT);  
  pinMode (downButton, INPUT);

  Wire.begin();
  lcd.begin(16, 2);
  Serial.begin(9600);

  ////////////////////////////////
  /* force time setting:
   seconds = 00;
   minutes = 00;
   hours = 00;
   dayOfWeek = 1;
   dayOfMonth = 01;
   month = 01;
   year = 13;
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

  //V
  byte customCharV[8] = {
    0b00000,
    0b00000,
    0b11011,
    0b11011,
    0b11011,
    0b11111,
    0b01110,
    0b00100
  };

  //2
  byte customChar2[8] = {
    0b01110,
    0b11111,
    0b11011,
    0b00011,
    0b00110,
    0b01100,
    0b11111,
    0b11111
  };

  //.
  byte customCharP[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00110,
    0b00110
  };

  lcd.createChar(0, customCharL);
  lcd.createChar(1, customCharC);
  lcd.createChar(2, customCharD);
  lcd.createChar(3, customCharO);
  lcd.createChar(4, customCharK);
  lcd.createChar(5, customCharV);
  lcd.createChar(6, customChar2);
  lcd.createChar(7, customCharP);

  lcd.setCursor(0,0);

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
  lcd.print(" "); // print space 
  lcd.write((uint8_t)5);  //V
  lcd.write((uint8_t)6);  //2
  lcd.write((uint8_t)7);   //.
  lcd.write((uint8_t)3);   //0

  lcd.setCursor(0,1); //move cursor to start of next line
  lcd.print("Kevin Rye - 2012");
  delay(3000);
  lcd.clear(); // clear LCD screen

  //read 12/24 hr setting from EEPROM
  savedMode = EEPROM.read(address); 
  if(savedMode == 0) { //0 = 12 hr mode //1 = 24 hr mode
    display12HrMode = true; //savedMode value stored in EEPRMOM is 0 , so set clock to display 12 hr mode
    Serial.print("time mode is 12 hr mode");
  } 
  else {
    display12HrMode = false; //savedMode value stored in EEPRMOM is 1 , so set clock to display 24 hr mode
    Serial.print("time mode is 24 hr mode");
  }
}


void loop()
{
  //poll the DS3231 for the date and time
  get_time();
  get_date();

  lcd.clear(); // clear LCD screen    //do I need this? is this causing the 'blink'?
  lcd.setCursor(0,0);

  /////////////////////////////////////////////////////

  //display set menus if nextButton is pressed
  if (digitalRead(nextButton) == HIGH) {

    buttonCount = buttonCount + 1;

    if (buttonCount == 7) {
      buttonCount = 0;
    }
    delay(100);
  }

  /////////////////////////////////////////////////////

  if(buttonCount == 0) {
    //first line - TIME
    if (display12HrMode == false) {  //display 24 hour format

      //display hours
      if(hours < 10) {
        lcd.print("    0");
      } 
      else {
        lcd.print("    ");
      }

      lcd.print(hours);
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
    }
    else {        //display 12 hour format

      //display hours
      switch(hours){
    case 0: 
      lcd.print("  12");
      break;
    case 1: 
      lcd.print("   1");
      break;
    case 2: 
      lcd.print("   2");
      break;
    case 3: 
      lcd.print("   3");
      break;
    case 4: 
      lcd.print("   4");
      break;
    case 5: 
      lcd.print("   5");
      break;
    case 6: 
      lcd.print("   6");
      break;
    case 7: 
      lcd.print("   7");
      break;
    case 8: 
      lcd.print("   8");
      break;
    case 9: 
      lcd.print("   9");
      break;
    case 10: 
      lcd.print("  10");
      break;
    case 11: 
      lcd.print("  11");
      break;
    case 12: 
      lcd.print("  12");
      break;
    case 13: 
      lcd.print("   1");
      break;
    case 14: 
      lcd.print("   2");
      break;
    case 15: 
      lcd.print("   3");
      break;
    case 16: 
      lcd.print("   4");
      break;
    case 17: 
      lcd.print("   5");
      break;
    case 18: 
      lcd.print("   6");
      break;
    case 19: 
      lcd.print("   7");
      break;
    case 20: 
      lcd.print("   8");
      break;
    case 21: 
      lcd.print("   9");
      break;
    case 22: 
      lcd.print("  10");
      break;
    case 23: 
      lcd.print("  11");
      break;
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
      
      else if (hours = 0) {
        lcd.print(" AM");
      }
      else {
        lcd.print(" PM");
      }
    }
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


  //hours
  if (buttonCount == 1) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("HOURS:");
    lcd.setCursor(0,1);
    
    if (display12HrMode == false) {  //display 24 hour format
      if (hours < 10) {
        lcd.print("0");
      }
      lcd.print(hours);
    }

    else { //in 12 hr mode
       switch(hours){
    case 0: 
      lcd.print("12 AM");
      break;
    case 1: 
      lcd.print("1 AM");
      break;
    case 2: 
      lcd.print("2 AM");
      break;
    case 3: 
      lcd.print("3 AM");
      break;
    case 4: 
      lcd.print("4 AM");
      break;
    case 5: 
      lcd.print("5 AM");
      break;
    case 6: 
      lcd.print("6 AM");
      break;
    case 7: 
      lcd.print("7 AM");
      break;
    case 8: 
      lcd.print("8 AM");
      break;
    case 9: 
      lcd.print("9 AM");
      break;
    case 10: 
      lcd.print("10 AM");
      break;
    case 11: 
      lcd.print("11 AM");
      break;
    case 12: 
      lcd.print("12 PM");
      break;
    case 13: 
      lcd.print("1 PM");
      break;
    case 14: 
      lcd.print("2 PM");
      break;
    case 15: 
      lcd.print("3 PM");
      break;
    case 16: 
      lcd.print("4 PM");
      break;
    case 17: 
      lcd.print("5 PM");
      break;
    case 18: 
      lcd.print("6 PM");
      break;
    case 19: 
      lcd.print("7 PM");
      break;
    case 20: 
      lcd.print("8 PM");
      break;
    case 21: 
      lcd.print("9 PM");
      break;
    case 22: 
      lcd.print("10 PM");
      break;
    case 23: 
      lcd.print("11 PM");
      break;
    }
      
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


  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //pressing 'up' button should increment field being set
  //hours
  if ((digitalRead(upButton) == HIGH) && (buttonCount == 1)) {

    hours++; 
    if (hours > 23) hours = 0;

    //send update to RTC 
    set_time();
  }

  //mins
  else if ((digitalRead(upButton) == HIGH) && (buttonCount == 2)) {   

    minutes++;              
    if (minutes > 59) minutes = 0;

    //reset seconds to '0'
    seconds = 0;

    //send update to RTC
    set_time();
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


  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //pressing 'down' button should decrement field being set
  //hours
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 1)) {
    
   if (hours == 0) hours = 23;    
    else if ((hours > 0) && (hours <= 23)) hours--;

    //send update to RTC 
    set_time();
  }

  //mins
  else if ((digitalRead(downButton) == HIGH) && (buttonCount == 2)) {   

    if (minutes == 0) minutes = 59;    
    else if ((minutes > 0) && (minutes <= 59)) minutes--; 

    //reset seconds to '0'
    seconds = 0;

    //send update to RTC
    set_time();
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
    if (year < 13) year = 13;

    //send update to RTC
    set_date();
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //button error handling
  if ((digitalRead(upButton) == HIGH) && (digitalRead(downButton) == HIGH) && (buttonCount != 0)) {

    lcd.setCursor(0,1);
    lcd.print("BOOM! ");

    delay(1000);
  }


  ////////////////////////////////////////
  //set seconds to '00' shortcut
  if ((digitalRead(downButton) == HIGH) && (buttonCount == 0)) {

    //reset seconds to '0'
    seconds = 0;

    //send update to RTC
    set_time();    
  }


  //toggle 12/24 mode
  if ((digitalRead(upButton) == HIGH) && (buttonCount == 0)) {     

    //toggle 12/24 mode
    display12HrMode = !display12HrMode;

    if (display12HrMode == false) {

      savedMode = 1; //24 hr mode
      //send update to EEPROM for 24 hr mode
      EEPROM.write(address, savedMode); //this sets the savedMode in EEPROM to true //24 hr mode

    } 

    else {
      savedMode = 0; //12 hr mode
      //send update to EEPROM for 12 hour mode
      EEPROM.write(address, savedMode); //this sets the savedMode in EEPROM to fasle //12 hr mode

    }
  }


} //end of loop









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
















