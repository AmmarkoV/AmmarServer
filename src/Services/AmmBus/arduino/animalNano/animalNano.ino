#include <Arduino.h>

//Use 
//Arduino AVR -> Arduino Nano -> Processor AtMEGA 328P (Old Bootloader)
// 9600 BAUDS

#if USE_ENCRYPTION  
#include <AESLib.h>
//cd ~/Arduino/libraries && git clone https://github.com/DavyLandman/AESLib
#include <aes128_enc.h> 
#endif

#include "configuration.h"

#include <EtherCard.h> 
//cd ~/Arduino/libraries && git clone https://github.com/njh/EtherCard

unsigned int incorrectRequests=0;
char requestsPending=0;
char request[64];
byte Ethernet::buffer[ETHERNET_BUFFER];


static unsigned long timer=0;
static unsigned long timerTemp=0;
static unsigned long timerLedOn=0;
static unsigned long timerTest=0;
static unsigned long timerConnectionError=0;


char sTmp[6];
char sHum[6];

char criticalTemperature=0;
int lowestAcceptableTemperature=19;
int highestAcceptableTemperature=26;

float temperatureAvg=0.0;
float humidityAvg=0.0;
int   temperatureSum=0;
int   humiditySum=0;
int   numberOfDHT11Samples=0;


void(* resetArduino) (void) = 0; //declare reset function at address 0


void setLED(char R,char G,char B)
{
  if (R) { digitalWrite(pinR_LED,HIGH); } else { digitalWrite(pinR_LED,LOW); }
  if (G) { digitalWrite(pinG_LED,HIGH); } else { digitalWrite(pinG_LED,LOW); }
  if (B) { digitalWrite(pinB_LED,HIGH); } else { digitalWrite(pinB_LED,LOW); }
}

void notifyErrorNetwork()
{
  setLED(1,0,0); 
}

void notifyIntializing()
{
  setLED(0,1,1); 
}

void notifyTransmitting()
{
  setLED(1,0,1); 
}

void notifyTemperature()
{
 if (lowestAcceptableTemperature>temperatureAvg) 
     {
      setLED(0,0,1);  //BLUE   
     }
     else
  if (highestAcceptableTemperature<temperatureAvg)
     {
       setLED(1,0,0);  //RED    
     } else
     {
       setLED(0,1,0); //GREEN 
     } 
 timerLedOn=millis();     
}


void initializeEthernetClient()
{ 
  pinMode(pinR_LED, OUTPUT);  digitalWrite(pinR_LED, LOW); 
  pinMode(pinG_LED, OUTPUT);  digitalWrite(pinG_LED, LOW); 
  pinMode(pinB_LED, OUTPUT);  digitalWrite(pinB_LED, LOW);   
  
  notifyIntializing();
  pinMode(buttonPin, INPUT);  digitalWrite(buttonPin, HIGH);    
//-----------------------------------------------------------
 

 Serial.println(F("Initializing Ethernet Controller\n"));
 if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
 {
  Serial.println( "Failed to access Ethernet controller");
  notifyErrorNetwork();
  delay(1000);
 }


 #if USE_DHCP
    Serial.println(F("Attempting to get DHCP address"));
    notifyIntializing();
    if (ether.dhcpSetup()) 
         { Serial.println(F("DHCP success")); } 
     else
 #else
    { ether.staticSetup(myip,gwip,dnsip,subnet); }
 #endif

 

ether.printIp("My IP: ", ether.myip);
ether.printIp("Netmask: ", ether.netmask);
ether.printIp("GW IP: ", ether.gwip);
ether.printIp("DNS IP: ", ether.dnsip);
ether.printIp("SRV: ", ether.hisip);

Serial.println(F("\nWaiting for gateway..\n"));

timer=millis();
while (ether.clientWaitingGw()) 
    {  
      ether.packetLoop(ether.packetReceive()); 
      //If we can't find a gateway for 1 minute we throw a RED indication..
      if (millis() > timer + 6000) 
       {
         notifyErrorNetwork();
       }
       
      //If we can't find a gateway for 10 minutes we power cycle
      if (millis() > timer + 60000) 
       {
         Serial.println(F("Restarting.."));
         delay(1000);
         timer=millis();
         resetArduino(); //call reset  
       }
    } 
Serial.println(F("Gateway found"));


#if USE_DNS
  Serial.println(F("DNS Lookup")); 
  if(ether.dnsLookup (website, false))
   {
    Serial.println( "dnsLookup ok");
   } else
#else
   {
    #if USE_DNS
     Serial.println(F("DNSLookup failed"));
    #endif
    Serial.print(F("Using static target ip : ")); 
    Serial.println(websiteIP); 
    // if website is a string containing an IP address instead of a domain name,
    // then use it directly. Note: the string can not be in PROGMEM. 
    
    ether.parseIp(ether.hisip,websiteIP);
   } 
#endif




ether.printIp("My IP: ", ether.myip);
ether.printIp("Netmask: ", ether.netmask);
ether.printIp("GW IP: ", ether.gwip);
ether.printIp("DNS IP: ", ether.dnsip);
ether.printIp("SRV: ", ether.hisip);

//timer = - REQUEST_RATE_NORMAL; // start timing out right away
timer=0;
setLED(0,0,0);
}



 
void readTemperature(int coldRun)
{
  //DHT requires sampling at 1Hz
  if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  
   {  
    //Unable to read..
    setLED(1,1,1); 
   } 
    else
   { 
     temperatureSum+=temperature;
     humiditySum+=humidity;
     ++numberOfDHT11Samples;

     if (numberOfDHT11Samples==NUMBER_OF_TEMPERATURE_SAMPLES)
     {
      temperatureAvg=(float) temperatureSum/numberOfDHT11Samples;
      humidityAvg=(float) humiditySum/numberOfDHT11Samples;
      temperatureSum=0;
      humiditySum=0;
      numberOfDHT11Samples=0;   
    
      Serial.print(F("Temperature Avg: "));
      Serial.print(temperatureAvg);
      Serial.print(F("oC\n"));

      Serial.print(F("Humidity Avg: "));
      Serial.print(humidityAvg);
      Serial.print(F("%\n"));
          
      Serial.print(F("Time until next update:"));
      Serial.print(millis()-timer);
      Serial.print("/");
      
      if (criticalTemperature)  { Serial.println(REQUEST_RATE_CRITICAL); } else
                                { Serial.println(REQUEST_RATE_NORMAL); } 
      
      
      notifyTemperature();
     }

     //First cold run will set the average temperature to keep thermometer from freaking out.. 
     if (coldRun)
      {
       temperatureAvg=(float) temperature;
       humidityAvg=(float) humidity;    
      }
     
     criticalTemperature = 0;
     if ( (lowestAcceptableTemperature>temperatureAvg) || (highestAcceptableTemperature<temperatureAvg) )
     { 
      criticalTemperature=1;
     }

     
   /*
    Serial.print("Temperature:"); Serial.print(temperature); Serial.print("oC\n");
    Serial.print("Humidity:");    Serial.print(humidity);    Serial.print("%\n");*/
  } 
}



void setup () 
{
 requestsPending=0;

 for (int i=0; i<10; i++)
  {
    Serial.println(F("\n"));  
  }
       
 Serial.begin(9600);
 Serial.println(F("\nInitializing"));
 Serial.println(F("\nhttps://github.com/AmmarkoV/AmmarServer/blob/master/src/Services/AmmBus/arduino/animalNano\n"));
 
 initializeEthernetClient();
 
 //Initial thermometer reading before gathering a lot of samples..
 readTemperature(1);  //First cold run..
 Serial.println(F("Ready ..!\n\n\n"));
}


// called when the client request is complete
static void requestResult(byte status, word off, word len) 
{
  Serial.print(F("<<< reply reqst "));
  Serial.print(millis() - timer);
  Serial.println(" ms");
  Serial.println((const char*) Ethernet::buffer + off);
  if (requestsPending>0) { --requestsPending; }
}

// called when the client request is complete
static void testResult(byte status, word off, word len) 
{
  Serial.print(F("<<< reply test "));
  Serial.print(millis() - timerTest);
  Serial.println(" ms");
  Serial.println((const char*) Ethernet::buffer + off);
  if (requestsPending>0) { --requestsPending; }
}


void testTransmission()
{
  if (millis() > timerTest + 10000) 
     {
       notifyTransmitting();
       requestsPending=1;
       timerTest=millis();
       Serial.println(F("\n>>> TEST"));
       ether.hisport = hisPort;//to access local host 

       /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
       dtostrf(temperatureAvg, 4, 2, sTmp);
       dtostrf(humidityAvg, 4, 2, sHum);
       snprintf(request,64,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);

       #if USE_ENCRYPTION  
       aes128_enc_single(privateKey,request);
       #endif
       
       Serial.println(request);
       
       ether.browseUrl(PSTR("/test.html"), request , website, testResult);   
       //setLED(0,0,0);
     } else
     {
       setLED(1,1,1);  
     }
}

 

void loop () 
{
   //Keep ethernet working..
   ether.packetLoop(ether.packetReceive());


   ///----------------------------------------------------------------------------------
   //                               READ TEMPERATURE READINGS
   ///----------------------------------------------------------------------------------
   //Read DHT11 Sensor -----------------------  
    if (millis() > timerTemp + 1500) 
     {
      timerTemp = millis();
      readTemperature(0); 
     }  
   //-----------------------------------------
   if (millis() > timerLedOn + 100) 
     {
      setLED(0,0,0);  
     }
   //-----------------------------------------


   ///----------------------------------------------------------------------------------
   //                         NOTIFY ABOUT TEMPERATURE READINGS
   ///----------------------------------------------------------------------------------
   if (criticalTemperature)
    {
       if (millis() > timer + REQUEST_RATE_CRITICAL) 
        {
         notifyTransmitting();
         requestsPending=1;
         timer = millis();  
         Serial.println(F("\n>>> REQ_CRITICAL"));
         ether.hisport = hisPort;//to access local host  
         
         /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
         dtostrf(temperatureAvg, 4, 2, sTmp);
         dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,64,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);

         #if USE_ENCRYPTION  
          aes128_enc_single(privateKey,request);
         #endif
       
         Serial.println(request);
         
         ether.browseUrl(PSTR("/alarm.html"),request, website, requestResult);   
        }  
    } 
      else
    {
     if (millis() > timer + REQUEST_RATE_NORMAL) 
       {
        notifyTransmitting();
        requestsPending=1;
        timer = millis();     
        Serial.println(F("\n>>> REQ_NORMAL"));
        ether.hisport = hisPort;//to access local host 

         /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
         dtostrf(temperatureAvg, 4, 2, sTmp);
         dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,64,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);
         
         #if USE_ENCRYPTION  
          aes128_enc_single(privateKey,request);
         #endif
       
         Serial.println(request);
         
        ether.browseUrl(PSTR("/push.html"),request, website, requestResult); 
       } 
    }
   ///----------------------------------------------------------------------------------
      


  //BUTTON ----------------------------------------------------------------
  int buttonState = digitalRead(buttonPin);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) { 
                             // turn LED on:
                             testTransmission();
                            } 
  //-----------------------------------------------------------------------


   ///----------------------------------------------------------------------------------
   //                         NOTIFY ABOUT TEMPERATURE READINGS
   ///----------------------------------------------------------------------------------
  if (requestsPending)
   {
    notifyErrorNetwork();
   }
  if (requestsPending>1)
   {
    if (millis() > timerConnectionError + REQUEST_RATE_NORMAL) 
       { 
          ++incorrectRequests;
          Serial.print(F("Unable to contact server ( "));
          Serial.print(incorrectRequests);
          Serial.println(F(" )"));
          timerConnectionError = millis();
       }
      requestsPending=1;   
   }
   ///----------------------------------------------------------------------------------
}
