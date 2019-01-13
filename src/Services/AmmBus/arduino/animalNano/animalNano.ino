#include <Arduino.h>

#include "configuration.h"
#include <EtherCard.h>
//cd ~/Arduino/libraries && git clone https://codeload.github.com/njh/EtherCard/zip/master


char requestsPending=0;
char request[64];
byte Ethernet::buffer[ETHERNET_BUFFER];
static long timer;
static long timerTemp;
static long timerLedOn;
static long timerTest;

char criticalTemperature=0;
int lowestAcceptableTemperature=24;
int highestAcceptableTemperature=24;

void setLED(char R,char G,char B)
{
  if (R) { digitalWrite(pinR_LED,HIGH); } else { digitalWrite(pinR_LED,LOW); }
  if (G) { digitalWrite(pinG_LED,HIGH); } else { digitalWrite(pinG_LED,LOW); }
  if (B) { digitalWrite(pinB_LED,HIGH); } else { digitalWrite(pinB_LED,LOW); }
}


void initializeEthernetClient()
{ 
  pinMode(pinR_LED, OUTPUT);  digitalWrite(pinR_LED, LOW); 
  pinMode(pinG_LED, OUTPUT);  digitalWrite(pinG_LED, LOW); 
  pinMode(pinB_LED, OUTPUT);  digitalWrite(pinB_LED, LOW);   
  
  setLED(1,0,1);
  pinMode(buttonPin, INPUT);  digitalWrite(buttonPin, HIGH);    
//-----------------------------------------------------------
 

 Serial.println("\nInitializing Ethernet Controller\n");
 if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
 {
  Serial.println( "Failed to access Ethernet controller");
 }


 #if USE_DHCP
    if (!ether.dhcpSetup()) Serial.println(F("DHCP failed"));
 #else
    ether.staticSetup(myip, gwip,dnsip,subnet);
 #endif
 
 
#if USE_DNS
  Serial.println( "DNS Lookup"); 
  if(ether.dnsLookup (website, false))
   {
    Serial.println( "dnsLookup ok");
   } else
   {
    Serial.println( "dnsLookup faild");
   }
#else
  Serial.println( "Using static target ip"); 
  // if website is a string containing an IP address instead of a domain name,
  // then use it directly. Note: the string can not be in PROGMEM. 
  ether.parseIp(ether.hisip, websiteIP);
#endif



Serial.println("Listening for gateway.. ");
while (ether.clientWaitingGw()) {  ether.packetLoop(ether.packetReceive()); } 
Serial.println("Gateway found");

ether.printIp("My IP: ", ether.myip);
ether.printIp("Netmask: ", ether.netmask);
ether.printIp("GW IP: ", ether.gwip);
ether.printIp("DNS IP: ", ether.dnsip);
ether.printIp("SRV: ", ether.hisip);

timer = - REQUEST_RATE_NORMAL; // start timing out right away
  
}



 
void readTemperature(int activateLED)
{
  //DHT requires sampling at 1Hz
  if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  
   {  
    //Unable to read..
    setLED(1,1,1); 
   } 
    else
   { 
    criticalTemperature = 0;
    if (lowestAcceptableTemperature>temperature) 
    {
      if (activateLED) { setLED(0,0,1); } //BLUE  
      criticalTemperature=1;
    }
     else
    if (highestAcceptableTemperature<temperature)
    {
      if (activateLED) {  setLED(1,0,0); } //RED   
      criticalTemperature=1; 
    } else
    {
      if (activateLED) {  setLED(0,1,0); } //GREEN 
    } 
 
    Serial.print("Temperature:");
    Serial.print(temperature);
    Serial.print("oC\n");

    Serial.print("Humidity:");
    Serial.print(humidity);
    Serial.print("%\n");
  } 
}



void setup () 
{
 requestsPending=0;
       
 Serial.begin(9600);
 Serial.println("\nInitializing\n");
 initializeEthernetClient();
 delay(100);
 readTemperature(0);
}




void testTransmission()
{
  if (millis() > timerTest + 10000) 
     {
      timerTest=millis();
      setLED(1,0,1);
       ++requestsPending;
       Serial.println("\n>>> TEST");
       ether.hisport = hisPort;//to access local host 
       snprintf(request,64,"?s=%s&tmp=%u&hum=%u",serialNumber,temperature,humidity);
       ether.browseUrl(PSTR("/test.html"), request , website, requestResult);   
       setLED(0,0,0);
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
   //                         NOTIFY ABOUT TEMPERATURE READINGS
   ///----------------------------------------------------------------------------------
   if (criticalTemperature)
    {
       if (millis() > timer + REQUEST_RATE_CRITICAL) 
        {
         timer = millis();
         ++requestsPending;
         Serial.println("\n>>> REQ_CRITICAL");
         ether.hisport = hisPort;//to access local host  
         snprintf(request,64,"?s=%s&tmp=%u&hum=%u",serialNumber,temperature,humidity);
         ether.browseUrl(PSTR("/alarm.html"),request, website, requestResult);   
        }  
    } 
      else
    {
     if (millis() > timer + REQUEST_RATE_NORMAL) 
       {
        timer = millis();
        ++requestsPending;
        Serial.println("\n>>> REQ_NORMAL");
        ether.hisport = hisPort;//to access local host 
        snprintf(request,64,"?s=%s&tmp=%u&hum=%u",serialNumber,temperature,humidity); 
        ether.browseUrl(PSTR("/push.html"),request, website, requestResult); 
       } 
    }
   ///----------------------------------------------------------------------------------
      

   //Read DHT11 Sensor -----------------------  
    if (millis() > timerTemp + 1500) 
     {
      timerTemp = millis();
      timerLedOn=timerTemp;
      readTemperature(1); 
     }  
   //-----------------------------------------

   
   if (millis() > timerLedOn + 100) 
     {
      setLED(0,0,0);  
     }

  //BUTTON ----------------------------------------------------------------
  int buttonState = digitalRead(buttonPin);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) { 
                             // turn LED on:
                             testTransmission();
                            } 
  //-----------------------------------------------------------------------


  if (requestsPending)
   {
      Serial.println("Unable to send request..");
      setLED(1,1,0);  
      requestsPending=0;  
   }
}

// called when the client request is complete
static void requestResult(byte status, word off, word len) 
{
  Serial.print("<<< reply ");
  Serial.print(millis() - timer);
  Serial.println(" ms");
  Serial.println((const char*) Ethernet::buffer + off);
  if (requestsPending>0) { --requestsPending; }
}
