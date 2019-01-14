#include <Arduino.h>

#include "configuration.h"

#include <EtherCard.h> 
//cd ~/Arduino/libraries && git clone https://github.com/njh/EtherCard

unsigned int incorrectRequests=0;
char requestsPending=0;
char request[64];
byte Ethernet::buffer[ETHERNET_BUFFER];


static long timer=0;
static long timerTemp=0;
static long timerLedOn=0;
static long timerTest=0;
static long timerConnectionError=0;


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


void(* resetArduino) (void) = 0;//declare reset function at address 0


void setLED(char R,char G,char B)
{
  if (R) { digitalWrite(pinR_LED,HIGH); } else { digitalWrite(pinR_LED,LOW); }
  if (G) { digitalWrite(pinG_LED,HIGH); } else { digitalWrite(pinG_LED,LOW); }
  if (B) { digitalWrite(pinB_LED,HIGH); } else { digitalWrite(pinB_LED,LOW); }
}

void notifyErrorNetwork()
{
  setLED(0,1,1); 
}

void notifyIntializing()
{
  setLED(0,1,1); 
}

void notifyTransmitting()
{
  setLED(1,0,1); 
}


void initializeEthernetClient()
{ 
  pinMode(pinR_LED, OUTPUT);  digitalWrite(pinR_LED, LOW); 
  pinMode(pinG_LED, OUTPUT);  digitalWrite(pinG_LED, LOW); 
  pinMode(pinB_LED, OUTPUT);  digitalWrite(pinB_LED, LOW);   
  
  notifyIntializing();
  pinMode(buttonPin, INPUT);  digitalWrite(buttonPin, HIGH);    
//-----------------------------------------------------------
 

 Serial.println("Initializing Ethernet Controller\n");
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
 
 
Serial.println("Waiting for gateway..");
timer=millis();
while (ether.clientWaitingGw()) 
    {  
      ether.packetLoop(ether.packetReceive()); 

      //If we can't find a gateway for 10 minutes we power cycle
      if (millis() > timer + 1*60*1000) 
       {
         timer=millis();
         resetArduino(); //call reset  
       }
    } 
Serial.println("Gateway found");


#if USE_DNS
  Serial.println( "DNS Lookup"); 
  if(ether.dnsLookup (website, false))
   {
    Serial.println( "dnsLookup ok");
   } else
#else
   {
    #if USE_DNS
     Serial.println( "dnsLookup failed");
    #endif
    Serial.print("Using static target ip : "); 
    Serial.println(websiteIP); 
    // if website is a string containing an IP address instead of a domain name,
    // then use it directly. Note: the string can not be in PROGMEM. 
    
    ether.parseIp(ether.hisip,websiteIP);
    //ether.hisip[0]=hisip[0];
    //ether.hisip[1]=hisip[1];
    //ether.hisip[2]=hisip[2];
    //ether.hisip[3]=hisip[3]; 
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
      temperatureAvg=temperatureSum/numberOfDHT11Samples;
      humidityAvg=humiditySum/numberOfDHT11Samples;
      temperatureSum=0;
      humiditySum=0;
      numberOfDHT11Samples=0;   
    
      Serial.print("Temperature Avg:");
      Serial.print(temperatureAvg);
      Serial.print("oC\n");

      Serial.print("Humidity Avg:");
      Serial.print(humidityAvg);
      Serial.print("%\n");
     }

     //First cold run will set the average temperature to keep thermometer from freaking out.. 
     if (coldRun)
      {
       temperatureAvg=(float) temperature;
       humidityAvg=(float) humidity;    
      }
     
     criticalTemperature = 0;
     if (lowestAcceptableTemperature>temperatureAvg) 
     {
      setLED(0,0,1);  //BLUE  
      criticalTemperature=1;
     }
     else
     if (highestAcceptableTemperature<temperatureAvg)
     {
       setLED(1,0,0);  //RED   
      criticalTemperature=1; 
     } else
     {
       setLED(0,1,0); //GREEN 
     } 
   
   /*
    Serial.print("Temperature:");
    Serial.print(temperature);
    Serial.print("oC\n");

    Serial.print("Humidity:");
    Serial.print(humidity);
    Serial.print("%\n");*/
  } 
}



void setup () 
{
 requestsPending=0;
       
 Serial.begin(9600);
 Serial.println("\nInitializing");
 initializeEthernetClient();
 
 //Initial thermometer reading before gathering a lot of samples..
 readTemperature(1);  //First cold run..
 Serial.println("Ready ..!\n\n\n");
}


// called when the client request is complete
static void requestResult(byte status, word off, word len) 
{
  Serial.print("<<< reply reqst ");
  Serial.print(millis() - timer);
  Serial.println(" ms");
  Serial.println((const char*) Ethernet::buffer + off);
  if (requestsPending>0) { --requestsPending; }
}

// called when the client request is complete
static void testResult(byte status, word off, word len) 
{
  Serial.print("<<< reply test ");
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
       Serial.println("\n>>> TEST");
       ether.hisport = hisPort;//to access local host 

       /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
       dtostrf(temperatureAvg, 4, 2, sTmp);
       dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,64,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);
         
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
      timerLedOn=timerTemp;
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
         Serial.println("\n>>> REQ_CRITICAL");
         ether.hisport = hisPort;//to access local host  
         
         /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
         dtostrf(temperatureAvg, 4, 2, sTmp);
         dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,64,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);
         
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
        Serial.println("\n>>> REQ_NORMAL");
        ether.hisport = hisPort;//to access local host 

         /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
         dtostrf(temperatureAvg, 4, 2, sTmp);
         dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,64,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);
         
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
          Serial.print("Unable to contact server ( ");
          Serial.print(incorrectRequests);
          Serial.println(" )");
          timerConnectionError = millis();
       }
      requestsPending=1;   
   }
   ///----------------------------------------------------------------------------------
     
}
