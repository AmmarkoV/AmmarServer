#include <SPI.h>
#include <Ethernet.h>

#include "configuration.h"

#include "webserver.h"
#include "email.h"

#include "serialCommunication.h"
AmmBusUSBProtocol ammBusUSB;

#include "ammBus.h"
struct ammBusState ambs={0};


void setup()
{
   Serial.begin(9600); //Set rate for communicating with phone 

   Serial.print(F("Clock\n"));
   clock.begin();

   // Set sketch compiling time
   if (RESET_CLOCK_ON_NEXT_COMPILATION)
              { clock.setDateTime(__DATE__, __TIME__); } 
   dt = clock.getDateTime();   

   Serial.println(F("WebServer"));
   // start the Ethernet connection and the server:
   Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) 
  {
    Serial.print(F("Check Ethernet"));
    Serial.println(F(" controller."));
    while (true) 
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  
  if (Ethernet.linkStatus() == LinkOFF) 
  {
    Serial.print(F("Check Ethernet"));
    Serial.println(F(" cable."));
  }

  // start the server
  server.begin();
  Serial.print(F("WebServer:"));
  Serial.println(Ethernet.localIP());
}


void setRelayState( byte * valves )
{ 
  int i=0;
  byte port ;
  for (i=0; i<NUMBER_OF_RELAYS_INSTALLED; i++) 
  {
     port=2+i;
     if (!valves[i] )  {  digitalWrite(port, HIGH);   } else
                       {  digitalWrite(port, LOW);   }
  }  
}


void checkForSerialInput()
{
  if ( 
       ammBusUSB.newUSBCommands(
                                ambs.valvesState,
                                ambs.valvesScheduled,
                                &clock,
                                &dt,
                                &ambs.idleTicks
                               ) 
     )
     {  
      setRelayState(ambs.valvesState);
     }
}


void loop() 
{
  dt = clock.getDateTime();
  EthernetClient client = server.available();
  if (client)
    {
      ServeWebServerClient(&ambs,&client,ambs.temperature,ambs.humidity,ambs.tooHotCounter,ambs.tooColdCounter);
    }

   checkForSerialInput();

  if (dht11.read(pinDHT11, &ambs.temperature, &ambs.humidity, dataDHT11))
   {
     //Serial.write("\nDHT11 error\n");
   } else
   {
     Serial.print(F("DHT11: "));
     Serial.print((int)ambs.temperature); Serial.print(F("*C "));
     Serial.print((int)ambs.humidity);    Serial.println(F("%"));

     byte sendMessage=0;
     if (ambs.temperature<minimumTemperature)
      {
        if (ambs.tooColdCounter<10000) { ambs.tooColdCounter++; }
        ++sendMessage;
      }

     if (ambs.temperature>maximumTemperature)
      {
        if (ambs.tooHotCounter<10000) { ambs.tooHotCounter++; }
        ++sendMessage;
      }

      if (sendMessage)
      {
        if (dt.unixtime - ambs.lastEmail < sendMailEveryXMinutes*60 )
        {
          Serial.println(F("No Spam.."));
        } else
        {
         // Instantiate the client with which to send email
         EthernetClient client;
         if (
             sendEmail(
                       &client,
                       dt.unixtime,
                       mailServer,
                       fromMail,
                       toMail
                      )
            )
            {
             Serial.println(F("E-Mail: done"));
             ambs.lastEmail=dt.unixtime;
            }
        }
      }
   }
}
