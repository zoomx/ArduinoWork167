/*
   NRF24SensorsRX


*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

float Meas[2];

RF24 radio(9, 10);
const uint64_t pipe = 0xE8E8F0F0E1LL;

void setup(void) {
  Serial.begin(115200);
  radio.begin();
  radio.openReadingPipe(1, pipe);
  radio.startListening();
}

void loop(void)
{
  if ( radio.available() )
  {
    while (radio.available()) {
      radio.read(Meas, sizeof(Meas));

      // done = radio.read(Meas, sizeof(Meas));

      Serial.print(Meas[0]);
      Serial.print(" ");
      Serial.println(Meas[1]);
    }
  }
}
