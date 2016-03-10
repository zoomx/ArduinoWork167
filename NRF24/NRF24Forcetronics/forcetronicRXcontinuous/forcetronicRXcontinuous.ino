//This sketch is from a tutorial video for getting started with the nRF24L01 tranciever module on the ForceTronics YouTube Channel
//the code was leverage from Ping pair example at http://tmrh20.github.io/RF24/pingpair_ack_8ino-example.html
//This sketch is free to the public to use and modify at your own risk
//http://forcetronic.blogspot.it/2015/02/getting-started-with-nrf24l01.html

//REMOVED 10 limit packet

#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include "printf.h" //This is used to print the details of the nRF24 board. if you don't want to use it just comment out "printf_begin()"

const int pinCE = 9; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 10; //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
byte gotByte = 0; //used to store payload from transmit module
bool done = false;
RF24 wirelessSPI(pinCE, pinCSN); // Declare object from nRF24 library (Create your wireless SPI)
const uint64_t pAddress = 0xB00B1E5000LL;  //Create a pipe addresses for the 2 nodes to communicate over, the "LL" is for LongLong type

void setup()
{
  Serial.begin(57600);  //start serial to communicate process
  Serial.println("forcetronicRXcontinuous");
  printf_begin();  //This is only used to print details of nRF24 module, needs Printf.h file. It is optional and can be deleted
  wirelessSPI.begin();  //Start the nRF24 module
  wirelessSPI.setAutoAck(1);                    // Ensure autoACK is enabled, this means rec send acknowledge packet to tell xmit that it got the packet with no problems
  wirelessSPI.enableAckPayload();               // Allow optional payload or message on ack packet
  wirelessSPI.setRetries(5, 15);                // Defines packet retry behavior: first arg is delay between retries at 250us x 5 and max no. of retries
  wirelessSPI.openReadingPipe(1, pAddress);     //open pipe o for recieving meassages with pipe address
  wirelessSPI.startListening();                 // Start listening for messages
  wirelessSPI.printDetails();                   //print details of nRF24 module to serial, must have printf for it to print to serial
}

void loop()
{

  //loop until all of the payload data is recieved, for this example loop should only run once
  while (wirelessSPI.available()) {
    wirelessSPI.read( &gotByte, 1 ); //read one byte of data and store it in gotByte variable
    Serial.print("Recieved packet number: "); //payload counts packet number
    Serial.println(gotByte); //print payload / packet number
  }


  delay(200);
}
