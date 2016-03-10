/* YourDuinoStarter Example: nRF24L01 Transmit Joystick values
 - WHAT IT DOES: Reads Analog values on A0, A1 and transmits
   them over a nRF24L01 Radio Link to another transceiver.
 - SEE the comments after "//" on each line below
 - CONNECTIONS: nRF24L01 Modules See:
 http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC 3.3V !!! NOT 5V
   3 - CE to Arduino pin 9
   4 - CSN to Arduino pin 10
   5 - SCK to Arduino pin 13
   6 - MOSI to Arduino pin 11
   7 - MISO to Arduino pin 12
   8 - UNUSED
   -
   Analog Joystick or two 10K potentiometers:
   GND to Arduino GND
   VCC to Arduino +5V
   X Pot to Arduino A0
   Y Pot to Arduino A1

 - V1.00 11/26/13
   Based on examples at http://www.bajdi.com/
   Questions: terry@yourduino.com */

/*-----( Import needed libraries )-----*/
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
/*-----( Declare Constants and Pin Numbers )-----*/
#define CE_PIN   9
#define CSN_PIN 10

// NOTE: the "LL" at the end of the constant is "LongLong" type
//const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe
const uint64_t pipe = 0xB00B1E5000LL; // Define the transmit pipe


/*-----( Declare objects )-----*/
RF24 radio(CE_PIN, CSN_PIN); // Create a Radio
/*-----( Declare Variables )-----*/
String Hello = "Hello from Radio 0";
int msg[1];

void setup()   /****** SETUP: RUNS ONCE ******/
{
  Serial.begin(9600);
  Serial.println("NRF24SayHEllo");
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled, this means rec send acknowledge packet to tell xmit that it got the packet with no problems
  radio.enableAckPayload();               // Allow optional payload or message on ack packet
  radio.setRetries(5, 15);                // Defines packet retry behavior: first arg is delay between retries at 250us x 5 and max no. of retries

  radio.openWritingPipe(pipe);
}//--(end setup )---


void loop()   /****** LOOP: RUNS CONSTANTLY ******/
{

  int messageSize = Hello.length();
  for (int i = 0; i < messageSize; i++) {
    int charToSend[1];
    charToSend[0] = Hello.charAt(i);
    radio.write(charToSend, 1);
    Serial.print(charToSend[0]);
  }
  //send the 'terminate string' value...
  msg[0] = 2;
  radio.write(msg, 1);
  radio.print("Hello");
  Serial.println();
  delay(1000);
}//--(end main loop )---

/*-----( Declare User-written Functions )-----*/

//NONE
//*********( THE END )***********
