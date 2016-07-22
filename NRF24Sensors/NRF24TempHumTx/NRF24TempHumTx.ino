//http://www.elec-cafe.com/arduino-wireless-temperature-lcd-display-nrf24l01-dht11/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
//#include <DHT11.h>

int pin = A0;
//DHT11 dht11(pin);
float temperature[2];

double Fahrenheit(double celsius) {
  return ((double)(9 / 5) * celsius) + 32;
}

double Kelvin(double celsius) {
  return celsius + 273.15;
}

RF24 radio(9, 10);
const uint64_t pipe = 0xE8E8F0F0E1LL;

void setup(void) {
  radio.begin();
  radio.openWritingPipe(pipe);
}

void loop(void)
{
  float temp, humi;
  //dht11.read(humi, temp);
  humi = 50;
  temp = 25.6;
  temperature[0] = temp;
  temperature[1] = humi;
  radio.write(temperature, sizeof(temperature));
  delay(1000);
}
