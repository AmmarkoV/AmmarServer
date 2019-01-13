 
// AmmBus Nano Version..!
#define RESET_CLOCK_ON_NEXT_COMPILATION 0 

//These are needed here for the IDE to understand which modules to import
#include <Wire.h>          
#include <etherShield.h>
//cd ~/Arduino/libraries/ &&  git clone https://github.com/kuba1999/etherShield.git
#include <ETHER_28J60.h>
//---------------------------------------------------------------------

#include "configuration.h"

#include "ethernetCommunication.h"
AmmBusEthernetProtocol ammBusEthernet;

#include "serialCommunication.h"
AmmBusUSBProtocol ammBusUSB;

#include "ammBus.h"
struct ammBusState ambs={0};

#include "timeCalculations.h" 

int idleCounter=0;

int lowestAcceptableTemperature=24;
int highestAcceptableTemperature=24;

                         
void setup()
{
  pinMode(pinR_LED, OUTPUT);  digitalWrite(pinR_LED, LOW); 
  pinMode(pinG_LED, OUTPUT);  digitalWrite(pinG_LED, LOW); 
  pinMode(pinB_LED, OUTPUT);  digitalWrite(pinB_LED, LOW);   
  
  pinMode(buttonPin, INPUT);  digitalWrite(buttonPin, HIGH);    
//-----------------------------------------------------------
 

//BUTTON ----------------------------------------------------- 
  Serial.begin(9600);
  
  Serial.print("Starting Up! \n");
  initializeAmmBusState(&ambs); 
  
   
   Serial.print("Starting Up Clock! \n");
  // clock.begin();

   // Set sketch compiling time
  // if (RESET_CLOCK_ON_NEXT_COMPILATION)
  //     { clock.setDateTime(__DATE__, __TIME__); } 
  // dt = clock.getDateTime();   
   
  Serial.print("Setting up ethernet @ ");
  Serial.print(ip[0]);   Serial.print("."); Serial.print(ip[1]);   Serial.print("."); Serial.print(ip[2]);   Serial.print("."); Serial.print(ip[3]); 
 
  Serial.print(":");
  Serial.print(port);
  Serial.print("\n");
  e.setup(mac, ip, port); 
  Serial.print("Ready \n");
}
  

void checkForEthernetInput()
{
  if (  
       ammBusEthernet.receiveEthernetRequests(&e,temperature,humidity)
     )
     {  
      //setRelayState(ambs.valvesState);
     }
}



void setLED(char R,char G,char B)
{
  if (R) { digitalWrite(pinR_LED,HIGH); } else { digitalWrite(pinR_LED,LOW); }
  if (G) { digitalWrite(pinG_LED,HIGH); } else { digitalWrite(pinG_LED,LOW); }
  if (B) { digitalWrite(pinB_LED,HIGH); } else { digitalWrite(pinB_LED,LOW); }
}


void readTemperature()
{
  //DHT requires sampling at 1Hz
  if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  { ambs.errorDetected=1; } 

  if (lowestAcceptableTemperature>temperature) 
  {
     setLED(0,0,1); //BLUE  
  }
   else
  if (highestAcceptableTemperature<temperature)
  {
     setLED(1,0,0); //RED    
  } else
  {
     setLED(0,1,0); //GREEN 
  } 
 
  Serial.print("Temperature:");
  Serial.print(temperature);
  Serial.print("oC\n");

  Serial.print("Humidity:");
  Serial.print(humidity);
  Serial.print("%\n");
   
}

void loop()
{ 
 delay(10);

 if (idleCounter%100==0)
 {
  readTemperature(); 
 }  
 ++idleCounter;  

 if (idleCounter==200)
 { 
  setLED(0,0,0);
  delay(500);
 } else
 if (idleCounter==300)
 { 
   idleCounter=0;
 }


  //BUTTON ----------------------------------------------------------------
 int buttonState = digitalRead(buttonPin);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) { 
                             // turn LED on:
                             setLED(1,0,1);
                            } 
  //-----------------------------------------------------------------------
   
   
  checkForEthernetInput(); 
}
