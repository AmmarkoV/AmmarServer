// A simple web server that always just says "Hello World"

#include <etherShield.h>
#include <ETHER_28J60.h>

#define RESET_CLOCK_ON_NEXT_COMPILATION 0
#define USE_CLOCK 1


#if USE_CLOCK
#include <Wire.h>
//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------
#endif

byte ticks=0;
char onStr[10]={"?cmdX=on"};
char offStr[10]={"?cmdX=off"};

// Define MAC address and IP address - both should be unique in your network
static uint8_t mac[6] = {0x54, 0x55, 0x58, 0x10, 0x00, 0x24};  
static uint8_t ip[4] = {192, 168, 1, 179};
static uint16_t port = 80; // Use port 80 - the standard for HTTP                                    

ETHER_28J60 e;

void setup()
{
  pinMode(2, OUTPUT);  digitalWrite(2, HIGH); 
  pinMode(3, OUTPUT);  digitalWrite(3, HIGH); 
  pinMode(4, OUTPUT);  digitalWrite(4, HIGH);   
  pinMode(5, OUTPUT);  digitalWrite(5, HIGH);   
  pinMode(6, OUTPUT);  digitalWrite(6, HIGH);   
  pinMode(7, OUTPUT);  digitalWrite(7, HIGH); 
  pinMode(8, OUTPUT);  digitalWrite(8, HIGH); 
  pinMode(9, OUTPUT);  digitalWrite(9, HIGH); 
  
  Serial.begin(9600);
  
  Serial.print("Starting Up! \n");
  
  #if USE_CLOCK
   Serial.print("Starting Up Clock! \n");
   clock.begin();

   // Set sketch compiling time
   if (RESET_CLOCK_ON_NEXT_COMPILATION)
       { clock.setDateTime(__DATE__, __TIME__); }
    
   Serial.print("Getting Time! \n");
   dt = clock.getDateTime(); 
   
   Serial.print("Unixtime:                    ");
   Serial.println(dt.unixtime);
  #endif
  
  Serial.print(" ethernet.setup(mac, ip, port);\n"); 
  e.setup(mac, ip, port);
  
  Serial.print("Ready \n");
}



void sendPage()
{      
   byte i=0; 
   e.print("<H1>Web Remote</H1>");
      for (i=0; i<8; i++)
      { 
       onStr[4]  = 'a'+i; 
       offStr[4]  = 'a'+i; 
       e.print("<A HREF='");
       e.print(offStr);
       e.print("'>Turn Off</A>\&nbsp;");
       e.print("<A HREF='");
       e.print(onStr);
       e.print("'>Turn On</A><br>"); 
      }
    e.respond();
}









void loop()
{
  Serial.print(".");  
  if (ticks==10) 
   { 
    ticks=0;  
    Serial.print("t:");  
    dt = clock.getDateTime();
    Serial.println(dt.unixtime);
   } 
  
  
  char* params;
  if (params = e.serviceRequest())
  {
    Serial.print("Client");
    sendPage(); 
    
    byte i;
    byte port;
    for (i=0; i<8; i++)
      { 
       onStr[4]  = 'a'+i; 
       offStr[4]  = 'a'+i; 
       port=2+i;
       
       if (strcmp(params, offStr) == 0)
       {
        Serial.print(" Set ");
        //Serial.print(offStr);
        Serial.print(i);
        Serial.println(" Off");
        digitalWrite(port, HIGH); 
        Serial.println(" !");
       } else 
       if (strcmp(params, onStr) == 0) 
       {
        Serial.print("Set ");
        //Serial.print(onStr);
        Serial.print(i);
        Serial.println(" On");
        digitalWrite(port, LOW); 
        Serial.println(" !");
       }
      } 
  }
  ++ticks;
  delay(100);
}
