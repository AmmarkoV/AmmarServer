#include <SPI.h>
#include <Ethernet.h>

#include "configuration.h"

#include "webserver.h"


#include "serialCommunication.h"
AmmBusUSBProtocol ammBusUSB;

#include "ammBus.h"
struct ammBusState ambs={0};
 
 
unsigned int minimumTemperature=10; 
unsigned int maximumTemperature=25;

//DHT11 -----------------------------------------------------
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 9
#include "SimpleDHT.h"
int pinDHT11 = 9;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;

byte dataDHT11[40] = {0};
uint32_t lastDHT11SampleTime=0;
//-----------------------------------------------------------


//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------


void setup()
{
Serial.begin(9600); //Set rate for communicating with phone 

   Serial.print("Starting Up Clock! \n");
   clock.begin();

   // Set sketch compiling time
   if (RESET_CLOCK_ON_NEXT_COMPILATION)
       { clock.setDateTime(__DATE__, __TIME__); } 
   dt = clock.getDateTime();   

  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
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
  EthernetClient client = server.available();
  if (client) 
    {
      ServeWebServerClient(&client,ambs.temperature,ambs.humidity,ambs.tooHotCounter,ambs.tooColdCounter);
    }

    checkForSerialInput();
  
  if (dht11.read(pinDHT11, &ambs.temperature, &ambs.humidity, dataDHT11))  
   {
     //Serial.write("\nDHT11 error\n"); 
   } else
   { 
     Serial.print("DHT11 OK: ");
     Serial.print((int)ambs.temperature); Serial.print(" *C, ");
     Serial.print((int)ambs.humidity); Serial.println(" %");

     if (ambs.temperature<minimumTemperature) 
      { 
        if (ambs.tooColdCounter<10000) { ambs.tooColdCounter++; }
      }  

     if (ambs.temperature>maximumTemperature) 
      { 
        if (ambs.tooHotCounter<10000) { ambs.tooHotCounter++; }
      }   
      
   } 
   
   
}
