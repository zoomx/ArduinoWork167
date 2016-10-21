/*
   Quad_Relay_Shield_test
   RelayShield by Catalex catalex.taobao.com
   V1.0 12/02/2013

   A simple test of the Catalex Relay Shiled v1.0 of 12/02/2013
   by Zoomx 20/10/2016

*/

#define Relay1pin 7
#define Relay2pin 6
#define Relay3pin 5
#define Relay4pin 4

#define ritardo 500

void setup() {
  Serial.begin(115200);
  Serial.println("Quad_Relay_Shield_test");

  pinMode(Relay1pin, OUTPUT);
  pinMode(Relay2pin, OUTPUT);
  pinMode(Relay3pin, OUTPUT);
  pinMode(Relay4pin, OUTPUT);

  digitalWrite(Relay1pin, LOW);
  digitalWrite(Relay2pin, LOW);
  digitalWrite(Relay3pin, LOW);
  digitalWrite(Relay4pin, LOW);
}

void loop() {
  digitalWrite(Relay1pin, HIGH);
  Serial.println("Relay 1");
  delay(ritardo);
  digitalWrite(Relay1pin, LOW);

  digitalWrite(Relay2pin, HIGH);
  Serial.println("Relay 2");
  delay(ritardo);
  digitalWrite(Relay2pin, LOW);

  digitalWrite(Relay3pin, HIGH);
  Serial.println("Relay 3");
  delay(ritardo);
  digitalWrite(Relay3pin, LOW);

  digitalWrite(Relay4pin, HIGH);
  Serial.println("Relay 4");
  delay(ritardo);
  digitalWrite(Relay4pin, LOW);
}
