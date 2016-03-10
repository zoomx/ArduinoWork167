//#ifdef statements like those below do not operate across separate files
//e.g. from the main .ino file to .h and .cpp files

#include "utility.h"
#include <EEPROM.h>
#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>
#include <utility/w5100.h>
#include <utility/socket.h>

//Enable this define when debugging emails
//#define EMAILDebug
//Enable this define when debugging the FileCache
//#define FileCacheDebug
//Enable this define when debugging the Heap
//heap stats are dumped to the serial monitor
//#define HeapDebug

#ifdef HeapDebug
boolean G_HeapDumpPC = false;
void HeapOptimisation();
#endif

//------------------------------------------------------------------------------
//NOTE THAT VARIABLES DEFINED HERE ARE STILL VISIBLE TO APPLICATION SKETCHES
//------------------------------------------------------------------------------

void UpdateSRAMHeapFreeStatistic(int p_value, int p_free_heap_list[]);
boolean UpdateSRAMStatistic(byte p_statistic, int p_value);

boolean MessageOpenSPIBA(File &p_msg_file);
String MessageReadSPIBA(File &p_msg_file, int p_msg_num);

boolean G_RAM_Checking;
boolean G_RAM_Checking_Start;
TypeRAMData G_RAMData[DC_RAMDataCount];

boolean G_SDCardOK;

boolean G_EthernetOK;
EthernetClient G_EthernetClient; //Used for the web site and emails out
//We hold EthernetClient connections open for two seconds
//to allow the write buffer to clear and get to the client
unsigned long G_EthernetClientTimer = 0;
boolean G_EthernetClientActive = false;

//Now the UDP NTP time server stuff
#define DC_NTP_PACKET_SIZE 48 // NTP time stamp is in the first 48 bytes of the message
byte G_PacketBuffer[DC_NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
EthernetUDP G_Udp;

unsigned long G_PrevMillis;
boolean G_UDPNTPOK = false;
int C_UDPNTPUpdateTime; //01:27AM
unsigned long G_NextUDPNTPUpdateTime = 0;

boolean G_Email_Active = true; //Loads from EEPROM
boolean G_SendTestEmail = false;
byte G_CallHierarchy[DC_CallHierarchyCount];
byte G_CallHierarchyIndex;
int G_HeapMaxFreeList[DC_HeapListCount];

//This variable is set everytime we get internet time using UDP NTP
//with an offset of current millis().
//The value might vary a little because of internet time read errors.
//It will jump forward whenever we get a 55 day rollover
long G_StartDatetime = 0;
boolean G_DaylightSavingTime = false;

unsigned long G_SocketConnectionTimers[MAX_SOCK_NUM];
unsigned long G_SocketConnectionTimes[MAX_SOCK_NUM];

//Set up the head and tail of the linked list cache
PCacheItem G_CacheHead;
PCacheItem G_CacheTail;
byte G_CacheCount; //count of records in the cache
int G_CacheSize; //size of records in the cache
boolean G_Hack_Flag; //Redirects hack HTML data to the HACKLOGS
int G_Web_Crawler_Index; //Redirects web crawler HTML data to the Crawler Logs
boolean G_CWZ_HTML_Req_Flag; //Redirects Catweazle HTML data to the Catweazle Logs

PHTMLParm G_HTMLParmListHead;

String G_Website;

char C_Cache_Filename[] = "/CACHE.TXT";

unsigned int G_StatisticCounts[DC_StatisticsCount][2];
//unsigned int G_MonthlyStatCounts[DC_StatisticsCount];
//unsigned int G_DailyStatCounts[DC_StatisticsCount];
unsigned int G_StatActvMth[DC_StatisticsCount];
unsigned int G_StatActvDay[DC_StatisticsCount];

//------------------------------------------------------------------------------
//DATETIME FUNCTIONS
//------------------------------------------------------------------------------

void CheckMillisRollover() {
  const byte c_proc_num = 150;
  Push(c_proc_num);
//This may not be foolproof
//In the time between the rollover and when we reset the clock below using UDP NTP
//we may get incorrect date and time values based on the previous system
//start date time. i.e. We could be reporting datetime values about 50 days previous.

  unsigned long l_millis = 0;
  l_millis = millis();
  if (G_PrevMillis <= l_millis) {
	G_PrevMillis = l_millis;
	Pop(c_proc_num);
	return;
  }

  ActivityWrite(Message(M_Processor_Time_Rollover_5));
  G_UDPNTPOK = ReadTimeUDP(); //We error out if G_EthernetOk == false
  if (!G_UDPNTPOK) {
	//The email heading is automatically written to the activity log
	//The success or failure or the email is automatically written to the activity log
	//If UDP NTP did not update our system clock will have reset back 50 days
	//We carry on anyway having sent an email alert
	EmailInitialise(Message(M_UDP_NTP_Rollover_Failure_6));
	EmailLine(Message(M_Home_Automation_Datetime_Rollover_Error_7));
	EmailDisconnect();
	Pop(c_proc_num);
	return;
  }

  //Rollover all OK
  //G_StartDateTime has been reset
  G_PrevMillis = millis();
  ActivityWrite(EPSR(E_Date__35) + DateToString(Now()));
  ActivityWrite(EPSR(E_Time__36) + TimeToHHMMSS(Now()));

  //In CheckSecondsDelay we reset active timers when the rollover occurs
  //We will be left with some timing errors because each active timer will start again
  CheckRAM();
  Pop(c_proc_num);
} //CheckMillisRollover

//------------------------------------------------------------------------------

void SetDaylightSavingTime(boolean p_dst) {
  //CheckRAM not required
  G_DaylightSavingTime = p_dst;
} //SetDaylightSavingTime

//------------------------------------------------------------------------------

void SetStartDatetime(long p_startdatetime) {
  //CheckRAM not required
  G_StartDatetime = p_startdatetime;
}

//------------------------------------------------------------------------------

void SetStartDate(long p_startdate) {
  //Daylight saving
  //CheckRAM not required
  G_StartDatetime = G_StartDatetime % 100000; //Keep existing time (trims date)
  G_StartDatetime += DateExtract(String(p_startdate)); //DateExtract uses zerofill
} //SetStartDate

//------------------------------------------------------------------------------

void SetStartTime(long p_starttime) {
//Subtract one hour is daylight saving in operation
//We hold time in NZST, not NZDST
  //CheckRAM not required
  G_StartDatetime = long(G_StartDatetime / 100000) * 100000; //Keep existing date (trims time)
  //Serial.println(G_StartDatetime);
  long l_time = TimeExtract(String(p_starttime));
  //Serial.println(l_time);
  G_StartDatetime += l_time;
  //Serial.println(G_StartDatetime);
} //SetStartTime

//------------------------------------------------------------------------------

unsigned long Now() {
  //Add one hour if daylight saving is in operation
  //CheckRAM not required
  unsigned long l_now = millis()/6/6/24;
  //Serial.println(l_now);
  l_now += G_StartDatetime;
  if (G_DaylightSavingTime) {
	//Serial.println(l_now);
	l_now += long(100000 / 24);
	//Serial.println(l_now);
  }
  //Serial.println(l_now);
  return l_now;
} //Now

//------------------------------------------------------------------------------

unsigned long Time() {
  //Add one hour if daylight saving is in operation
  //CheckRAM not required
  unsigned long l_time = millis()/6/6/24;
  //Serial.println(l_now);
  l_time += G_StartDatetime;
  if (G_DaylightSavingTime) {
	//Serial.println(l_now);
	l_time += long(100000 / 24);
	//Serial.println(l_now);
  }
  //Serial.println(l_now);
  return l_time % 100000;
} //Time

//------------------------------------------------------------------------------

unsigned long DateTime(unsigned long p_millis) {
//Convert a (typically) timer millis value to its date and time
  //Add one hour if daylight saving is in operation
  //CheckRAM not required
  long l_datetime = p_millis/6/6/24;
  //Serial.println(l_datetime);
  l_datetime += G_StartDatetime;
  if (G_DaylightSavingTime) {
	//Serial.println(l_datetime);
	l_datetime += long(100000 / 24);
	//Serial.println(l_datetime);
  }
  //Serial.println(l_datetime);
  return l_datetime;
} //DateTime

//------------------------------------------------------------------------------

unsigned long Date() {
  //CheckRAM not required
  long l_date = long(Now() / 100000) * 100000; //Keep existing date (trims time)
  //Serial.println(l_now);
  return l_date;
} //Date

//------------------------------------------------------------------------------

long DateExtract(const String &p_date){
  //Parse a DDMMYYYY string into separate dd, mm, yyyy
  //Then encode it as a long date number multiplied by 100000
  //to provide space for a RHS integer time portion.
  const byte c_proc_num = 152;
  Push(c_proc_num);
  char tarray[9];
  p_date.toCharArray(tarray, sizeof(tarray));
  long l_long = atol(tarray);
  String l_date8 = String(ZeroFill(l_long,8));
  int l_dd, l_mm, l_yyyy;
  String l_string = String();
  //Serial.println(l_date8);

  //Serial.println(p_date);
  //Serial.println(l_long);
  //Serial.println(l_date8);
  l_string = l_date8.substring(0, 2);
  l_dd = l_string.toInt();
  l_string = l_date8.substring(2, 4);
  l_mm = l_string.toInt();
  l_string = l_date8.substring(4);
  l_yyyy = l_string.toInt();

  CheckRAM();
  Pop(c_proc_num);
  return DateEncode(l_dd, l_mm, l_yyyy);
} //DateExtract

//------------------------------------------------------------------------------

long DateEncode(int p_dd, int p_mm, int p_yyyy) {
  //Base zero is 31/12/2012
  //Formula from the user manual of the HP12C calculator
  const byte c_proc_num = 153;
  Push(c_proc_num);
  int l_yyyy, l_mth_adj, l_year_adj = 0;
  //Serial.println(p_dd);
  //Serial.println(p_mm);
  //Serial.println(p_yyyy);
  l_yyyy = p_yyyy - 2000;
  if (p_mm <= 2) {
	l_mth_adj = 0;
	l_year_adj = l_yyyy - 1;
  }
  else {
	l_mth_adj = int((0.4 * p_mm) + 2.3);
	l_year_adj = l_yyyy;
  }
  //For our long datetimes we need to multiple the date integer by 100000
  //to make room for the time portion on the RHS of the integer
  CheckRAM();
  Pop(c_proc_num);
  return ((365 * l_yyyy) + (31 * (p_mm-1)) + p_dd + int(l_year_adj/4) - l_mth_adj - 4748) * 100000;
} //DateEncode

//------------------------------------------------------------------------------

String DateToString(long p_datetime) {
  const byte c_proc_num = 154;
  Push(c_proc_num);
  int l_day, l_month, l_year = 0;
  DateDecode(p_datetime, &l_day, &l_month, &l_year);
  String l_result = String(ZeroFill(l_day,2) + "/" + ZeroFill(l_month,2) + "/" + ZeroFill(l_year,4));
  //Serial.println(l_result);
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //DateToString

//------------------------------------------------------------------------------

void DateDecode(long p_datetime, int *p_dd, int *p_mm, int *p_yyyy) {
//We use a long to record datetime
//Time is the last five digits, the date it the higher order digits before that
//01/01/2013 is Day 1 - No support before this date
//We may pass in a datetime long - but we only want the preceeding date portion
  const byte c_proc_num = 155;
  Push(c_proc_num);
  const word MonthDays[] = {0,31,59,90,120,151,181,212,243,273,304,334,365};

  //Serial.print("Datetime ");
  //Serial.println(p_datetime);
  long l_date = p_datetime / 100000;
  //Serial.print("Date ");
  //Serial.println(l_date);
  int l_year = int((l_date/365.25)) + 2013;
  //int l_year = int(((l_date-1)/365.25)) + 2013;
  //Serial.print("Year ");
  //Serial.println(l_year);
  int l_days = l_date - int((l_year-2013) * 365.25);
  //Serial.print("Days ");
  //Serial.println(l_days);

  int l_leap_day = 0;
  if (((l_year % 4) == 0) &&
	  (l_days  > 59)) {
	l_leap_day = 1;
  }
  //Serial.print("Leap Day ");
  //Serial.println(l_leap_day);
  //iterate through to find days for all previous months
  int l_month = 0;
  int l_day = 0;
  for (int l_index = 1; l_index < 13; l_index++) {
	if ((MonthDays[l_index] + l_leap_day) >= l_days) {
	  //So it is this month
	  l_month = l_index;
	  //Serial.print("Month ");
	  //Serial.println(l_month);
	  //Now calculate residual days in the month
	  l_day = l_days - (MonthDays[l_index-1] + l_leap_day);
	  //Serial.print("Day ");
	  //Serial.println(l_day);
	  break;
	}
  }
  *p_dd = l_day;
  *p_mm = l_month;
  *p_yyyy = l_year;
  CheckRAM();
  Pop(c_proc_num);
} //DateDecode

//------------------------------------------------------------------------------

long TimeExtract(const String &p_time){
  //Parse a HHMM (24 hour) time
  //Convert to a fraction of one day
  //We return an integer between 0 and 99999 to respresent
  //the portion of on date. Time is recorded ioin a datetime long
  //on the RHS of a datetime long (preceeding date part may be nil)
  const byte c_proc_num = 156;
  Push(c_proc_num);
  int l_hh, l_mm;
  String l_string;
  float l_time;
  String l_time4 = String(ZeroFill(p_time.toInt(),4));
  //Serial.println("");
  //Serial.println(l_time4);


  l_string = l_time4.substring(0, 2);
  l_hh = l_string.toInt();
  l_string = l_time4.substring(2);
  l_mm = l_string.toInt();

  //l_time = (float)((l_hh * 60) + l_mm) / (float)(24*60);
  //Serial.println(l_hh);
  //Serial.println(l_mm);

  l_time = float(((l_hh * 60) + l_mm)) /24 /60 * 100000;
  //Serial.println(l_time);
  CheckRAM();
  Pop(c_proc_num);
  return long(l_time + 1); //force rounding
} //TimeExtract

//------------------------------------------------------------------------------

String TimeToHHMM(long p_time) {
//Time is expressed as an integer between 0 and 999999
//It is the last five digits of a datetime long
//50000 (/100000 = 0.5) is 12:00 midday
  const byte c_proc_num = 157;
  Push(c_proc_num);
  long l_time = p_time % 100000; //strip the days part to get the time
  //Serial.print("l_time = ");
  //Serial.println(l_time);
  int l_mins = int(l_time * 24 * 6 /10000);
  int l_hh, l_mm;
  l_hh = l_mins / 60;
  l_mm = l_mins % 60;
  //Now format the return string
  String l_result = String(ZeroFill(l_hh,2) + ":" + ZeroFill(l_mm,2));
  //Serial.println(l_result);
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //TimeToHHMM

//------------------------------------------------------------------------------

void TimeDecode(long p_datetime, int *p_hh, int *p_mm) {
  //CheckRAM not required
  long l_time = p_datetime % 100000; //strip the days part to get the time
  int l_mins = int(l_time * 24 * 6 /10000);
  int l_hh, l_mm;
  l_hh = l_mins / 60;
  l_mm = l_mins % 60;
  *p_hh = l_hh;
  *p_mm = l_mm;
} //TimeDecode

//------------------------------------------------------------------------------

unsigned long TimeEncode(int p_hour, int p_min, int p_sec) {
  //CheckRAM not required
  long l_time = (p_hour * 3600) + (p_min * 60) + p_sec;
  l_time = long(float(l_time) * 100000 / (60*60*24));
  return l_time;
} //TimeEncode

//------------------------------------------------------------------------------

String TimeToHHMMSS(long p_time) {
//Time is expressed as an integer between 0 and 999999
//It is the last five digits of a datetime long
//50000 (/100000 = 0.5) is 12:00 midday
  const byte c_proc_num = 158;
  Push(c_proc_num);
  long l_time = p_time % 100000; //strip the days part to get the time
  //Serial.print("l_time = ");
  //Serial.println(l_time);
  long l_secs = long(l_time * 864 /1000);
  long l_hh, l_mm, l_ss;
  l_mm = l_secs / 60;
  l_ss = l_secs % 60;
  l_hh  = l_mm / 60;
  l_mm = l_mm % 60;
  //Now format the return string
  String l_result = String(ZeroFill(l_hh,2) + ":" +
						   ZeroFill(l_mm,2) + ":" +
						   ZeroFill(l_ss,2));
  //Serial.println(l_result);
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //TimeToHHMMSS

//------------------------------------------------------------------------------

int DayOfWeekNum(long p_datetime){ //Mon = 1, Sun = 7;
  //CheckRAM not required
  int l_date = p_datetime / 100000;
  return (l_date %7) + 1;
} //DayOfWeekNum

//------------------------------------------------------------------------------

String DayOfWeek(long p_datetime){
  const byte c_proc_num = 159;
  Push(c_proc_num);
  const String C_DOW = ("MONTUEWEDTHUFRISATSUN");
  int l_date = p_datetime / 100000;
  int l_dow = l_date % 7; //Monday will be zero
  CheckRAM();
  Pop(c_proc_num);
  return C_DOW.substring(l_dow * 3, (l_dow + 1) * 3);
}  //DayOfWeek

//----------------------------------------------------------------------------------------

boolean CheckSecondsDelay(unsigned long &p_AnyTime, unsigned long p_MilliSecDelay) {
//Make sure p_AnyTime is derived from millis()
//This proc does not work if p_AnyTime is a datetime variable derived from
//Now(), Date() or Time()
  //CheckRAM not required
  const byte c_proc_num = 151;
  Push(c_proc_num);
  if (p_AnyTime == 0) {
	CheckRAM();
	Pop(c_proc_num);
	return true;
  }
  unsigned long l_millis = millis();
  //Sometimes p_AnyTime is set at millis() + X to a forward time for an extended delay.
  //If millis() is an hour or more less than p_AnyTime we assume a 50 day time rollover
  if ((l_millis < p_AnyTime) &&
	  ((p_AnyTime - l_millis) > (C_OneSecond * 3600))) {
	p_AnyTime = 1; //restart the delay
  }
  if ((l_millis > p_AnyTime) && //Initially check past (possibly adjusted) "Start" time
								//Avoids negative unsigned arithmetic when "Start" adjusted forward.
								//Otherwise a negative "l_millis - p_AnyTime" would be a large positive number
	  ((l_millis - p_AnyTime) > p_MilliSecDelay)) {
	p_AnyTime = 0;
	CheckRAM();
	Pop(c_proc_num);
	return true;
  }
  CheckRAM();
  Pop(c_proc_num);
  return false;
}  //CheckSecondsDelay

//----------------------------------------------------------------------------------------
//EEPROM
//----------------------------------------------------------------------------------------

void SettingSwitch(const String &p_setting_desc, int p_setting) {
  const byte c_proc_num = 170;
  Push(c_proc_num);
  if (EPSR(p_setting) == "T") EPSW(p_setting,"F");
  else                        EPSW(p_setting,"T");
  ActivityWrite(String(p_setting_desc + EPSR(E__setting_switched_to__94) + EPSR(p_setting)));
  CheckRAM();
  Pop(c_proc_num);
} //SettingSwitch

//----------------------------------------------------------------------------------------

void EPSW(int p_start_posn, char p_string[] ) { //EEPROMStringWrite
  const byte c_proc_num = 171;
  Push(c_proc_num);
//Write a string to EEPROM and terminate it with a NULL
//We read first and do not rewrite bytes/chars that are unchanged

  for (int l_posn = 0; l_posn < (int) strlen(p_string); l_posn ++) {
	byte l_byte = (byte) p_string[l_posn];
	byte l_read = EEPROM.read(p_start_posn + l_posn);
	if (l_read != l_byte) {
	  EEPROM.write(p_start_posn + l_posn, l_byte);
	}
  }
  //write the NULL termination
  if (EEPROM.read(p_start_posn + strlen(p_string)) != 0)
	EEPROM.write(p_start_posn + strlen(p_string), 0);
  //
  CheckRAM();
  Pop(c_proc_num);
} //EPSW

//----------------------------------------------------------------------------------------

String EPSRU(int p_start_posn) { //EEPROMStringReadUpperCase
  String l_result = EPSR(p_start_posn);
  l_result.toUpperCase();
  return l_result;
} //EPSRU

//----------------------------------------------------------------------------------------

String EPSR(int p_start_posn) { //EEPROMStringRead
//Read a NULL terminated string from EEPROM
//Only strings up to 128 bytes are supported
  const byte c_proc_num = 172;
  Push(c_proc_num);
  byte l_byte;

  //Count first, reserve exact string length and then extract
  int l_posn = 0;
  while (true) {
	l_byte = EEPROM.read(p_start_posn + l_posn);
	if (l_byte == 0) {
	  break;
	}
	l_posn ++;
  }

  //Now extract the string
  String l_string = "";
  l_string.reserve(l_posn + 1);
  char l_char;
  l_posn = 0;
  while (true) {
	l_byte = EEPROM.read(p_start_posn + l_posn);
	if (l_byte == 0) {
	  break;
	}
	l_char = (char) l_byte;
	l_string += l_char;
	l_posn ++;
	if (l_posn == 128)
	  break;
	//
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_string;
} //EPSR

//----------------------------------------------------------------------------------------

String EPFR(int &p_start_posn, char p_delimiter) { //EEPROMFieldRead
//Read a field from a NULL terminated string in EEPROM
//Only strings up to 16 bytes are supported
//Similar to ENDF
//Return -1 when the end of the string is found
  const byte c_proc_num = 191;
  Push(c_proc_num);

  String l_string;
  l_string.reserve(17);
  byte l_byte;
  char l_char;
  byte l_length = 0;
  while (true) {
	l_byte = EEPROM.read(p_start_posn);

	if (l_byte == 0) {
	  p_start_posn = -1;
	  break;
	}

	l_char = (char) l_byte;
	if (l_char == p_delimiter) {
	  p_start_posn++; //Skip the delimiter
	  break;
	}

	l_string += l_char;
	p_start_posn ++;
	l_length++;
	if (l_length == 16)
	  break;
	//
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_string;
} //EPFR

//----------------------------------------------------------------------------------------
//MEMORY MANAGEMENT
//----------------------------------------------------------------------------------------

void CheckSRAMCheckingStart() {
  if (G_RAM_Checking == false)
	return; //do nothing
  //
  //SRAMChecking is enabled
  if (G_RAM_Checking_Start == false)
	return; //Start up not required
  //
  //RAM Checking not started yet - START IT
  ResetSRAMStatistics();
  G_RAM_Checking_Start = false;
  G_CallHierarchyIndex = -1;
} //CheckSRAMCheckingStart

//----------------------------------------------------------------------------------------

void Push(byte p_procedure_num) {
  if ((!G_RAM_Checking) || (G_RAM_Checking_Start))
	return;
  //
  if ((G_CallHierarchyIndex + 1) == DC_CallHierarchyCount) {//avoid overflow
/*
	String l_content;
	l_content.reserve(4 * DC_CallHierarchyCount);
	l_content = "";
	for (int l_idx = (DC_CallHierarchyCount - 1); l_idx >= 0; l_idx--)
	  l_content += String( G_CallHierarchy[l_idx]);
	//
	SendEmail(EPSR(E_Call_Hierarchy_Overflow_2608), l_content);

	G_RAM_Checking = false;
*/
	return;
  }
  G_CallHierarchyIndex++;
  G_CallHierarchy[G_CallHierarchyIndex] = p_procedure_num;
} //Push

//----------------------------------------------------------------------------------------

void Pop(byte p_proc_num) {
  if ((!G_RAM_Checking) || (G_RAM_Checking_Start))
	return;
  //
  if (G_CallHierarchyIndex == -1) { //avoid underflow
	//SendEmail(EPSR(E_Call_Hierarchy_Underflow_2632), String(EPSR(E_Procedure__2758) + p_proc_num));
	G_RAM_Checking = false; }
  else if (G_CallHierarchy[G_CallHierarchyIndex] == p_proc_num) //rollback
	G_CallHierarchyIndex--; //Last stack element is this proc
  else if (G_CallHierarchyIndex != (DC_CallHierarchyCount - 1)) { //Not stack top - an error
	//This proc may not be on the G_CallHierarchy stack if it reached maximum - that is OK
	//SendEmail(EPSR(E_Push_Pop_Error_2743), String(EPSR(E_Procedure__2758) + p_proc_num));
	G_RAM_Checking = false; }
  //

  //If G_CallHierarchy gets to the maximum record count push does not add new records.
  //In that we get here and return without error and disabling G_RAM_Checking.
} //Pop

//----------------------------------------------------------------------------------------

void ResetSRAMStatistics() {
  for (byte l_idx = 0; l_idx < DC_RAMDataCount; l_idx++) {
	for (byte l_idx2 = 0; l_idx2 < DC_CallHierarchyCount; l_idx2++)
	  G_RAMData[l_idx].CallHierarchy[l_idx2] = 0;
	//
	G_RAMData[l_idx].TimeOfDay     = 0;
	if (l_idx == DC_RAMMinFree) G_RAMData[l_idx].Value = 8192;
	else                        G_RAMData[l_idx].Value = 0;
	//G_RAMData[l_idx].HeapFreeList  = "";
	G_RAMData[l_idx].Updated = false;
  }
  for (byte l_index = 0; l_index < DC_HeapListCount; l_index++)
	G_HeapMaxFreeList[l_index] = 0;
  //

/*
  G_Min_RAM = false;
  G_Min_RAM_Free_Procedure = 0;
  G_Min_RAM_Free_Time = Now();
  G_Min_RAM_Free = 8192;

  G_Max_Heap = false;
  G_Max_Heap_Free_Procedure = 0;
  G_Max_Heap_Free_Time = Now();
  G_Max_Heap_Free = 0;
  G_Max_Heap_Free_List_Sizes = "";
*/
} //ResetSRAMStatistics

//----------------------------------------------------------------------------------------

int freeRam() {
  //If __brkval is zero nothing has been allocated on the heap - so use heap start
  //If __brkval is non zero it indicates the top of the heap
  extern int __heap_start, *__brkval;

  //Calculate the free RAM between the top of the heap and top of the stack
  //This new variable is on the top of the stack
  int l_total = 0;
  if (__brkval == 0)
	l_total = (int) &l_total - (int) &__heap_start;
  else
	l_total = (int) &l_total - (int) __brkval;
  //
  l_total -= sizeof(l_total); //Because free RAM starts after this local variable
  return l_total;
} //freeRam

//------------------------------------------------------------------------------

void  RamUsage(int &p_data_bss_size, int &p_heap_size, int &p_free_ram, int &p_stack_size, int &p_heap_free_size, int p_heap_list_sizes[]) {
//void RamUsage(int *p_data_size    , int *p_heap_size, int *p_stack_size, int *p_ram_free, int *p_heap_free, int p_heap_list_sizes[]) {

//PART I - work out the basic memory split
//Total memory is (int)&__stack - (int)&__data_start + 1;
//From bottom (low memory) to top (high memory) RAM divides as follows:
// ----------
// |  STACK | (Moves Down)
// |FREE RAM|
// |  HEAP  | (Moves Up - contains fragmented internal free space)
// |  DATA  | (Global Variables, Arrays, etc - fixed size)
// ----------
//DATA + BSS + H EA P>> (if any) + FREE + <<STACK
//"H EA P" indicates a heap with embedded free space.
//The HEAP moves up into FREE space and the STACK moves down into FREE space.
//If the two collide you are out of FREE ram and your program goes crazy.

  extern int __data_start, __stack,  __bss_end, __data_end, __heap_start, *__brkval, _end;

  p_data_bss_size = (int)&__bss_end - (int)&__data_start;

  int l_heap_end;
  //__brkval is a pointer
  if ((int)__brkval == 0)
	//If there are no heap records then the end of the heap is the start of the heap
	l_heap_end = (int)&__heap_start;
  else
	l_heap_end = (int)__brkval;
  //

  p_heap_size  = l_heap_end - (int)&__bss_end;
  p_free_ram   = (int)&l_heap_end - l_heap_end;
  p_stack_size = (int)&__stack - (int)&l_heap_end + 1;

  //PART II - Walk the HEAP free linked list
  byte l_heap_free_count = 0;
  AnalyseFreeHeap(p_heap_free_size, l_heap_free_count, p_heap_list_sizes);

} //RamUsage

//------------------------------------------------------------------------------

#ifdef HeapDebug
void HeapDump(const String p_posn) {
  //We dump heap stats to the serial monitor.
  //Only use when the mah Heap Size statistic is updated
  //This allows us to create a map of used and unused heap space in Excel
  extern int __bss_end, *__brkval, __heap_start;
  extern struct __freelist *__flp;
  struct __freelist {
	size_t sz;
	struct __freelist *nx;
  };
  struct __freelist *current;

  //Start of heap
  Serial.print((int)&__bss_end);
  if (p_posn == "")
	Serial.println(" S");
  else
	Serial.println(p_posn);
  //

  //List of free heap node
  int l_sum = 0;
  for (current = __flp; current; current = current->nx) {
	Serial.print((int) current);
	Serial.print(' ');
	Serial.println((int) current->sz + 2); // Add two bytes for the memory block's header
	l_sum += (int) current->sz + 2;
  }

  //End of heap
  int l_heap_end;
  //__brkval is a pointer
  if ((int)__brkval == 0)
	//If there are no heap records then the end of the heap is the start of the heap
	l_heap_end = (int)&__heap_start;
  else
	l_heap_end = (int)__brkval;
  //
 Serial.print(l_heap_end);
 Serial.print(" Sum ");
 Serial.println(l_sum);
 Serial.println();
} //HeapDump
#endif

//------------------------------------------------------------------------------

void AnalyseFreeHeap(int &p_heap_free_size, byte &p_heap_free_count, int p_heap_list_sizes[]) {
//We count how many blocks of free space are hiding within the heap.
//We calculate the size of total HEAP free space.

  extern struct __freelist *__flp;
  struct __freelist {
	size_t sz;
	struct __freelist *nx;
  };
  struct __freelist *current;

  //Reset our heap free list of the largest free list memory blocks
  if (p_heap_list_sizes) {
	for (int l_index = 0; l_index < DC_HeapListCount; l_index++)
	  p_heap_list_sizes[l_index] = 0;
	//
   }

  //Now walk the free list
  p_heap_free_size = 0;
  p_heap_free_count = 0;
  int l_size;
  for (current = __flp; current; current = current->nx) {
	p_heap_free_count++;
	l_size = (int) current->sz + 2; // Add two bytes for the memory block's header
	p_heap_free_size += l_size;

	//update our list of the largest heap free memory blocks
	if (p_heap_list_sizes) { //Can be NULL if the list is not required
	  for (int l_index = 0; l_index < DC_HeapListCount; l_index++) {
		if (p_heap_list_sizes[l_index] < l_size) { //Insert here - shuffle the list
		  for (int l_index2 = DC_HeapListCount - 1; l_index2 > l_index; l_index2--)
			p_heap_list_sizes[l_index2] = p_heap_list_sizes[l_index2 - 1];
		//
		  p_heap_list_sizes[l_index] = l_size;
		  break;
		} //insert here and break
	  } //iterate of free list to find the insertion point
	} //skip list update if no list provided
  } //for each heap free list node
} //AnalyseFreeHeap

//------------------------------------------------------------------------------

void CheckRAM() {
/*
struct TypeRAMData {
  String CallHierarchy;
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

  if ((!G_RAM_Checking) || (G_RAM_Checking_Start))
	return;
  //

  int l_data_size;
  int l_heap_size;
  int l_stack_size;
  int l_ram_free;
  int l_heap_free;
  int l_heap_list_sizes[DC_HeapListCount];

  RamUsage(l_data_size, l_heap_size, l_ram_free, l_stack_size, l_heap_free, l_heap_list_sizes);
  //Serial.println("Checkram1");
  UpdateSRAMStatistic(DC_RAMDataSize,l_data_size);
  UpdateSRAMStatistic(DC_RAMStackSize,l_stack_size);
  UpdateSRAMStatistic(DC_RAMHeapSize,l_heap_size);
  UpdateSRAMStatistic(DC_RAMMinFree,l_ram_free);
  UpdateSRAMHeapFreeStatistic(l_heap_free,l_heap_list_sizes);
  //Serial.println("Checkram2");
} //CheckRAM

//----------------------------------------------------------------------------------------

boolean UpdateSRAMStatistic(byte p_statistic, int p_value){
/*
#define DC_RAMDataSize    0  MAX
#define DC_RAMStackSize   1  MAX
#define DC_RAMHeapSize    2  MAX
#define DC_RAMMinFree     3  MIN
#define DC_RAMHeapMaxFree 4  MAX
*/
  //Serial.print("UpdateSRAMStatistic ");
  //Serial.println(p_statistic);
  boolean l_updated = false;
  if (p_statistic == DC_RAMMinFree) { //MIN
	if (G_RAMData[p_statistic].Value > p_value)
	  l_updated = true;
	//
  }
  else { //MAX
	if (G_RAMData[p_statistic].Value < p_value)
	  l_updated = true;
	//
  }

  if (!l_updated)
	return false;
  //

  //Serial.println("UpdateSRAMStatistic2 ");
  //Save the current call hierarchy for this statistic
  for (byte l_idx = 0; l_idx < DC_CallHierarchyCount; l_idx++) {
	if (l_idx <= G_CallHierarchyIndex)
	  G_RAMData[p_statistic].CallHierarchy[l_idx] = G_CallHierarchy[l_idx];
	else //overwrite any excess old call hierarchy values
	  G_RAMData[p_statistic].CallHierarchy[l_idx] = 0;
	//
  }

  G_RAMData[p_statistic].TimeOfDay = Now();
  G_RAMData[p_statistic].Value     = p_value;
  G_RAMData[p_statistic].Updated   = true;

  //Serial.println("UpdateSRAMStatistic3 ");
  return true;
} //UpdateSRAMStatistic

//----------------------------------------------------------------------------------------

void UpdateSRAMHeapFreeStatistic(int p_value, int p_free_heap_list[]){
  //We do not Push and Pop any procedure that is a decendant of CheckRam
  if (UpdateSRAMStatistic(DC_RAMHeapMaxFree,p_value)) {
	//G_RAMData[DC_RAMHeapMaxFree].HeapFreeList = p_free_heap_list;
	for (byte l_index = 0; l_index < DC_HeapListCount; l_index++)
	  G_HeapMaxFreeList[l_index] =  p_free_heap_list[l_index];
	//
#ifdef HeapDebug
	HeapDump(" Free Heap");
#endif


  }
} //UpdateSRAMHeapFreeStatistic

//----------------------------------------------------------------------------------------
//RAM USAGE REPORTING
//------------------------------------------------------------------------------

String DirectFileRecordRead(File &p_checkram_file, int p_record_length, int p_proc_num) {
/* This is the first five record (0-4) of an example file
setup***************************
loop****************************
CheckIfUDPNTPUpdateRequired*****
SDFileDeleteSPICSC**************
WebServerProcess****************
WebServerProcess2***************
LocalWebPage********************
*/
  const byte c_proc_num = 173;
  Push(c_proc_num);
  int l_current_SPI_device = CurrentSPIDevice();
  //Determine file length
  SPIDeviceSelect(DC_SDCardSSPin);
  unsigned long l_filesize = p_checkram_file.size();
  unsigned long l_posn = (p_proc_num - 1) * p_record_length;
  String l_result = "";
  if (l_posn >= l_filesize) {
	 l_result = String(p_proc_num + ' ' + EPSR(E_File_Record_Length_Error_172));
  }
  else {
	p_checkram_file.seek(l_posn);
	l_result = p_checkram_file.readStringUntil('*');
  }
  SPIDeviceSelect(l_current_SPI_device);
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //DirectFileRecordRead

//----------------------------------------------------------------------------------------------

String SRAMStatisticLabel(byte p_statistic) {
  const byte c_proc_num = 174;
  Push(c_proc_num);
  CheckRAM();
  if (p_statistic == 0) {
	Pop(c_proc_num);
	return EPSR(E_MAX_RAM_DATA_SIZE_101); }
  else if (p_statistic == 1) {
	Pop(c_proc_num);
	return EPSR(E_MAX_RAM_STACK_SIZE_102); }
  else if (p_statistic == 2) {
	Pop(c_proc_num);
	return EPSR(E_MAX_RAM_HEAP_SIZE_103); }
  else if (p_statistic == 3) {
	Pop(c_proc_num);
	return EPSR(E_MINIMUM_FREE_RAM_104);}
  else if (p_statistic == 4) {
	Pop(c_proc_num);
	return EPSR(E_MAXIMUM_FREE_HEAP_105);
  }
} //SRAMStatisticLabel

//----------------------------------------------------------------------------------------

void WriteSRAMStatisticSPICSC2(byte p_statistic) {
  const byte c_proc_num = 175;
  Push(c_proc_num);
/*
struct TypeRAMData {
  String CallHierarchy;
  long   TimeOfDay;
  int    Value;
  String HeapFreeList;
  boolean Updated;
};
#define DC_RAMDataSize    0
#define DC_RAMStackSize   1
#define DC_RAMHeapSize    2
#define DC_RAMMinFree     3
#define DC_RAMHeapMaxFree 4
#define DC_RAMDataCount   5
TypeRAMData G_RAMData[DC_RAMDataCount];
*/

//We don't activity log RAM statistics less that certain baseline values
//However all current statistics are available via the RAM Usage web page
//This reduces clutter in the daily activity log files.
  if ((p_statistic == DC_RAMStackSize) &&
	  (G_RAMData[p_statistic].Value < 400)) {
	Pop(c_proc_num);
	return;
  }
  if ((p_statistic == DC_RAMHeapSize) &&
	  (G_RAMData[p_statistic].Value < 2250)) {
	Pop(c_proc_num);
	return;
  }
  if ((p_statistic == DC_RAMMinFree) &&
	  (G_RAMData[p_statistic].Value > GC_MinFreeRAM)) {
	Pop(c_proc_num);
	return;
  }
  if ((p_statistic == DC_RAMHeapMaxFree) &&
	  (G_RAMData[p_statistic].Value < 1750)) {
	Pop(c_proc_num);
	return;
  }

  String l_message = SRAMStatisticLabel(p_statistic);
  l_message += EPSR(E_tbCheckPoint__164);

  String l_call_hierarchy = "";
  for (byte l_idx = 0; l_idx < DC_CallHierarchyCount; l_idx++) {
	if (G_RAMData[p_statistic].CallHierarchy[l_idx] == 0)
	  break;
	//
	if (l_idx != 0)
	  l_call_hierarchy += ", ";
	//
	l_call_hierarchy += String(G_RAMData[p_statistic].CallHierarchy[l_idx]);
  }
  l_message = String(l_message + l_call_hierarchy +
					 EPSR(E_tbTime__86) + TimeToHHMMSS(G_RAMData[p_statistic].TimeOfDay) +
					 EPSR(E_tbStatistic__107) + G_RAMData[p_statistic].Value); //Was EPSR(E__tMin_Free_RAM__1499)
  if (p_statistic == DC_RAMHeapMaxFree) {
	l_message += EPSR(E_tbFree_Heap_List__108);
	for (byte l_index = 0; l_index < DC_HeapListCount; l_index++) {
	  if (G_HeapMaxFreeList[l_index] == 0)
		break;
	  //
	  if (l_index != 0)
		l_message += ", ";
	  //
	  l_message += String(G_HeapMaxFreeList[l_index]);
	}
  }
  CheckRAM();
  ActivityWrite(l_message);
  Pop(c_proc_num);
} //WriteSRAMStatisticSPICSC2

//----------------------------------------------------------------------------------------

void WriteSRAMAnalysisChgsSPICSC3() {
  const byte c_proc_num = 116;
  Push(c_proc_num);
  if ((!G_RAM_Checking) || (G_RAM_Checking_Start)) {
	Pop(c_proc_num);
	return;
  }
  //We do not write changed statistics for periodic (cycle) RAM reporting
  //Rather we just write the minimum/maximum statistic and the end of the cycle
  if (EPSR(E_Periodic_RAM_Reporting_6) == "T") {
	Pop(c_proc_num);
	return;
  }
  //When periodic RAM reporting is off we report each change over 24 hours
  //as soon as it occurs.
  boolean l_updates_found = false;
  for (byte l_idx = 0; l_idx < DC_RAMDataCount; l_idx ++) {
	if (G_RAMData[l_idx].Updated) {
	  l_updates_found = true;
	  WriteSRAMStatisticSPICSC2(l_idx);
	  G_RAMData[l_idx].Updated = false; //Reset the update flag
	} //skip if statistic not updated
  } //for each memory statistic
/*
  //We do not want RAM statistics filling up the cache.
  //If any are written then force the cache clear
  ** THIS NOT REQD - IS DONE IN LOOP ANYWAY **
  if (l_updates_found == true)
	ProcessCache(true);
  //
*/
  CheckRAM();
  Pop(c_proc_num);
} //WriteSRAMAnalysisChgsSPICSC3

//------------------------------------------------------------------------------
//ETHERNET FUNCTIONS
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//STRING HANDLING
//------------------------------------------------------------------------------

String ZeroFill(long p_long, int p_length) {
  const byte c_proc_num = 177;
  Push(c_proc_num);
  //Serial.println(p_date);
  String l_string = String(p_long);
  //Serial.println(l_date);
  if (l_string.length() < p_length) {//too short so zerofill
	while (l_string.length() < p_length) {
	  String l_temp = "0" + l_string;
	  //Serial.println(l_temp);
	  l_string = l_temp;
	  //Serial.println(l_date);
	}
  }
  else if (l_string.length() > p_length) {//too long - return asterisks
	l_string = "";
	for (byte l_count = 0; l_count < p_length; l_count++) {
	  String l_temp = "*" + l_string;
	  l_string = l_temp;
	}
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_string;
} //ZeroFill

//------------------------------------------------------------------------------

String FormatAmt(double p_double, byte p_dec_plc) {
  const byte c_proc_num = 178;
  Push(c_proc_num);
  //First do rounding at the last displayed decimal number
  double l_double = p_double;
  String l_minus = "";
  if (l_double < 0) {
	l_minus = "-";
	l_double   = - l_double;
  }
  l_double = l_double + (5/pow(10,p_dec_plc+1));
  //Now take the whole number portion
  //This may be rounded if we are displaying to zero dec places
  long l_int = long(l_double);
  if (p_dec_plc == 0)  {
	//If we are displaying no decimal place just return the integer value
	Pop(c_proc_num);
	return l_minus + String(l_int);}
  else { //extract the decimal part to the correct number of places (as a long)
	long l_dec = long(l_double * pow(10,p_dec_plc)) - (l_int * pow(10,p_dec_plc));
	//Concatenate the integer and decimal portions and return the String
	Pop(c_proc_num);
	return l_minus + String(l_int) + "." + String(l_dec);
  }
} //FormatAmt

//----------------------------------------------------------------------------------------

double StringToDouble(String p_string) {
//We can handle integer and decimal numbers
//We can handle positive and negative (-) numbers
//TUM - We do handle negative numbers that use parentheses to indicate negativity
  const byte c_proc_num = 179;
  Push(c_proc_num);
  //Extract a plain double (float) from a String
  p_string.trim();
  //First find the decimal point
  byte l_dec_pt = p_string.indexOf('.');
  if (l_dec_pt == -1) {
	//If there is no decimal point convert the integer to double
	Pop(c_proc_num);
	return double(p_string.toInt());
  }

  //We have a double - with integer and decimal portions
  //First extract the decimal fraction as an integer
  double l_dec = double(p_string.substring(l_dec_pt + 1).toInt());
  //Now divide it by a power of ten to set the correct float/double value
  l_dec = l_dec / pow(10,p_string.length() - l_dec_pt - 1);

  //Now extract the negative sign and integer portion
  boolean l_negative;
  double  l_value;
  if (p_string.substring(0,1) == "-") {
	l_negative = true;
	l_value    = double(p_string.substring(1,l_dec_pt).toInt()); }
  else {
	l_negative = false;
	l_value    = double(p_string.substring(0,l_dec_pt).toInt());
  }
  l_value += l_dec;
  if (l_negative)
	l_value = -l_value;
  //
  CheckRAM();
  Pop(c_proc_num);
  return l_value;
} //StringToDouble

//-----------------------------------------------------------------------------

String ENDF2(const String &p_line, int &p_start, char p_delimiter) {
  //Extract Next Delmited Field V2
  const byte c_proc_num = 180;
  Push(c_proc_num);
//Extract fields from a line one at a time based on a delimiter.
//Because the line remains intact we dont fragment heap memory
//p_start would normally start as 0
//p_start increments as we move along the line
//We return p_start = -1 with the last field

  //If we have already parsed the whole line then return null
  if (p_start == -1) {
	Pop(c_proc_num);
	return "";
  }

  CheckRAM();
  int l_start = p_start;
  int l_index = p_line.indexOf(p_delimiter,l_start);
  if (l_index == -1) { //last field of the data line
	p_start = l_index;
	Pop(c_proc_num);
	return p_line.substring(l_start);
  }
  else { //take the next field off the data line
	p_start = l_index + 1;
	Pop(c_proc_num);
	return p_line.substring(l_start,l_index); //Include, Exclude
  }
  //Pop(c_proc_num);
} //ENDF2

//----------------------------------------------------------------------------------------
//SPI SELECTION
//----------------------------------------------------------------------------------------

int CurrentSPIDevice() {
  const byte c_proc_num = 181;
  Push(c_proc_num);
  for (int l_index = 0; l_index < C_SPIDeviceCount; l_index++) {
	if (digitalRead(C_SPIDeviceSSList[l_index]) == LOW) {
	  Pop(c_proc_num);
	  return C_SPIDeviceSSList[l_index];
	}
  }
  Pop(c_proc_num);
  return 0;
} //CurrentSPIDevice

//----------------------------------------------------------------------------------------

void SPIDeviceSelect(const int p_device) {
  const byte c_proc_num = 182;
  Push(c_proc_num);
//const int DC_SDCardSSPin = 4;
//const int DC_EthernetSSPin = 10;
//const int DC_SPIHardwareSSPin = 53;
  for (int l_index = 0; l_index < C_SPIDeviceCount; l_index++) {
	if (C_SPIDeviceSSList[l_index] == p_device) digitalWrite(C_SPIDeviceSSList[l_index],LOW); //SELECT
	else                                        digitalWrite(C_SPIDeviceSSList[l_index],HIGH); //DESELECT
  }
  Pop(c_proc_num);
} //SPIDeviceSelect

//-----------------------------------------------------------------------------
//UDP NTP
//-----------------------------------------------------------------------------

void CheckForUPDNTPUpdate() {
  //We get time updates at 6:27am each day
  const byte c_proc_num = 183;
  Push(c_proc_num);
  if ((G_NextUDPNTPUpdateTime == 0) || (Now() < G_NextUDPNTPUpdateTime)) {
	Pop(c_proc_num); //Pop off the G_CallHierarchy stack when we return early
	return;
  }
  //ListSocketStatus(); //List the sockets before doing UDP NTP
  G_UDPNTPOK = ReadTimeUDP(); //Will error out if G_EthernetOk == false
  CheckRAM();
  if (G_UDPNTPOK) ActivityWrite(EPSR(E_UDP_NTP_OK_18));
  else            ActivityWrite(Message(M_UDP_NTP_Failure_2)); //Can happen if returned packet not 48 bytes
  //If the UDPNTP time update failed we update to the next update time and skip this day's update
  G_NextUDPNTPUpdateTime = Date() + 100000 + C_UDPNTPUpdateTime; //Tomorrow @ 6:27am
  CheckRAM();
  Pop(c_proc_num);
} //CheckForUPDNTPUpdate

//----------------------------------------------------------------------------------------

boolean ReadTimeUDP() {
  const byte c_proc_num = 184;
  Push(c_proc_num);
  IPAddress G_TimeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
  // IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
  // IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server
  const unsigned int C_UDPlocalPort = 8888;      // local port to listen for UDP packets

  if (!G_EthernetOK) {  //Indicates that the EthernetServer is up
	Pop(c_proc_num);
	return false;
  }

  //If the EthernetClient is active just wait for it to timeout
  //Otherwise it seems we will likely get a system crash
  if (G_EthernetClientActive) {
	Pop(c_proc_num);
	return false;
  }

  //ActivityWrite(EPSR(E_UPD_NTP_hy_Start_3496));
  boolean l_result = false; //D
  if (G_Udp.begin(C_UDPlocalPort) == 0) {
	//Failed - we will skip today's update because no socket available
	Pop(c_proc_num);
	return false;
  }
  //ActivityWrite("Socket: " + String(G_Udp._sock);
  CheckRAM();

  sendNTPpacket(G_TimeServer); // send an NTP packet to a time server
  //ActivityWrite("- Send");

  //Wait upto three seconds for a result
  unsigned long l_three_seconds = millis();
  int l_pck_size = 0;
  while (CheckSecondsDelay(l_three_seconds, C_OneSecond * 3) == false) {
	l_pck_size = G_Udp.parsePacket();
	if (l_pck_size == DC_NTP_PACKET_SIZE)
	  break;
	//
	delay(250); //quarter second
  }
  //ActivityWrite("- Packet");
  if (l_pck_size == DC_NTP_PACKET_SIZE) {
	// We've received a packet, read the data from it
	//Serial.print("Packet Size: ");
	//Serial.println(l_pck_size);
	G_Udp.read(G_PacketBuffer,DC_NTP_PACKET_SIZE);  // read the packet into the buffer
	//ActivityWrite("- Read");
	//the timestamp starts at byte 40 of the received packet and is four bytes,
	// or two words, long. First, extract the two words:
	unsigned long l_highWord = word(G_PacketBuffer[40], G_PacketBuffer[41]);
	unsigned long l_lowWord = word(G_PacketBuffer[42], G_PacketBuffer[43]);
	// combine the four bytes (two words) into a long integer
	// this is NTP time (seconds since Jan 1 1900):
	unsigned long l_secsSince1900 = l_highWord << 16 | l_lowWord;

	//Adjust for the millis() offset when we began
	l_secsSince1900 -= (millis() / 1000);

	// now convert NTP time into UNIX time starting Jan 1 1970
	//In seconds, that's 2208988800:
	const unsigned long l_seventyYears = 2208988800UL;
	unsigned long l_epoch = l_secsSince1900 - l_seventyYears;

	//Now add NZ Time Advance (We are still in seconds)
	l_epoch = l_epoch + (long(3600) * 12); //NZST - we record time in NZST and adjust time requests for DST

	//Now adjust to Ardy time starting 01 Jan 2013
	unsigned long l_ArdyOffset = (15705L * 86400L); //
	unsigned long l_ArdyTimeSecs = l_epoch - l_ArdyOffset;

	//Now calculare Ardy days
	unsigned long l_ArdyDays = (l_ArdyTimeSecs / 86400L);

	//Now calculate Ardy seconds
	unsigned long l_ArdySeconds = l_ArdyTimeSecs - (l_ArdyDays * 86400L);
	//Scale Ardy Seconds to Ardy time - a fraction of 100,000 (Ardy units per day)
	long l_ArdyTime = l_ArdySeconds * 1000L / 864L;

	//Finally ass Ardy days to Ardy seconds to create Ardy Time
	long l_ArdyDateTime = (100000L * l_ArdyDays) + l_ArdyTime;
	//And initialise the Ardy clock

	//Serial.print(F("NTP Date: "));
	//Serial.println(DateToString(ArdyDateTime));
	//Serial.print(F("NTP Time: "));
	//Serial.println(TimeToHHMMSS(ArdyDateTime));

	//ActivityWrite("- Pre Init");
	SetStartDatetime(l_ArdyDateTime);
	//ActivityWrite("- Post Init");
	l_result = true;
  }
  //else
	//ActivityWrite("- Packet Size NOK " + String(l_pck_size));
  //
  CheckRAM();
  G_Udp.stop();
  Pop(c_proc_num);
  return l_result;
} //ReadTimeUDP

//-------------------------------------------------

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& p_address) {
  const byte c_proc_num = 185;
  Push(c_proc_num);
  // set all bytes in the buffer to 0
  memset(G_PacketBuffer, 0, DC_NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  G_PacketBuffer[0] = 0b11100011;   // LI, Version, Mode
  G_PacketBuffer[1] = 0;     // Stratum, or type of clock
  G_PacketBuffer[2] = 6;     // Polling Interval
  G_PacketBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  G_PacketBuffer[12] = 49;
  G_PacketBuffer[13] = 0x4E;
  G_PacketBuffer[14] = 49;
  G_PacketBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  G_Udp.beginPacket(p_address, 123); //NTP requests are to port 123
  CheckRAM();
  G_Udp.write(G_PacketBuffer,DC_NTP_PACKET_SIZE);
  G_Udp.endPacket();

  Pop(c_proc_num);
  return 0; //??
} //sendNTPpacket

//----------------------------------------------------------------------------------------------
//LOGGING FUNCTIONALITY
//----------------------------------------------------------------------------------------

void ActivityWrite(const String &p_activity) {
  const byte c_proc_num = 186;
  Push(c_proc_num);
  AppendCacheNode(DC_Activity_Log, p_activity);
  Pop(c_proc_num);
}

void HTMLWrite(const String &p_html_request) {
  const byte c_proc_num = 187;
  Push(c_proc_num);
  AppendCacheNode(DC_HTML_Request_Log, p_html_request);
  Pop(c_proc_num);
}
/*
void HackWrite(const String &p_html_request) {
  const byte c_proc_num = 188;
  Push(c_proc_num);
  AppendCacheNode(DC_Hack_Attempt_Log, p_html_request);
  Pop(c_proc_num);
}
*/
void ActivityWriteXM(const String &p_activity) {
  //This is still called by TwoWayCommsSPICSC2() that is called from setup()
  const byte c_proc_num = 142;
  Push(c_proc_num);
  FileMMDDWriteSPICSC(EPSR(E_Activity_310),EPSR(E_ACTV_151),p_activity);
  Pop(c_proc_num);
} //ActivityWrite

/*
void HackWriteXM(const String &p_html_request) {
  const byte c_proc_num = 188;
  Push(c_proc_num);
  FileMMDDWriteSPICSC(EPSR(E_HACKLOGS_154),EPSR(E_HACK_155),p_html_request);
  Pop(c_proc_num);
} //HackWrite

void HTMLWriteXM(const String &p_html_request) {
  const byte c_proc_num = 187;
  Push(c_proc_num);
  FileMMDDWriteSPICSC(EPSR(E_HTMLREQU_152),EPSR(E_HTML_153),p_html_request);
  Pop(c_proc_num);
} //HTMLWrite
*/
//----------------------------------------------------------------------------------------

String DetermineFilename(const int p_type) {
//Folder (directory) filenames for log files use embedded date (DD, MM and/or YY) values.
//This allows us to order all log files on the Micro SD Card in chronological folders and files.
  const byte c_proc_num = 137;
  Push(c_proc_num);

  int l_dd, l_mm, l_yyyy, l_hh, l_mn = 0;
  DateDecode(Date(), &l_dd, &l_mm, &l_yyyy);
  TimeDecode(Now(), &l_hh, &l_mn);

  String l_dir_file;
  l_dir_file.reserve(40); //Minimise heap fragmentation
  //e.g. /BACKUPS/2015/08/21/08210500.TXT/ - 33 chars
  String l_filename_prefix;
  if (p_type == DC_Activity_Log) {
	l_dir_file = EPSRU(E_Activity_310);
	l_filename_prefix = EPSR(E_ACTV_151); }
  else if (p_type == DC_Crawler_Request_Log) {
	l_dir_file = EPSR(E_CRAWLER_298);
	l_filename_prefix = "CRW"; }
  else if (p_type == DC_CWZ_Request_Log) {
	l_dir_file = "CWZLREQU";
	l_filename_prefix = "CWZL"; }
  else if (p_type == DC_HTML_Request_Log) {
	l_dir_file = EPSR(E_HTMLREQU_152);
	l_filename_prefix = "HTM"; }
  else {     //HACK
	l_dir_file = EPSR(E_HACKLOGS_154);
	l_filename_prefix = EPSR(E_HACK_155);}
  //
  l_dir_file += "/" + ZeroFill(l_yyyy,4) + "/" + ZeroFill(l_mm,2);
  if ((p_type == DC_Crawler_Request_Log) || (p_type == DC_HTML_Request_Log)) {
	l_dir_file += "/" + ZeroFill(l_dd,2);
  }

  int l_current_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);
  SD.mkdir(&l_dir_file[0]);
  SPIDeviceSelect(l_current_device);

  l_dir_file += "/" + l_filename_prefix + ZeroFill(l_mm,2) + ZeroFill(l_dd,2);
  if ((p_type == DC_Crawler_Request_Log) || (p_type == DC_HTML_Request_Log)) {
	String l_sub = GetSubFileChar(l_hh);
	l_dir_file += l_sub;
  }
  l_dir_file += ".txt";

  CheckRAM();
  Pop(c_proc_num);
  //Serial.println(l_dir_file);
  return l_dir_file;
} //DetermineFilename

//----------------------------------------------------------------------------------------

void FileMMDDWriteSPICSC(const String &p_directory, const String &p_filename_prefix, const String &p_message) {
//This is now only called by ActivityWriteXM as follows:
//FileMMDDWriteSPICSC(EPSR(E_ACTIVITY_150),EPSR(E_ACTV_151),p_activity);
  const byte c_proc_num = 189;
  Push(c_proc_num);
  if (!G_SDCardOK) {
	Pop(c_proc_num);
	return;
  }

  //We record the current SPI device so we can return to it
  int l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);

  String l_dir_file = "";
  if (p_filename_prefix == EPSR(E_ACTV_151))
	l_dir_file = DetermineFilename(DC_Activity_Log);
  else if (p_filename_prefix == EPSR(E_HTML_153)) //No Call?
	l_dir_file = DetermineFilename(DC_HTML_Request_Log);
  else //No Call?
	l_dir_file = DetermineFilename(DC_Hack_Attempt_Log);
  //
  File l_SD_file = SD.open(&l_dir_file[0], FILE_WRITE);
  CheckRAM();
  if (l_SD_file) {
	if (p_message[0] != '-') {
	  l_SD_file.print(TimeToHHMMSS(Now()));
	  l_SD_file.print(" ");
	}

	int l_posn = 0;
	l_SD_file.println(ENDF2(p_message,l_posn,'\t')); //First line of message
	while (l_posn != -1) {
	  l_SD_file.print("- ");
	  l_SD_file.println(ENDF2(p_message,l_posn,'\t')); //indent additional lines
	}
	l_SD_file.close();
  }
  SPIDeviceSelect(l_current_SPI_device);
  CheckRAM();
  Pop(c_proc_num);
} //FileMMDDWriteSPICSC

//------------------------------------------------------------------------------
/*
This is an example email conversation

S: 220 smtp.example.com ESMTP Postfix
C: HELO relay.example.org
S: 250 Hello relay.example.org, I am glad to meet you
C: MAIL FROM:<bob@example.org>
S: 250 Ok
C: RCPT TO:<alice@example.com>
S: 250 Ok
C: RCPT TO:<theboss@example.com>
S: 250 Ok
C: DATA
S: 354 End data with <CR><LF>.<CR><LF>
C: From: "Bob Example" <bob@example.org>
C: To: "Alice Example" <alice@example.com>
C: Cc: theboss@example.com
C: Date: Tue, 15 January 2008 16:02:43 -0500
C: Subject: Test message
C:
C: Hello Alice.
C: This is a test message with 5 header fields and 4 lines in the message body.
C: Your friend,
C: Bob
C: .
S: 250 Ok: queued as 12345
C: QUIT
S: 221 Bye
{The server closes the connection}
*/
//----------------------------------------------------------------------------------------

void SendTestEmail() {
  const byte c_proc_num = 190;
  Push(c_proc_num);

  if (!G_SendTestEmail) {
	Pop(c_proc_num);
	return;
  }
  G_SendTestEmail = false;
  ActivityWrite(EPSR(E_SEND_TEST_EMAIL_206));
#ifdef EMAILDebug
  Serial.println(EPSR(E_SEND_TEST_EMAIL_206));
#endif
  EmailInitialise(EPSR(E_SEND_TEST_EMAIL_206));
  for (byte l_count = 1; l_count < 4; l_count++)
	EmailLine("Line " + String(l_count));
  //
  EmailDisconnect();
  ActivityWrite(EPSR(E_Test_email_Complete_208));
#ifdef EMAILDebug
  Serial.println(EPSR(E_Test_email_Complete_208));
#endif
  CheckRAM();
  Pop(c_proc_num);
} //SendTestEmail

//----------------------------------------------------------------------------------------

void EmailInitialise(const String &p_heading) {
  const byte c_proc_num = 134;
  Push(c_proc_num);
  AppendCacheNode(DC_Email_Initialise, p_heading);
  Pop(c_proc_num);
}

byte EmailInitialiseXM(const String &p_heading) {
//This procedure initialises an email.
//We do everything except send the email content.
//We then call EmailLine() multiple times to write the email content
//Iterative calls to EmailLine() should minimise string use for emails
//We then call EmailDisconnect to finish up.

//If we exit early with an error message the G_EthernetClient may still be instantiated
//However it will be deleted within WebServerProcess() after two seconds

//Return (Error) Codes:
//0 - All OK - Email Initialised, Ready to send email lines
//1 - G_Email_active is false (email functionality is turned off)
//2 - G_EthernetOK is false (connection to ethernet not established at startup)
//3 - Could not connect to the remote email server via port 25
//4 - The remote email server has not said it is ready to talk to us
//5 - The remote email server did not acknowledge our helo greeting
//6 - The remote email server did not acknowledge the sender email address
//7 - The remote email server did not acknowledge the recipient email address
//8 - The remote email server did not acknowledge our DATA start command
//9 - We cannot open the message file

//After we send the DATA start command we resend the TO and FROM email addresses,
//send the email subject line and send a blank line to indiccate the start of the
//email lines. (We use the EmailLine() procedure to send each email line.
//Finally we will call the EmailDisconnect() procedure to complete the transmission.

  const byte c_proc_num = 139;
  Push(c_proc_num);

  //We cannot assume that the Ethernet SPI (the default) is active
  byte l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_EthernetSSPin);

  if (!G_Email_Active) {
	ActivityWrite(Message(M_Email_Currently_Disabled_16));
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 1;
  }
  if (!G_EthernetOK) {
	ActivityWrite(Message(M_Ethernet_Email_Not_Operational_12));
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 2;
  }

  CheckRAM();

  //Wait for the EthernetClient to timeout
  while (CheckSecondsDelay(G_EthernetClientTimer, C_OneSecond * 2) == false)
	; //do nothing
  //
  //Reset the EthernetClient if needed
  //This releases the heap object
  if (G_EthernetClientActive) {
	//Clear the input buffer
	while( G_EthernetClient.available())
	  G_EthernetClient.read();
	//
	G_EthernetClient.flush();
	G_EthernetClient.stop();
	G_EthernetClientActive = false;
	//Timer reset above in call to CheckSecondsDelay()
	//At this point we have released the G_EthernetClient to the heap
  }

  //Catweazle's ISP uses SSL and we don't have that in Arduino
  //Catweazle's server has relay open for his static IP addresses.
  String l_smtp_server = EPSR(E_SMTPServer_238);
  boolean l_result = G_EthernetClient.connect(&l_smtp_server[0],25); //SMTP normally on port 25

#ifdef EMAILDebug
  Serial.println(F("Email Connect"));
  Serial.println(l_smtp_server);
#endif
  if (!l_result) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 3;
  }
  //We have just instantiated a new EthernetClient
  G_EthernetClientActive  = true;
  //No need for a timer during email
  CheckRAM();
  if(!CheckEmailResponse()) {
	//220 emailserver.co.xyz Microsoft ESMTP MAIL Service, Version: 6.0.3790.4675 ready at  Fri, 28 Mar 2014 12:14:42 +1300
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 4;
  }

  //Open the message file and download all the required messages
  SPIDeviceSelect(DC_SDCardSSPin);
  File l_message_file;
  if (!MessageOpenSPIBA(l_message_file)) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 9;
  }

  //Download the required messages
  String l_email_server    = MessageReadSPIBA(l_message_file, M_email_server_IP_22);
  String l_email_sender    = MessageReadSPIBA(l_message_file, M_email_sender_24);
  String l_email_recipient = MessageReadSPIBA(l_message_file, M_email_recipient_23);
#ifdef EMAILDebug
  Serial.println(l_email_server);
  Serial.println(l_email_sender);
  Serial.println(l_email_recipient);
#endif

  //Close the message file
  l_message_file.close();
  SPIDeviceSelect(DC_EthernetSSPin);

  //helo <<ip address>>
  String l_helo_return_ip = String(EPSR(E_helo__146) + l_email_server);
  G_EthernetClient.println(l_helo_return_ip); // change to your public ip
#ifdef EMAILDebug
  Serial.println(l_helo_return_ip); // change to your public ip
#endif
  if (!CheckEmailResponse()) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 5;
  }

  String l_sender = String(EPSR(E_MAIL_Fromco__147) + l_email_sender);
  G_EthernetClient.println(l_sender);
#ifdef EMAILDebug
  Serial.println(l_sender);
#endif
  if (!CheckEmailResponse()) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 6;
  }

  String l_recipient = String(EPSR(E_RCPT_Toco__148) + l_email_recipient);
  G_EthernetClient.println(l_recipient); // change to recipient address
#ifdef EMAILDebug
  Serial.println(l_recipient);
#endif
  if (!CheckEmailResponse()) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 7;
  }
/*
C: DATA
S: 354 Send message content; end with <CRLF>.<CRLF>
C: Date: Thu, 21 May 2008 05:33:29 -0700
C: From: SimLogix <mail@simlogix.xyz>
C: Subject: The Next Meeting
C: To: john@mail.com
C:
C: Hi John,
C: The next meeting will be on Friday.
C: Anna.
C: .
S: 250 OK
*/
  G_EthernetClient.println(F("DATA"));
#ifdef EMAILDebug
  Serial.println(F("DATA"));
#endif
  if (!CheckEmailResponse()) {
	//354 Start mail input; end with <CRLF>.<CRLF>
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return 8;
  }

  //During the DATA section there are no responses from the mail server
  //Why do we do to and from a second time
  l_recipient = String("To: " + l_email_recipient);  //recipient
  G_EthernetClient.println(l_recipient);  //recipient
#ifdef EMAILDebug
  Serial.println(l_recipient);
#endif
  //Responses not received during the DATA section

  //Why do we do to and from a second time
  l_sender = String(EPSR(E_Fromco__149) + l_email_sender);
  G_EthernetClient.println(l_sender);  //Sender
#ifdef EMAILDebug
  Serial.println(l_sender);
#endif
  //Responses not received during the DATA section

  G_EthernetClient.print(F("Subject: "));
  G_EthernetClient.print(p_heading);
  //This blank line completes the heading part of the DATA section
  G_EthernetClient.println(F("\r\n"));
#ifdef EMAILDebug
  Serial.print(F("Subject: "));
  Serial.print(p_heading);
  Serial.println(F("\r\n"));
#endif
  //Responses not received during the DATA section

  String l_message = String(EPSR(E_Email__56) + p_heading);
  ActivityWrite(l_message);

  SPIDeviceSelect(l_current_SPI_device);
  CheckRAM();
  Pop(c_proc_num);
  return 0;
} //EmailInitialiseXM

//----------------------------------------------------------------------------------------

void EmailLine(const String &p_line) {
  const byte c_proc_num = 131;
  Push(c_proc_num);
  AppendCacheNode(DC_Email_Line, p_line);
  Pop(c_proc_num);
}

void EmailLineXM(const String &p_line) {
  const byte c_proc_num = 141;
  Push(c_proc_num);

  //We cannot assume that the Ethernet SPI (the default) is active
  byte l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_EthernetSSPin);

  G_EthernetClient.println(p_line);
#ifdef EMAILDebug
  Serial.println(p_line);
#endif

  SPIDeviceSelect(l_current_SPI_device);
  CheckRAM();
  Pop(c_proc_num);
} //EmailLineXM

//-----------------------------------------------------------------------------

void EmailDisconnect() {
  const byte c_proc_num = 133;
  Push(c_proc_num);
  AppendCacheNode(DC_Email_Disconnect, "");
  Pop(c_proc_num);
}

void EmailDisconnectXM() {
  const byte c_proc_num = 140;
  Push(c_proc_num);

  //We cannot assume that the Ethernet SPI (the default) is active
  byte l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_EthernetSSPin);

  CheckRAM();

  //Signature
  G_EthernetClient.println(F(""));
  G_EthernetClient.print(F("Sent by Catweazles "));
  G_EthernetClient.print(G_Website);
  G_EthernetClient.println(F(" EtherMega Monitoring System"));
  //This full stop signals completion of the DATA section
  G_EthernetClient.println(F(".")); //End of Message
  if (!CheckEmailResponse()) {
	ActivityWrite(Message(M_ERROR_Email_Failure_13));
	G_EthernetClient.println(F("QUIT"));
	//Now clear the input buffer
	while( G_EthernetClient.available())
	  G_EthernetClient.read();
	//
	G_EthernetClient.flush();
	G_EthernetClient.stop();
	G_EthernetClientActive = false;
	//No need to reset timer during email
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return;
  }

  G_EthernetClient.println(F("QUIT"));
  if (!CheckEmailResponse()) {
	ActivityWrite(Message(M_ERROR_Email_Failure_13));
	//Now clear the input buffer
	while( G_EthernetClient.available())
	  G_EthernetClient.read();
	//
	G_EthernetClient.flush();
	G_EthernetClient.stop();
	G_EthernetClientActive = false;
	//No need to reset timer during email
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return;
  }
  ActivityWrite(EPSR(E_Email_OK_57));

  CheckRAM();
  //Make sure input buffer is clear
  while( G_EthernetClient.available())
	G_EthernetClient.read();
  //
  G_EthernetClient.flush();
  G_EthernetClient.stop();
  G_EthernetClientActive = false;
  //No need to reset timer during email
  SPIDeviceSelect(l_current_SPI_device);
  Pop(c_proc_num);
} //EmailDisconnectXM

//----------------------------------------------------------------------------------------

boolean CheckEmailResponse() {
//After sending an SMTP command to the email server this
//procedure receives and checks the response.
  const byte c_proc_num = 132;
  Push(c_proc_num);

  CheckRAM();
  //Wait up to five seconds for a response
  unsigned long l_starttime = millis();
  while(!G_EthernetClient.available()) {
	if (CheckSecondsDelay(l_starttime, C_OneSecond * 5) == true) {
#ifdef EMAILDebug
	  Serial.println(F("Five Sec Timeout"));
#endif
	  G_EthernetClient.flush();
	  G_EthernetClient.stop();
	  G_EthernetClientActive = false;
	  //No need to reset timer during email
	  Pop(c_proc_num);
	  return false;
	}
  }
  CheckRAM();

/* THE LIST OF POSSIBLE SMTP RESPONSE CODES
Besides the intermediate reply for DATA, each server's reply can be either
positive (2xx reply codes) or negative. Negative replies can be permanent
(5xx codes) or transient (4xx codes). A reject is a permanent failure by
an SMTP server; in this case the SMTP client should send a bounce message.
A drop is a positive response followed by message discard rather than delivery.
200 	(nonstandard success response, see rfc876)
211 	System status, or system help reply
214 	Help message
220 	<domain> Service ready
221 	<domain> Service closing transmission channel
250 	Requested mail action okay, completed
251 	User not local; will forward to <forward-path>
252 	Cannot VRFY user, but will accept message and attempt delivery
354 	Start mail input; end with <CRLF>.<CRLF>
421 	<domain> Service not available, closing transmission channel
450 	Requested mail action not taken: mailbox unavailable
451 	Requested action aborted: local error in processing
452 	Requested action not taken: insufficient system storage
500 	Syntax error, command unrecognised
501 	Syntax error in parameters or arguments
502 	Command not implemented
503 	Bad sequence of commands
504 	Command parameter not implemented
521 	<domain> does not accept mail (see rfc1846)
530 	Access denied (???a Sendmailism)
550 	Requested action not taken: mailbox unavailable
551 	User not local; please try <forward-path>
552 	Requested mail action aborted: exceeded storage allocation
553 	Requested action not taken: mailbox name not allowed
554 	Transaction failed
*/
  //Get the first char of the response code
  //"250" is the OK response code
  //Codes of "400" and above seem to be a problem
  byte l_respCode = G_EthernetClient.peek();

  //Now deal with the response message
  while( G_EthernetClient.available()) {
	char l_char = G_EthernetClient.read();
#ifdef EMAILDebug
	Serial.print(l_char);
#endif
  }

  CheckRAM();
  if (l_respCode >= '4') {
	G_EthernetClient.println(F("QUIT"));
#ifdef EMAILDebug
	Serial.println(F("QUIT"));
#endif
	//Now clear the input buffer
	while( G_EthernetClient.available())
	  G_EthernetClient.read();
	//
	G_EthernetClient.flush();
	G_EthernetClient.stop();
	G_EthernetClientActive = false;
	//No need to reset timer during email
	Pop(c_proc_num);
	return false;
  }

  CheckRAM();
  Pop(c_proc_num);
  return true;
} //CheckEmailResponse

//-----------------------------------------------------------------------------

void TwoWayCommsSPICSC2(const String &p_message) {
  const byte c_proc_num = 135;
  Push(c_proc_num);
  //Write to the serial port
  //Write to the activity log
  //Build up the content for an email
  //CheckRAM();
  Serial.println(p_message); //KEEP THIS OR IT WON'T BE TWO WAY
  ActivityWriteXM(p_message);
  //if (p_email_init == 0)
  //  EmailLine(p_message);
  //
  Pop(c_proc_num);
}  //TwoWayCommsSPICSC2

//----------------------------------------------------------------------------------------
/*
void ListSocketStatus() {
//Status/Destination/Port
//Socket#0:0x0 80 D:192.168.1.55(49643)
//Socket#1:0x14 80 D:192.168.1.55(49642)
//Socket#2:0x0 0 D:0.0.0.0(0)
//Socket#3:0x0 0 D:0.0.0.0(0)

//A socket status list:
//0X0 = available.
//0x14 = socket waiting for a connection
//0x17 = socket connected to a server.
//1C - Close Wait
//0x22 = UDP socket.

//  static const uint8_t CLOSED      = 0x00;
//  static const uint8_t INIT        = 0x13;
//  static const uint8_t LISTEN      = 0x14;
//  static const uint8_t SYNSENT     = 0x15;
//  static const uint8_t SYNRECV     = 0x16;
//  static const uint8_t ESTABLISHED = 0x17;
//  static const uint8_t FIN_WAIT    = 0x18;
//  static const uint8_t CLOSING     = 0x1A;
//  static const uint8_t TIME_WAIT   = 0x1B;
//  static const uint8_t CLOSE_WAIT  = 0x1C;
//  static const uint8_t LAST_ACK    = 0x1D;
//  static const uint8_t UDP         = 0x22;
//  static const uint8_t IPRAW       = 0x32;
//  static const uint8_t MACRAW      = 0x42;
//  static const uint8_t PPPOE       = 0x5F;

  const byte c_proc_num = 136;
  Push(c_proc_num);
  ActivityWrite(EPSR(E_ETHERNET_SOCKET_LIST_27));
  ActivityWrite(EPSR(E_Socket_Heading_28));

  String l_line = "";
  l_line.reserve(64);
  char l_buffer[10] = "";
  for (uint8_t l_socket = 0; l_socket < MAX_SOCK_NUM; l_socket++) {
	l_line = " " + String(l_socket);
	uint8_t l_status = W5100.readSnSR(l_socket); //status
	l_line += " 0x";
	sprintf(l_buffer,"%x",l_status);
	if (l_status != 23) { //HEX 17 Connected
	  //The connection status has cleared since the last one minute check
	  G_SocketConnectionTimers[l_socket] = 0;
	  G_SocketConnectionTimes[l_socket] = 0;
	}
	l_line += l_buffer;
	l_line += " ";
	l_line += String(W5100.readSnPORT(l_socket)); //port
	l_line += " D:";
	uint8_t dip[4];
	W5100.readSnDIPR(l_socket, dip); //IP Address
	for (int j=0; j<4; j++) {
	  l_line += int(dip[j]);
	  if (j<3) l_line += ".";
	}
	l_line += " (";
	l_line += String(W5100.readSnDPORT(l_socket)); //port on destination
	l_line += ") ";
	if (G_SocketConnectionTimes[l_socket] != 0)
	  l_line += TimeToHHMM(G_SocketConnectionTimes[l_socket]);
	//

	ActivityWrite(l_line);
  }
  CheckRAM();
  Pop(c_proc_num);
} // ListSocketStatus
*/

//----------------------------------------------------------------------------------------

void CacheNodeDelete(struct TCacheItem *&p_Prev, struct TCacheItem *&p_Curr) {
//Delete the current node from the linked list
//We may need to reset the value of Next in the previous node to skip the node we delete
//If the current node is the head of the linked list we need to update G_CacheHead
//If the current node is the end (tail) of the free list we need to adjust G_CacheTail to the new (prev) tail.
//We might be deleting the last node in the cache linked list so G_CacheHead & G_CacheTail are both reset to NULL.
//We retain the value of p_Prev - that is still the previous node (may be NULL if we are delecting the first cache record.
//We update p_Curr to the next node in the cache linked list - it is needed to the calling procedure.
  const byte c_proc_num = 80;
  Push(c_proc_num);
  //Serial.print("CACHE DELETE ");
  //Serial.println(p_Curr->ItemData);
  G_CacheCount--;
  G_CacheSize -= (sizeof(TCacheItem) + p_Curr->ItemData.length());
  if (p_Curr == G_CacheHead) {     //We are deleting the head
	G_CacheHead = p_Curr->Next;    //The new head points to the record after this one
	delete(p_Curr);                //Delete the current record (p_Curr now invalid)
	if (G_CacheHead == NULL)       //If the new head is null (no more nodes)
	  G_CacheTail = NULL;          //  then the tail must be NULL
	//
	p_Curr = G_CacheHead; }        //p_Curr moves fwd to next node (it may be NULL - end of list/p_Prev must still be NULL)
  else {                           //if the delete record is not the head p_Prev must have been instantiated
	p_Prev->Next = p_Curr->Next;   //skip record to be deleted
	delete(p_Curr);                //and delete it (p_Curr now invalid)
	if (p_Prev->Next == NULL)      //p_Prev must be pointing to the tail
	  G_CacheTail = p_Prev;        //  so update the tail
	//
	p_Curr = p_Prev->Next;         //p_Curr moved fwd to next node (it may be NULL - end of list)
  } //
  CheckRAM();
  Pop(c_proc_num);
} //CacheNodeDelete

//------------------------------------------------------------------------------

void FileCacheWrite(const byte p_type, const String &p_data) {
//To preserve RAM we write the cache to a file when memory is getting low.
//We pass in a new record (in some circumstances) which we append to the file cache as well.
/*
typedef struct TCacheItem *PCacheItem;
struct TCacheItem {
  String ItemData;
  byte Type;
  PCacheItem Next;
};
*/
  const byte c_proc_num = 143;
  Push(c_proc_num);

  int l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);

#ifdef FileCacheDebug
  Serial.print(F("FileCacheWrite "));
  Serial.println(freeRam());
  int l_count = 0;
#endif
  File l_SD_file = SD.open(C_Cache_Filename, FILE_WRITE);
  if (!l_SD_file) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return;
  }
  CheckRAM();

  PCacheItem l_Curr = G_CacheHead;
  PCacheItem l_Prev = NULL;
  while (l_Curr) {
#ifdef FileCacheDebug
	l_count++;
#endif
	l_SD_file.print(l_Curr->Type);
	l_SD_file.print('\t');
	l_SD_file.println(l_Curr->ItemData);
	CacheNodeDelete(l_Prev,l_Curr);
  } //iterate the cache linked list
  //The cache has now been cleared and the SRAM released

  //If we also passed in a new record we append it to the file cache.
  if (p_data != "") {
	//
#ifdef FileCacheDebug
	l_count++;
#endif
	l_SD_file.print(p_type);
	l_SD_file.print('\t');
	l_SD_file.println(p_data);
  }
  l_SD_file.close();
#ifdef FileCacheDebug
  Serial.print(F("New Cache Records "));
  Serial.println(l_count);
#endif

  CheckRAM();
  SPIDeviceSelect(l_current_SPI_device);
  Pop(c_proc_num);
} //FileCacheWrite

//------------------------------------------------------------------------------

void AppendCacheNode(const byte p_type, const String &p_data) {
//Pass in the two data fields for this new linked list cache item
//p_data may be tab delimited multiple lines needing multiple nodes
//Dynamically allocate some ram for this new cached item using new()
//malloc() cannot be used - it will not instantiate the cached item ItemData String
//Add the new cache record to the end of the cache linked list.
//We want to process (clear) the linked list later in chronological order
  const byte c_proc_num = 138;
  Push(c_proc_num);

  //If we are running low on SRAM we write the current cache to the
  //cache file and then write the this new item to the file cache.
  if ((freeRam() - p_data.length()) < 750) {
	FileCacheWrite(p_type, p_data);
	CheckRAM();
	Pop(c_proc_num);
	return;
  }

  PCacheItem l_Curr;
  int l_posn = 0;
  while (l_posn != -1) {
	l_Curr = new(TCacheItem);
	l_Curr->Type = p_type;
	//For email nodes there is only one iteration
	if ((p_type < 3) || (p_data.length() == 0)){
	  l_Curr->ItemData = p_data;
	  l_posn = -1; } //Force an exit
	else {
	  if ((l_posn == 0) && (p_data[0] != '-')) //Not a continuation line
		l_Curr->ItemData = TimeToHHMMSS(Now()) + " " + ENDF2(p_data,l_posn,'\t');
	  else if (p_data[l_posn] == '-') //No need to force continuation line
		l_Curr->ItemData = ENDF2(p_data,l_posn,'\t');
	  else //Force continuation
		l_Curr->ItemData = "- " + ENDF2(p_data,l_posn,'\t');
	  //
	}
	//Serial.print(l_Curr->Type);
	//Serial.print(" ");
	//Serial.println(l_Curr->ItemData);
	l_Curr->Next = NULL; //Appending at end of list
	if (G_CacheHead == NULL)
	  G_CacheHead = l_Curr; //First cache record
	else
	  G_CacheTail->Next = l_Curr; //Current tail points to this new tail
	//
	G_CacheTail = l_Curr; //Reset to the new tail and the end of the list
	G_CacheCount++;
	G_CacheSize += sizeof(TCacheItem) + l_Curr->ItemData.length();
  } //multi line tab processing for activities

  CheckRAM();
  Pop(c_proc_num);
} //AppendCacheNode

//------------------------------------------------------------------------------

boolean CacheLogProcessQuery(const int &p_process_type, const int &p_record_type) {
//Only activity and html log records are written to the cache.
//We pull html log data into the hack log when hacks occur.
//We pull crawler html log data (not being hacks) into the crawler log.
//#define DC_Activity_Log 3
//#define DC_Hack_Attempt_Log 4
//#define DC_Crawler_Request_Log 5
//#define DC_CWZ_Request_Log 6
//#define DC_HTML_Request_Log 7
  const byte c_proc_num = 145;
  Push(c_proc_num);

  boolean l_process = false;
  if ((p_process_type == DC_Activity_Log) &&
	  (p_record_type  == DC_Activity_Log))
	l_process = true;
  else if ((p_process_type == DC_CWZ_Request_Log) &&
		   (p_record_type  == DC_HTML_Request_Log) &&
		   (G_CWZ_HTML_Req_Flag)) { //Pull html log data for Catweazle into the CWZL log file (incl hacks)
	l_process = true;
  }
  else if ((p_process_type == DC_Hack_Attempt_Log) &&
		   (p_record_type  == DC_HTML_Request_Log) &&
		   (G_Hack_Flag)) { //Pull html log data for hacks into the hack file (incl Crawler errors)
	l_process = true;
  }
  else if ((p_process_type == DC_Crawler_Request_Log) &&
		   (p_record_type  == DC_HTML_Request_Log) &&
		   (G_Web_Crawler_Index != -1)) { //Pull html log data for crawlers into the crawler file (OK requests only)
	l_process = true;
  }
  else if ((p_process_type == DC_HTML_Request_Log) &&
		   (p_record_type  == DC_HTML_Request_Log) &&
		   (!G_Hack_Flag) &&
		   (!G_CWZ_HTML_Req_Flag) &&
		   (G_Web_Crawler_Index == -1)) {
	//html data is processed to the html file when no hack, CWZ or crawler involved
	l_process = true;
  }
  //CheckRAM is disabled because it is called within loop that is generating
  //more free heap records and we do not want to log every incremental free
  //heap increase - we call CheckRAM in the parent procedure are record the
  //total free heap increment.
  //CheckRAM();
  Pop(c_proc_num);
  return l_process;
} //CacheLogProcessQuery

//------------------------------------------------------------------------------

void WriteCachedLogRecds() {
//Write the cached activity records to one of three log files
//Since this procedure is called after cached emails are transmitted
//  the cache should be clear when this procedure is finished.
//We open the several log files once only and write all required records
//Hopefully each File object is instantiated in the same heap block

//We write to two files only - first the activity log
//and then to one of four log files (Hack, CWZL, Crawler, Public)
  const byte c_proc_num = 79;
  Push(c_proc_num);

  //We record the current SPI device so we can return to it
  int l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);

  //Serial.print("<<LOG FILE WRITE>> ");
  //int l_count = 0;

//#define DC_Activity_Log 3
//#define DC_Hack_Attempt_Log 4
//#define DC_Crawler_Request_Log 5
//#define DC_CWZ_Request_Log 6
//#define DC_HTML_Request_Log 7

  String l_filename;
  l_filename.reserve(40); //Minimise heap fragmentation
  //e.g. /BACKUPS/2015/08/21/08210500.TXT/ - 33 chars

  for (int l_type = DC_Activity_Log; l_type <= DC_HTML_Request_Log; l_type++) { //Types 3, 4, 5 & 6
	File l_SDFile;
	l_filename = DetermineFilename(l_type); //ACTVDDMM, HTMDDMMX, HACKDDMM, CRAWDDMM
	//Serial.println(l_filename);
	boolean l_file_open = false;
	PCacheItem l_Prev = NULL;
	PCacheItem l_Curr = G_CacheHead;
	while (l_Curr) {
	  if (CacheLogProcessQuery(l_type,l_Curr->Type)) {
		//process this node for the selected type
		if (l_file_open == false) {
		  l_SDFile = SD.open(&l_filename[0], FILE_WRITE);
		  if (l_SDFile)
			l_file_open = true;
		  else //could not open
			break; //abandon this log type, try others
		  //
		}
		l_SDFile.println(l_Curr->ItemData);
		CacheNodeDelete(l_Prev,l_Curr);
	  } //selected file type
	  else { //Skip cache records we do not want
		l_Prev = l_Curr;
		l_Curr = l_Curr->Next;
	  } //
	}  //iterate through the list for this record type
	//Fall through here when finished processing the cache
	//or for a break if we cannot open a log file.
	if (l_file_open == true) {
	  if (l_type != DC_Activity_Log) {
		//We write to two files only - first the activity log
		//and then to one of four log files (Hack, CWZL, Crawler, Public)
		//Since we have written html request data to the correct html log file
		//we can now append the html parameter cache to the html log file.
		//THIS CODE IS DUPLICATED HERE AND IN THE FILE CACHE WRITE
		PHTMLParm l_curr = G_HTMLParmListHead;
		while (l_curr) {
		  if (l_curr->Name == E_REQUEST_236)
			l_SDFile.println(EPSR(E_hy_REQUESTco__252) + l_curr->Value);
		  else if (l_curr->Name != E_PASSWORD_231) //Don't list password parms
			l_SDFile.println("- " + EPSR(l_curr->Name) + ": " + l_curr->Value);
		  //
		  l_curr = l_curr->Next;
		}
		l_SDFile.println(); //Force a blank line
	  }
	  l_SDFile.close();
	}
  } //For each log file type
  //Serial.println(l_count); //of log records written to a log file and removed from the cache
  CheckRAM();
  SPIDeviceSelect(l_current_SPI_device);
  Pop(c_proc_num);
} //WriteCachedLogRecds

//------------------------------------------------------------------------------

void FileCacheProcess() {
/*
typedef struct TCacheItem *PCacheItem;
struct TCacheItem {
  String ItemData;
  byte Type;
  PCacheItem Next;
};
*/
  const byte c_proc_num = 144;
  Push(c_proc_num);

  //We don't know if the SD Card SPI device is active
  int l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);

#ifdef FileCacheDebug
  Serial.print(F("FileCacheProcess "));
  Serial.println(freeRam());
  int l_count = 0;
#endif

  if (!SD.exists(C_Cache_Filename)) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return;
  }

  File l_SD_file = SD.open(C_Cache_Filename,FILE_READ);
  if (!l_SD_file) {
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return;
  }

  StatisticCount(St_File_Cache_Operations,false); //Don't exclude CWZ
  String l_line;
  int l_record_type;
  String l_item_data;
  boolean l_log_file_open;
  File l_log_file;
  String l_filename;
  int l_posn;
#ifdef FileCacheDebug
  Serial.print(F("Activity Logs "));
  Serial.println(freeRam());
#endif
  for (int l_type = DC_Activity_Log; l_type <= DC_HTML_Request_Log; l_type ++) { //Types 3, 4, 5, 6 & 7
#ifdef FileCacheDebug
	Serial.print(F("Activity Log "));
	Serial.println(l_type);
#endif
	l_filename = DetermineFilename(l_type); //ACTVDDMM, HTMDDMMX, HACKDDMM, CRAWDDMM, CWZLDDMM
	l_log_file_open = false;
	l_SD_file.seek(0);
	while (l_SD_file.available() != 0) {
	  l_line = l_SD_file.readStringUntil('\n');
	  l_posn = 0;
	  l_record_type = ENDF2(l_line,l_posn,'\t').toInt();
	  l_item_data   = ENDF2(l_line,l_posn,'\t');
	  if (CacheLogProcessQuery(l_type,l_record_type)) {
#ifdef FileCacheDebug
		l_count++;
#endif
		if (!l_log_file_open) {
		  l_log_file = SD.open(&l_filename[0], FILE_WRITE);
		  if (l_log_file)
			l_log_file_open = true;
		  else //could not open
			break; //abandon this log type, try others (and other files)
		  //
		}
		l_log_file.println(l_item_data);
	  } //skip is not correct record type
	} //for each cache file record
	//Fall through here when finished reading the file
	//or for a break if we cannot open the file.
	if (l_log_file_open == true) {
	  if (l_type != DC_Activity_Log) {
		//We write to two files only - first the activity log
		//and then to one of four log files (Hack, CWZL, Crawler, Public)
		//Since we have written html request data to the correct html log file
		//we can now append the html parameter cache to the html log file.
		//THIS CODE IS DUPLICATED HERE AND IN THE HEAP CACHE WRITE
		PHTMLParm l_curr = G_HTMLParmListHead;
		while (l_curr) {
		  if (l_curr->Name == E_REQUEST_236)
			l_log_file.println(EPSR(E_hy_REQUESTco__252) + l_curr->Value);
		  else if (l_curr->Name != E_PASSWORD_231) //Don't list password parms
			l_log_file.println("- " + EPSR(l_curr->Name) + ": " + l_curr->Value);
		  //
		  l_curr = l_curr->Next;
		}
		l_log_file.println(); //Force a blank line
	  }
	  l_log_file.close();
	}
  } //for each log record type

  //Now we do the emails
//#define DC_Email_Initialise 0
//#define DC_Email_Line 1
//#define DC_Email_Disconnect 2
  l_SD_file.seek(0);
  byte l_email_ok = 0; //D
#ifdef FileCacheDebug
  Serial.print(F("Emails "));
  Serial.println(freeRam());
#endif
  while (l_SD_file.available() != 0) {
	l_line = l_SD_file.readStringUntil('\n');
	l_posn = 0;
	l_record_type = ENDF2(l_line,l_posn,'\t').toInt();
	l_item_data   = ENDF2(l_line,l_posn,'\t');
	if (l_record_type <= DC_Email_Disconnect) {
#ifdef FileCacheDebug
	  l_count++;
#endif
	  if (l_record_type == DC_Email_Initialise)
		//Following selects Ethernet SPI and returns to current SPI setting
		l_email_ok = EmailInitialiseXM(l_item_data);
	  else if ((l_record_type == DC_Email_Line) && (l_email_ok == 0))
		//Following selects Ethernet SPI and returns to current SPI setting
		EmailLineXM(l_item_data); //
	  else if ((l_record_type == DC_Email_Disconnect) && (l_email_ok == 0))
		//Following selects Ethernet SPI and returns to current SPI setting
		EmailDisconnectXM();
	  //
	} //skip activity log (html request) records
  } //transmit emails

  //We are finished - close and purge the cache file.
  //If any operations failed we still delete the cache file
  //so the unprocessed log records and emails will be unsent.
  l_SD_file.close();
#ifdef FileCacheDebug
  Serial.print(F("Cache Record Count "));
  Serial.println(l_count);
  Serial.print(F("File Cache Remove "));
  Serial.println(freeRam());
#endif
  StatisticCount(St_File_Cache_Operations,false); //don't exclude CWZ
  SD.remove(C_Cache_Filename);

  //HeapOptimisation();
  CheckRAM();
  SPIDeviceSelect(l_current_SPI_device);
  Pop(c_proc_num);
} //FileCacheProcess

//------------------------------------------------------------------------------

void CachedEmailsSend() {
//Call three existing email fcns from here as cache list is traversed
//If emails do not transmit they are lost.
  const byte c_proc_num = 78;
  Push(c_proc_num);
#ifdef EMAILDebug
  boolean l_email_msg = true;
#endif
  PCacheItem l_Curr = G_CacheHead;
  PCacheItem l_Prev = NULL;
  //l_email_ok indicates if the email operation is working OK
  //0 - is all OK, different non zero values indicate different errors.
  //If we cannot initialise an email transmission we skip
  //remaining line and disconnect records for the current email.
  //Refer to the source code for the list of EmailInitialiseXM return values
  //to determine what corrective action may be required.
  byte l_email_ok = 0; //D
  while (l_Curr) {
	if (l_Curr->Type < 3) { //for 2 & 3 you must have l_email_ok
#ifdef EMAILDebug
	  if (l_email_msg) {
		l_email_msg = false;
		Serial.println(F("<<EMAILS SEND>> "));
	  }
#endif
	  if (l_Curr->Type == DC_Email_Initialise) {
		//Following selects Ethernet SPI and returns to current SPI setting
		l_email_ok = EmailInitialiseXM(l_Curr->ItemData);
#ifdef EMAILDebug
		if (l_email_ok != 0) {
		  Serial.print(F("Email Error: "));
		  Serial.println(l_email_ok);
		}
#endif
	  }
	  else if ((l_Curr->Type == DC_Email_Line) && (l_email_ok == 0))
		//Following selects Ethernet SPI and returns to current SPI setting
		EmailLineXM(l_Curr->ItemData); //
	  else if ((l_Curr->Type == DC_Email_Disconnect) && (l_email_ok == 0))
		//Following selects Ethernet SPI and returns to current SPI setting
		EmailDisconnectXM();
	  //
	  CacheNodeDelete(l_Prev,l_Curr);
	}
	else { //skip this record, move to next
	  l_Prev = l_Curr;
	  l_Curr = l_Curr->Next;
	}
  } //iterate the cache linked list
  CheckRAM();
  Pop(c_proc_num);
} //CachedEmailsSend

//------------------------------------------------------------------------------

void ProcessCache(boolean p_force) {
//During normal operations (each loop() iteration) we clear the cache.
//However:
//- we force the cache to be cleared at the end of webserver processing
//- before any webserver call to SD card file operations
//- whenever the cache size is going to exceed 512 bytes

//We also make discretionary calls that will clear the cache when free RAM
//is less than 750 bytes.

  const byte c_proc_num = 77;
  Push(c_proc_num);

  int l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);
  //Check if no cache to clear and exit.
  //Exit if free SRAM OK and clearance not being forced.
  if (((G_CacheHead == NULL) && (!SD.exists(C_Cache_Filename))) || //No in memory cache or cache file
	  ((freeRam() > 750) && (p_force == false))) {
	CheckRAM();
	SPIDeviceSelect(l_current_SPI_device);
	Pop(c_proc_num);
	return;
  }

  //If we get here we need to clear the cache
#ifdef HeapDebug
  if (G_HeapDumpPC) {
	CheckRAM();
	HeapDump(" Before");
  }
#endif
  if (SD.exists(C_Cache_Filename)) {
	//If there is already a cache file write all the current memory records to the
	//file to maximise FreeRAM() before processing the whole cache from the file.
	FileCacheWrite(0,""); //0,"" means no new record to add to cache
	FileCacheProcess();   //Includes emails and log file records
  }
  else {  //No cache file - just process the in memory cache
	CachedEmailsSend();          //Part II
	//HeapOptimisation();
	WriteCachedLogRecds(); //Part III
	//HeapOptimisation();
  }
  //The cache should be clear now and HEAP memory minimised

  //Unless there are email transmission entries the cache will be clear
  //and our HEAP use and fragmentation should be minimised.
#ifdef HeapDebug
  if (G_HeapDumpPC) {
	HeapDump(" After");
	HeapOptimisation();
	HeapDump(" Optimised");
  }
#endif
  CheckRAM();
  SPIDeviceSelect(l_current_SPI_device);

  Pop(c_proc_num);
} //ProcessCache

//----------------------------------------------------------------------------------------

void HTMLParmInsert(const int &p_name, const String &p_value) {
  //Insert at start of list - the list is not sorted
  const byte c_proc_num = 168;
  Push(c_proc_num);
  PHTMLParm l_curr = new(THTMLParm);
  l_curr->Name  = p_name;
  l_curr->Value = p_value;
  l_curr->Next  = G_HTMLParmListHead;
  G_HTMLParmListHead = l_curr;
  CheckRAM();
  Pop(c_proc_num);
} //HTMLParmInsert

//----------------------------------------------------------------------------------------

void HTMLParmDelete(const int &p_name) {
  //Delete the parm from the unsorted list
  const byte c_proc_num = 167;
  Push(c_proc_num);

  PHTMLParm l_curr = G_HTMLParmListHead;
  PHTMLParm l_prev = NULL;
  while (l_curr) {
	if (l_curr->Name == p_name) {
	  if (l_prev == NULL) //Delete first record
		G_HTMLParmListHead = l_curr->Next;
	  else //delete a record within the list
		l_prev->Next = l_curr->Next;
	  //
	  delete(l_curr);
	  break;
	}
	l_prev = l_curr;
	l_curr = l_curr->Next;
  }
  CheckRAM();
  Pop(c_proc_num);
} //HTMLParmDelete

//----------------------------------------------------------------------------------------

String HTMLParmRead(const int &p_name) {
  //Read the unsorted list looking for the named parm
  const byte c_proc_num = 166;
  Push(c_proc_num);
  PHTMLParm l_curr = G_HTMLParmListHead;
  while (l_curr) {
	if (l_curr->Name == p_name) {
	  CheckRAM();
	  Pop(c_proc_num);
	  return l_curr->Value;
	}
	l_curr = l_curr->Next;
  }
  CheckRAM();
  Pop(c_proc_num);
  return ""; //not found
} //HTMLParmRead

//----------------------------------------------------------------------------------------

void HTMLParmListDispose() {
  //release the whole linked list
  const byte c_proc_num = 165;
  Push(c_proc_num);
  PHTMLParm l_curr = G_HTMLParmListHead;
  while (l_curr) {
	G_HTMLParmListHead = l_curr->Next;
	delete(l_curr);
	l_curr = G_HTMLParmListHead;
  }
  CheckRAM();
  Pop(c_proc_num);
} //HTMLParmListDispose

//----------------------------------------------------------------------------------------

String MessageReadSPIBA(File &p_msg_file, int p_msg_num) {
  //SPIDeviceSelect(DC_SDCardSSPin) must be pre-selected
  const byte c_proc_num = 164;
  const byte C_MSG_Length = 66; //in CR/LF
  Push(c_proc_num);

  //Determine file length
  unsigned long l_filesize = p_msg_file.size();
  unsigned long l_posn = (p_msg_num - 1) * C_MSG_Length;
  String l_result = "";
  if (l_posn >= l_filesize)
	l_result = String(p_msg_num + ' ' + EPSR(E_File_Record_Length_Error_172));
  else {
	p_msg_file.seek(l_posn);
	l_result = p_msg_file.readStringUntil('*');
  }
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
}  //MessageReadSPIBA

//----------------------------------------------------------------------------------------------

boolean MessageOpenSPIBA(File &p_msg_file) {
  //SPIDeviceSelect(DC_SDCardSSPin) must be pre-selected
  const byte c_proc_num = 163;
  Push(c_proc_num);

  String l_filename = "/MESSAGES.TXT";
  if (!SD.exists(&l_filename[0])) {
	CheckRAM();
	Pop(c_proc_num);
	return false;
  }

  p_msg_file = SD.open(&l_filename[0],FILE_READ);
  if (!p_msg_file) {
	CheckRAM();
	Pop(c_proc_num);
	return false;
  }
  CheckRAM();
  Pop(c_proc_num);
  return true;
}  //MessageOpenSPIBA

//----------------------------------------------------------------------------------------------

String Message(int p_msg_num) {
  const byte c_proc_num = 162;
  Push(c_proc_num);

  byte l_current_SPI_device = CurrentSPIDevice();
  SPIDeviceSelect(DC_SDCardSSPin);

  File l_message_file;
  if (!MessageOpenSPIBA(l_message_file)) {
	SPIDeviceSelect(l_current_SPI_device);
	CheckRAM();
	Pop(c_proc_num);
	return "";
  }

  String l_result = MessageReadSPIBA(l_message_file, p_msg_num);

  l_message_file.close();

  SPIDeviceSelect(l_current_SPI_device);

  CheckRAM();
  Pop(c_proc_num);
  return l_result;
} //Message

//----------------------------------------------------------------------------------------------

void StatisticCount(int p_statistic, boolean p_excl_catweazle) {
//If we know the user is Catweazle and p_excl_catweazle is true we don't count
//We don't count web crawler stats
//For socket disconnects we force stat updates regardless
  const byte c_proc_num = 161;
  Push(c_proc_num);

  boolean l_exclude = false;
  if ((p_excl_catweazle) && (G_CWZ_HTML_Req_Flag))
	l_exclude = true;
  //
  if (G_Web_Crawler_Index != -1)
	l_exclude = true;
  //
  if ((!l_exclude) || (p_statistic == St_Socket_Disconnections) ) {
	G_StatActvMth[p_statistic] = G_StatActvMth[p_statistic] + 1;
	G_StatActvDay[p_statistic] = G_StatActvDay[p_statistic] + 1;
  }
  CheckRAM();
  Pop(c_proc_num);
}  //StatisticCount

//----------------------------------------------------------------------------------------

String GetSubFileChar(int p_hh) {
  const byte c_proc_num = 118;
  Push(c_proc_num);
  String l_sub;
  if (p_hh < 4)
	l_sub = "A";
  else if (p_hh < 8)
	l_sub = "B";
  else if (p_hh < 12)
	l_sub = "C";
  else if (p_hh < 16)
	l_sub = "D";
  else if (p_hh < 20)
	l_sub = "E";
  else
	l_sub = "F";
  //
  Pop(c_proc_num);
  return l_sub;
} //GetSubFileChar

//------------------------------------------------------------------------------

boolean CacheFileExists() {
  boolean l_result = false; //D
  SPIDeviceSelect(DC_SDCardSSPin);
  if (SD.exists(C_Cache_Filename))
	l_result = true;
  //
  SPIDeviceSelect(DC_EthernetSSPin);
  return l_result;
}  //CacheFileExists

//------------------------------------------------------------------------------

#ifdef HeapDebug
void HeapOptimisation() {
//We find the highest parm list or cache node in the heap and see if we can
//move it lower in the heap and to reduce the size of the heap.
//If our actions do not reduce the heap size we exit.
  const byte c_proc_num = 120;
  Push(c_proc_num);
  extern int __data_start, __stack,  __bss_end, __data_end, __heap_start, *__brkval, _end;
  //Serial.print(F("."));
  CheckRAM();
  if ((G_CacheHead == NULL) && (G_HTMLParmListHead == NULL)) {
  //if (G_CacheHead == NULL) {
	Pop(c_proc_num);
	return;
  }

  //int l_heap_at_start = (int)__brkval;
  int l_heap_end;

  //G_HTMLParmListHead;
  //Pointers for the parm list
  PHTMLParm l_highest_parm;
  PHTMLParm l_prev_parm;
  PHTMLParm l_next_parm;

  //Pointers for the cache
  PCacheItem l_highest_node;
  PCacheItem l_prev_node;
  PCacheItem l_next_node;

  while (true) {
	//Serial.println(F("Iterate .."));

	//Extract the current top of heap
	l_heap_end = (int)__brkval;

	//Iterate to find the current highest node in the parm list
	l_prev_parm    = NULL;
	l_highest_parm = G_HTMLParmListHead;
	l_next_parm    = G_HTMLParmListHead;
	if (G_HTMLParmListHead != NULL) {
	  while (true) {
		//Serial.println(F("Highest Parm.."));
		if (l_next_parm->Next == NULL)
		  break;
		//
		if (l_highest_parm < l_next_parm->Next) {
		  l_prev_parm    = l_next_parm;
		  l_highest_parm = l_next_parm->Next;
		}
		l_next_parm = l_next_parm->Next;
	  } //Iterate to find highest node in the parm list
	} //No need to iterate if list is NULL

	//Iterate to find the current highest node in the cache
	l_prev_node    = NULL;
	l_highest_node = G_CacheHead;
	l_next_node    = G_CacheHead;
	if (G_CacheHead != NULL) {
	  while (true) {
		//Serial.println(F("Highest Node .."));
		if (l_next_node->Next == NULL)
		  break;
		//
		if (l_highest_node < l_next_node->Next) {
		  l_prev_node    = l_next_node;
		  l_highest_node = l_next_node->Next;
		}
		l_next_node = l_next_node->Next;
	  } //Iterate to find highest heap node
	} //No need to iterate if cache is NULL

	//Extract the current top of heap
	l_heap_end = (int)__brkval;

	if (((l_highest_parm != NULL) && (l_highest_node != NULL) && ((int) l_highest_parm < (int) l_highest_node)) ||
		(l_highest_parm == NULL)) {
	  //Cache node is higher

	  //Create a new copy cache node
	  l_next_node = new(TCacheItem);

	  //Copy data from original to new cache node
	  l_next_node->ItemData = l_highest_node->ItemData;
	  l_next_node->Type     = l_highest_node->Type;
	  //l_next_node->Next =

	  //If Top of Heap has expanded
	  if (l_heap_end < (int)__brkval) {
		//Delete copy cache node which ended up on top of the heap
		//Serial.println(F("Expand Exit .."));
		delete(l_next_node);
		break;
	  }

	  //Serial.println(F("Moving .."));
	  //Insert new node into cache list
	  if (l_prev_node == NULL)
		G_CacheHead = l_next_node;
	  else
		l_prev_node->Next = l_next_node;
	  //
	  l_next_node->Next = l_highest_node->Next;
	  if (l_next_node->Next == NULL)
		G_CacheTail = l_next_node;
	  //
	  //Delete previous highest node
	  delete(l_highest_node);

	  //If top of heap has not reduced
	  if (l_heap_end == (int)__brkval) {
		//Indicates deleted node was not at top of heap
		//Serial.println(F("Same Exit .."));
		break;
	  }
	} //process higher cache node
	else {//Parm node is higher

	  //Create a new copy parmnode
	  l_next_parm = new(THTMLParm);

	  //Copy data from original to new parm node
	  l_next_parm->Name  = l_highest_parm->Name;
	  l_next_parm->Value = l_highest_parm->Value;
	  //l_next_parm->Next =

	  //If Top of Heap has expanded
	  if (l_heap_end < (int)__brkval) {
		//Delete copy parm node which ended up on top of the heap
		//Serial.println(F("Expand Exit .."));
		delete(l_next_parm);
		break;
	  }

	  //Serial.println(F("Moving .."));
	  //Insert new parm node into linked list
	  if (l_prev_parm == NULL)
		G_HTMLParmListHead = l_next_parm;
	  else
		l_prev_parm->Next = l_next_parm;
	  //
	  l_next_parm->Next = l_highest_parm->Next;
	  //Delete previous highest node
	  delete(l_highest_parm);
	} //process higher parm node

	//If top of heap has not reduced
	if (l_heap_end == (int)__brkval) {
	  //Indicates deleted highest node was not at top of heap
	  //Serial.println(F("Same Exit .."));
	  break;
	}

	//Serial.print(F("PREV HEAP END: "));
	//Serial.println(l_heap_end);
	//Serial.print(F("NEW HEAP END: "));
	//Serial.println(l_heap_end);
  } //iterate highest cache nodes and move lower in the heap

  //l_heap_end = (int)__brkval;
  //if (l_heap_at_start != l_heap_end) {
  //  String l_msg = "HEAP: " +
  //			   String(l_heap_at_start) + "/" +
  //  			   String(l_heap_end) + "/" +
  //  			   String(l_heap_at_start - l_heap_end);
  //  ActivityWrite(l_msg);
  //}
  CheckRAM();
  Pop(c_proc_num);
} //HeapOptimisation
#endif

//----------------------------------------------------------------------------------------

String CharRepeat(const char p_char, const int p_length) {
//Create a String from a repeated single character
  const byte c_proc_num = 192;
  Push(c_proc_num);
  if (p_length <= 0) {
	CheckRAM();
	Pop(c_proc_num);
	return "";
  }
  String l_result;
  l_result.reserve(p_length + 1);
  l_result = "";
  for (int l_count = 0; l_count < p_length; l_count++)
	l_result += p_char;
  //
  CheckRAM();
  Pop(c_proc_num);
  return l_result;
}

//----------------------------------------------------------------------------------------
//END OF FILE
//------------------------------------------------------------------------------

