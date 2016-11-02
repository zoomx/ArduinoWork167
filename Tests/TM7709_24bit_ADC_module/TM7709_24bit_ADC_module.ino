
//TM7709 24bit ADC module arduino sketch
//coldtears electronics

//default connection arduino UNO
//DRDY = 11		RDY on the board
//ADIO = 12		ADIO on the board
//SCLK = 13		CLK on the board


#define TM7710_DRDY        3          
#define TM7710_ADIO        4          
#define TM7710_ADIO_OUT()  DDRB|=1<<4     
#define TM7710_ADIO_IN()   DDRB&=~(1<<4)  
#define Set_TM7710_SCLK()  PORTB|=1<<5
#define Set_TM7710_ADIO()  PORTB|=1<<4
#define Clr_TM7710_SCLK()  PORTB&=~(1<<5)
#define Clr_TM7710_ADIO()  PORTB&=~(1<<4)

unsigned char x[3];
long Result;
float vref=4.89;

void TM7710_start(void)   
{
    Clr_TM7710_ADIO();
    delayMicroseconds(1);
    Clr_TM7710_SCLK();
    delayMicroseconds(1);
}

void TM7710_stop(void)    
{
    Clr_TM7710_ADIO();
    delayMicroseconds(1);
    Set_TM7710_SCLK();
    delayMicroseconds(1);
    Set_TM7710_ADIO();
    delayMicroseconds(1);
}
void TM7710_write(unsigned char dd)
{
    unsigned char i; 

    for(i=8;i>0;i--)
    {
        if(dd&0x80)
            Set_TM7710_ADIO();   
        else
            Clr_TM7710_ADIO();   
        
        delayMicroseconds(1);
        Set_TM7710_SCLK();       
        delayMicroseconds(1);
        Clr_TM7710_SCLK();      
        dd<<=1;                  
    }
}

unsigned char TM7710_read(void)
{
    unsigned char data=0,i; 

    for(i=0;i<8;i++)
    {
        Set_TM7710_SCLK();                   
        
        data=data<<1;                                  
        if((PINB&(1<<TM7710_ADIO))==(1<<TM7710_ADIO))  
        {
          data=data+1;
        }
        delayMicroseconds(1);
        Clr_TM7710_SCLK();
         delayMicroseconds(1);
    }
    return data;
}
void TM7710_Init()
{
   TM7710_ADIO_OUT();
   delay(100);
   TM7710_stop();
   TM7710_start();
   TM7710_write(0xBF);        
   TM7710_write(0x20);      //Gain=128
   //TM7710_write(0x00); //Gain=16
   TM7710_stop();      
}
void setup() 
{
DDRB|=1<<5 ;
delay(1000);
Serial.begin(9600);
Serial.println("TM7709 24bit ADC Module");
TM7710_Init();
}
    
void loop()
{  
   while((PINB&(1<<TM7710_DRDY))==(1<<TM7710_DRDY)); 
   TM7710_start();
   TM7710_write(0x7F);        
   TM7710_ADIO_IN();          
   for(unsigned char j=0;j<3;j++)
   {
   x[j]=TM7710_read();
   }
   TM7710_ADIO_OUT();         
   TM7710_stop();
   Result=x[0];
   Result = Result * 256;
   Result = Result + x[1];
   Result = Result * 256;
   Result = Result + x[2];
   Result = Result - 6912000;
   
   double volt = Result * vref /16 / 6912000; 
   printFloat(volt,6); 
   Serial.println(" V  ");

}


 void printFloat(float value, int places) {
 // this is used to cast digits
 int digit;
 float tens = 0.1;
 int tenscount = 0;
 int i;
 float tempfloat = value;

 // if value is negative, set tempfloat to the abs value

   // make sure we round properly. this could use pow from
 //<math.h>, but doesn't seem worth the import
 // if this rounding step isn't here, the value  54.321 prints as

 // calculate rounding term d:   0.5/pow(10,places)
 float d = 0.5;
 if (value < 0)
   d *= -1.0;
 // divide by ten for each decimal place
 for (i = 0; i < places; i++)
   d/= 10.0;
 // this small addition, combined with truncation will round our

 tempfloat +=  d;

 if (value < 0)
   tempfloat *= -1.0;
 while ((tens * 10.0) <= tempfloat) {
   tens *= 10.0;
   tenscount += 1;
 }

 // write out the negative if needed
 if (value < 0)
   Serial.print('-');

 if (tenscount == 0)
   Serial.print(0, DEC);

 for (i=0; i< tenscount; i++) {
   digit = (int) (tempfloat/tens);
   Serial.print(digit, DEC);
   tempfloat = tempfloat - ((float)digit * tens);
   tens /= 10.0;
 }

 // if no places after decimal, stop now and return
 if (places <= 0)
   return;

 // otherwise, write the point and continue on
 Serial.print('.');

 for (i = 0; i < places; i++) {
   tempfloat *= 10.0;
   digit = (int) tempfloat;
   Serial.print(digit,DEC);
   // once written, subtract off that digit
   tempfloat = tempfloat - (float) digit;
 }
 
  }





