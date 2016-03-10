
/*
 * NRF24SendString
 *
 * http://shanes.net/another-nrf24l01-sketch-string-sendreceive/
 *
 */



#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>


#define CE_PIN   9
#define CSN_PIN 10

/*
This sketch sends a string to a corresponding Arduino
with nrf24 attached.  It appends a specific value
(2 in this case) to the end to signify the end of the
message.
*/

int msg[1];
RF24 radio(CE_PIN, CSN_PIN);
const uint64_t pipe = 0xE8E8F0F0E1LL;
//const uint64_t pipe = 0xB00B1E5000LL; // Define the transmit pipe

void setup(void) {
  Serial.begin(9600);
  Serial.println("NRF24SendString");
  radio.begin();
  //radio.setAutoAck(1);                    // Ensure autoACK is enabled, this means rec send acknowledge packet to tell xmit that it got the packet with no problems
  //radio.enableAckPayload();               // Allow optional payload or message on ack packet
  //radio.setRetries(5, 15);                // Defines packet retry behavior: first arg is delay between retries at 250us x 5 and max no. of retries

  radio.openWritingPipe(pipe);
}
void loop(void) {
  String theMessage = "Hello there!";
  int messageSize = theMessage.length();
  for (int i = 0; i < messageSize; i++) {
    int charToSend[1];
    charToSend[0] = theMessage.charAt(i);
    radio.write(charToSend, 1);
  }
  //send the 'terminate string' value...
  msg[0] = 2;
  radio.write(msg, 1);
  delay(1000);
}
