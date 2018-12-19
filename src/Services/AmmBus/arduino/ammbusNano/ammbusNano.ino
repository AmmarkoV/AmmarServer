 
// AmmBus Nano Version..!
#define RESET_CLOCK_ON_NEXT_COMPILATION 0 

//These are needed here for the IDE to understand which modules to import
#include <Wire.h>          
#include <etherShield.h>
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
  initializeAmmBusState(&ambs); 
  
   
   Serial.print("Starting Up Clock! \n");
   clock.begin();

   // Set sketch compiling time
   if (RESET_CLOCK_ON_NEXT_COMPILATION)
       { clock.setDateTime(__DATE__, __TIME__); } 
   dt = clock.getDateTime();   
   
  e.setup(mac, ip, port); 
  Serial.print("Ready \n");
}

void setRelayState( byte * valves )
{ 
  int i=0;
  byte port ;
  for (i=0; i<8; i++) 
  {
     port=2+i;
    if (!valves[i] )  {  digitalWrite(port, HIGH);   } else
                      {  digitalWrite(port, LOW);   }
  }  
}




void valveAutopilot()
{
  unsigned int changes=0;
  unsigned int i=0;
  byte valvesRunning=ammBus_getRunningValves(&ambs);
  byte valvesRemaining=ammBus_getScheduledValves(&ambs);
  
   
  if (valvesRunning>0)
  {  
   //Check if a valve needs to be closed.. 
   for (i=0; i<NUMBER_OF_SWITCHES; i++)
   {
    if (ambs.valvesState[i])
    {
      //This valve is running, should it stop?  
      unsigned int runningTime =  getValveRunningTimeMinutes(
                                                             ambs.valveStartedTimestamp[i],
                                                             ambs.valvesState[i],
                                                             dt.unixtime  
                                                            );
      if ( runningTime > ambs.valvesTimes[i] ) 
         {
           //This valve has run its course so we stop it
           ammBus_stopValve(&ambs,i);
           ++changes;
         }
    }
   }
  }

  //If autopilotCreateNewJobs is set
  if (ammBus_getAutopilotState(&ambs))
  {
   //Check which valves should be scheduled for start
   for (i=0; i<NUMBER_OF_SWITCHES; i++)
    {
        if (
             getValveOfflineTimeSeconds(
                                        ambs.valveStoppedTimestamp[i],
                                        ambs.valvesState[i],
                                        dt.unixtime
                                       ) 
                                        >  
              getTimeAfterWhichWeNeedToReactivateValveInSeconds(
                                                                ambs.jobRunEveryXHours
                                                               ) 
            )
           {  
              if (
                   currentTimeIsCloseEnoughToHourMinute(
                                                        dt.unixtime,
                                                        ambs.jobRunAtXHour,
                                                        ambs.jobRunAtXMinute
                                                       )
                 )
              {
                ambs.valvesScheduled[i]=1;
              }
           }
    }

  //Recount valve status  
  valvesRunning=ammBus_getRunningValves(&ambs);
  valvesRemaining=ammBus_getScheduledValves(&ambs);

  //We can accomodate more jobs..
  if ( (valvesRunning<ambs.jobConcurrency) && (valvesRemaining>0)  )
  {
    for (i=0; i<NUMBER_OF_SWITCHES; i++)
    {
      //Open first possible scheduled valve 
      if ( (ambs.valvesScheduled[i]) && (!ambs.valvesState[i]) && (valvesRunning<ambs.jobConcurrency) )
      {    
            ++changes;
            ++valvesRunning;
            ambs.valvesState[i]=1;
            ambs.valveStartedTimestamp[i]=dt.unixtime; 
      }
    }
  }
  }

  //Changes made/update relays  
  if (changes) 
    { setRelayState(ambs.valvesState); }
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



void checkForEthernetInput()
{
  if (  
       ammBusEthernet.receiveEthernetRequests(&e,
                                              &clock,
                                              &dt)
     )
     {  
      //setRelayState(ambs.valvesState);
     }
}



void loop()
{ 
  if (ambs.idleTicks==100) 
   { 
    ambs.idleTicks=0;   
    dt = clock.getDateTime();  
    checkForSerialInput();
    //Valve Autopilot..
    valveAutopilot(); 
   } 
   
  checkForEthernetInput(); 
  
  ++ambs.idleTicks;
  delay(10);
}
