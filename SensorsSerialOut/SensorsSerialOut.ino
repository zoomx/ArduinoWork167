/*

  SensorsSerialOut01d
  Get data from sensor and print them on serial

  Sensors
  DS1820, DHT11, internal temperature

  2014 07 31
  From the original DS1028SerialOut5 for Energia

  2014 09 11
  Little corrections

  2016 04 28
  changed OneWire, DHT and analog pins

  2016 10 07
  Added mean on internal temperature

  2016 10 26
  Added output of Internal temperature ADC integer
  to extimate better coefficients.

  2016 10 27
  Changed intADC from long to float

  2016 11 02
  Added new coefficients for internal temperature after some days
  of measures
*/


//LIBRARIES
#include <DallasTemperature.h>
#include <OneWire.h>
#include "dht11.h"

#define INTERVAL 15000  //usually 10000

#define LED 13
#define LDR A1


//OneWire DS1820
//#define OWPIN  9
#define ONE_WIRE_BUS 3
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

//DHT11
#define DHTPIN 2
dht11 DHT11;

int errore;
float intADC;

void setup(void) {
  Serial.begin(9600);
  delay(500);
  Serial.print("Sensors Serial 01e mean\n");
  Serial.println(DHT11LIB_VERSION);
  // Start up the library ds1820
  sensors.begin();

  delay(500);
  pinMode(LED, OUTPUT);
  blink;
}

void loop(void) {
  Serial.print(GetTemp(), 1);
  Serial.print(";");
  sensors.requestTemperatures();		 // Send the command to get temperatures
  Serial.print(sensors.getTempCByIndex(0));
  Serial.print(";");
  errore = DHT11.read(DHTPIN);
  switch (errore)
  {
    case DHTLIB_OK:
      //Serial.println("OK");
      Serial.print((float)DHT11.temperature, 2);
      Serial.print(";");
      Serial.print((float)DHT11.humidity, 2);
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("C-err;C-err"); //Checksum error
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("T-err;T-err"); //Time out error
      break;
    default:
      Serial.print("Err;Err"); //Unknown error
      break;
  }
  Serial.print(";");
  Serial.print(1024 - analogRead(LDR));
  Serial.print(";");
  Serial.print(intADC);
  Serial.println();
  delay(INTERVAL);
  blink();
}

void blink() {
  digitalWrite(LED, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for a second
  digitalWrite(LED, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait for a second
}

double GetTemp(void)
{
  unsigned int wADC;
  double t;

  // The internal temperature has to be used
  // with the internal reference of 1.1V.
  // Channel 8 can not be selected with
  // the analogRead function yet.

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  t = 0;

  intADC = 0; //!!!!!!!! Only to evaluate coefficients!!!!

  for (int i = 0; i <= 25; i++) {

    ADCSRA |= _BV(ADSC);  // Start the ADC

    // Detect end-of-conversion
    while (bit_is_set(ADCSRA, ADSC));

    // Reading register "ADCW" takes care of how to read ADCL and ADCH.
    wADC = ADCW;

    intADC += wADC;
    // The offset of 324.31 could be wrong. It is just an indication.
    //t += (wADC - 324.31 ) / 1.22;
    //t += (wADC * 0.731 - 256.42);  //get a T too low
    //t += (wADC - 256.42 ) / 1.367989; //get a T too high
  }
  //t = t / 25;
  intADC = intADC / 25;  //!!!!!!!! Only to evaluate coefficients!!!! (but used anyway!!!)
  // The returned temperature is in degrees Celsius.
  t = intADC * 0.731 - 256.42;
  return (t);
}
