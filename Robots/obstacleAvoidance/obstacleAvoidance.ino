
int enA = 10;
int in1 = 9;
int in2 = 8;

int enB = 5;
int in3 = 7;
int in4 = 6;

int trigPin = 2;
int echoPin = 4;
long duration, cm, inches;

int redpin=A1;  
int greenpin=A2;      
int bluepin=A3;

void setup() {

  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

}

void forward()
{

  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);

  analogWrite(enA, 180);

  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);

  analogWrite(enB, 180);
}

void backward()
{

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  analogWrite(enA, 150);

  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);

  analogWrite(enB, 150);
  
}

void stop()
{
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);  
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void avoid()
{
  //STOP
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);  
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  //BACKUP
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 150);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  analogWrite(enB, 150);
  delay(1000);
  //TURN
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  analogWrite(enA, 140);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  analogWrite(enB, 140);
  delay(.001);
  
  
}

void RGB (int R, int G, int B){

    analogWrite(redpin,R);
    analogWrite(greenpin,G);
    analogWrite(bluepin,B);
}

void loop()
{
  RGB(0,0,255); //blue
  delay(1000);

  forward();

  int duration, distance, inch;
  digitalWrite(trigPin, HIGH);
  delay(100);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;
  inch = (distance * 0.3937);

  //Sets object detection distance to 4.5
  if (inch < 15) {
    RGB(255,0,0); //red
    avoid();
    delay(1000);
    forward();
    delay(2000);
   }

}
