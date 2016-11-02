#include <math.h>
//http://forum.arduino.cc/index.php/topic,209133.0.html

double Thermister(int RawADC) {
  double Temp;
  Temp = log(((10240000/RawADC) - 10000));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
  Temp = Temp - 273.15;// Convert Kelvin to Celcius
  return Temp;
}

void setup() {
  Serial.begin(9600);
  Serial.println("KY_013thermistor01");
}

void loop() {
  Serial.print(Thermister(analogRead(0)));
  Serial.println("c");
  delay(500);
}

