/*
 XFPM-115kpa

 Reads an analog input of a XFPM-115kpa, maps the result to a range from 0 to 5000 millivolt
 and in Pa
 prints the results to the serial monitor.

 by Zoomx
 2015/04/09

 */

// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = A0;  // Analog input pin that the  XFPM-115kpa is attached to

int sensorValue = 0;        // value read from the XFPM-115kpa
int Millivolt = 0;        //
long Pressure = 0;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  Serial.println("XFPM-115kpa");
}

void loop() {
  // read the analog in value:
  sensorValue = analogRead(analogInPin);
  // map it to the range of the analog out:
  Millivolt = map(sensorValue, 0, 1023, 0, 5000);     //5000 has to be checked
                                                      //It is the Vcc*1000
                                                      //May be 4900, for example.
  // map it to the Pressure value in Pa(see datasheet!):
  Pressure = map(Millivolt, 200, 4700, 15000, 115000);	//Values taken from the datasheet

  // print the results to the serial monitor:
  Serial.print("count = " );
  Serial.print(sensorValue);
  Serial.print("\t mV = ");
  Serial.print(Millivolt);
  Serial.print("\t P(Pa) = ");
  Serial.println(Pressure);
  // wait 20 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(20);
}
