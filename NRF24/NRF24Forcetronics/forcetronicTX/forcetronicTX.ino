//This sketch is from a tutorial video for getting started with the nRF24L01 tranciever module on the ForceTronics YouTube Channel
//the code was leverage from Ping pair example at http://tmrh20.github.io/RF24/pingpair_ack_8ino-example.html
//This sketch is free to the public to use and modify at your own risk
//http://forcetronic.blogspot.it/2015/02/getting-started-with-nrf24l01.html

#include <SPI.h> //Call SPI library so you can communicate with the nRF24L01+
#include <nRF24L01.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include <RF24.h> //nRF2401 libarary found at https://github.com/tmrh20/RF24/
#include "printf.h" //This is used to print the details of the nRF24 board. if you don't want to use it just comment out "printf_begin()"

const int pinCE = 9; //This pin is used to set the nRF24 to standby (0) or active mode (1)
const int pinCSN = 10; //This pin is used to tell the nRF24 whether the SPI communication is a command or message to send out
byte counter = 1; //used to count the packets sent
bool done = false; //used to know when to stop sending packets
RF24 wirelessSPI(pinCE, pinCSN); // Create your nRF24 object or wireless SPI connection
const uint64_t pAddress = 0xB00B1E5000LL;              // Radio pipe addresses for the 2 nodes to communicate.

void setup()
{
  Serial.begin(57600);   //start serial to communicate process
  Serial.println("ForcetronixTX");
  printf_begin();        //This is only used to print details of nRF24 module, needs Printf.h file. It is optional and can be deleted
  wirelessSPI.begin();            //Start the nRF24 module
  wirelessSPI.setAutoAck(1);                    // Ensure autoACK is enabled so rec sends ack packet to let you know it got the transmit packet payload
  wirelessSPI.enableAckPayload();               // Allow optional ack payloads
  wirelessSPI.setRetries(5, 15);                // Sets up retries and timing for packets that were not ack'd, current settings: smallest time between retries, max no. of retries
  wirelessSPI.openWritingPipe(pAddress);        // pipe address that we will communicate over, must be the same for each nRF24 module
  wirelessSPI.stopListening();
  wirelessSPI.printDetails();                   // Dump the configuration of the rf unit for debugging
}


void loop()
{

  if (!done) { //if we are not done yet
    Serial.print("Now send packet: ");
    Serial.println(counter); //serial print the packet number that is being sent
    unsigned long time1 = micros();  //start timer to measure round trip
    //send or write the packet to the rec nRF24 module. Arguments are the payload / variable address and size
    if (!wirelessSPI.write( &counter, 1 )) { //if the send fails let the user know over serial monitor
      Serial.println("packet delivery failed");
    }
    else { //if the send was successful
      unsigned long time2 = micros(); //get time new time
      time2 = time2 - time1; //calculate round trip time to send and get ack packet from rec module
      Serial.print("Time from message sent to recieve Ack packet: ");
      Serial.print(time2); //print the time to the serial monitor
      Serial.println(" microseconds");
      counter++; //up the packet count
    }

    //if the reciever sends payload in ack packet this while loop will get the payload data
    while (wirelessSPI.available() ) {
      char gotChars[5]; //create array to hold payload
      wirelessSPI.read( gotChars, 5); //read payload from ack packet
      Serial.print(gotChars[0]); //print each char from payload
      Serial.print(gotChars[1]);
      Serial.print(gotChars[2]);
      Serial.println(gotChars[3]);
      done = true; //the ack payload signals we are done
    }
  }

  delay(1000);
}
