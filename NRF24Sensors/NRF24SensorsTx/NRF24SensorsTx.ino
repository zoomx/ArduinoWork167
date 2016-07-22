// NRF24SensorsTX
// Connect SCL to i2c clock - Analog 5
// Connect SDA to i2c data - Analog 4

// **** INCLUDES *****
//#include <Wire.h>
//#include <Adafruit_BMP085.h>
//#include <LowPower.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
//Adafruit_BMP085 bmp;

RF24 radio(9, 10);
const uint64_t pipe = 0xE8E8F0F0E1LL;

float Meas[2];

void setup()
{
  Serial.begin(115200);
  Serial.println("NRF24SensorsTX");
  //bmp.begin();
  radio.begin();
  radio.openWritingPipe(pipe);
}

void loop()
{
  // Enter power down state for 8 s with ADC and BOD module disabled
  //LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

  // Do something here
  // Example: Read sensor, data logging, data transmission.
  /*
    Serial.print(bmp.readTemperature());
    Serial.print(";");
    Serial.print(bmp.readPressure());
    Serial.print(";");
  */
  Serial.print(GetInternalTemp());
  Serial.print(";");
  Serial.println(readVcc());
  Meas[0] = GetInternalTemp();
  //radio.write(Temp, sizeof(Temp));
  //delay(2);
  Meas[1] = readVcc();
  radio.write(Meas, sizeof(Meas));
  blink();
  delay(2000);
}

void blink() {
  digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(50);              // wait for a second
  digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
  delay(50);              // wait for a second
}


double GetInternalTemp(void) {
  //http://playground.arduino.cc/Main/InternalTemperatureSensor
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

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA, ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31 ) / 1.22;

  // The returned temperature is in degrees Celsius.
  return (t);
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0) ;
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
