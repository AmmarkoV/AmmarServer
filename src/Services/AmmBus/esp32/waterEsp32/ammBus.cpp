#include "ammBus.h"
#include "timeCalculations.h"


void initializeAmmBusState(struct ammBusState * ambs)
{
  unsigned int i=0;
  
  for (i=0; i<NUMBER_OF_SWITCHES; i++)
  {
    //Set initial times..
    ambs->valvesTimesNormal[i]=NORMAL_TIME;
    ambs->valvesTimesHigh[i]=HIGH_TIME;
    ambs->valvesTimesLow[i]=LOW_TIME;
    
    //Zero everything else
    ambs->valvesState[i]=0;
    ambs->valvesScheduled[i]=0;
    ambs->valveStartedTimestamp[i]=0;
    ambs->valveStoppedTimestamp[i]=0;
  }
   
 ambs->armedTimes = 0;
 ambs->valvesTimes = ambs->valvesTimesNormal;
  

 ambs->lastBootTime=0; 
 ambs->errorDetected=0;
 ambs->idleTicks=0;

 ambs->powerSaving=1;
 ambs->autopilotCreateNewJobs=1; 
 
 ambs->jobRunEveryXHours=1*24;
 ambs->jobRunAtXHour=6;
 ambs->jobRunAtXMinute=0;
 
 //Max concurrent jobs  
 ambs->jobConcurrency=1;   
}

//---------------------------------------------------------
byte ammBus_getRunningValves(struct ammBusState * ambs)
{
  unsigned int i=0;
  byte valvesRunning=0;
  for (i=0; i<NUMBER_OF_SWITCHES; i++)
  {
    if (ambs->valvesState[i])     {++valvesRunning;}  
  }
  
  return valvesRunning;
}
//---------------------------------------------------------
byte ammBus_getScheduledValves(struct ammBusState * ambs)
{
  unsigned int i=0;
  byte valvesScheduled=0;
  for (i=0; i<NUMBER_OF_SWITCHES; i++)
  { 
    if (ambs->valvesScheduled[i]) {++valvesScheduled;} 
  }
  
  return valvesScheduled;
}
//---------------------------------------------------------




int ammBus_automaticTriggerStart(struct ammBusState * ambs,uint32_t unixtimestamp)
{
  if (ammBus_getAutopilotState(ambs))
  {
    uint32_t elapsedTime = unixtimestamp - ambs->lastJobRunTimestamp;
    if (ambs->jobRunEveryXHours * 60 * 60 <= elapsedTime )
    {
       byte seconds,minutes,hours,day,month;
       unsigned short year;
      
      //Enough time has elapsed..!
      unixtimeToDate(
                     unixtimestamp,
                     &seconds,
                     &minutes,
                     &hours,
                     &day,
                     &month,
                     &year
                    );

      if ( 
            ( hours == ambs->jobRunAtXHour) && 
            ( minutes == ambs->jobRunAtXMinute)
         )
      {
       ambs->lastJobRunTimestamp = unixtimestamp;
       ammBus_scheduleAllValves(ambs);
      }

    }


    return 1;
  }

  return 0;
}





void ammBus_startValve(
                        struct ammBusState * ambs,
                        byte valveNumber,
                        byte minutesToLeaveItOpen
                       )
{
    ambs->valvesScheduled[valveNumber]=1;
    ambs->valvesTimes[valveNumber]=minutesToLeaveItOpen; 
    ambs->valvesState[valveNumber]=1;
    ambs->valveStartedTimestamp[valveNumber]=ambs->currentTime; 
}


void ammBus_scheduleValve(
                        struct ammBusState * ambs,
                        byte valveNumber,
                        byte minutesToLeaveItOpen
                       )
{
    ambs->valvesScheduled[valveNumber]=1;
    ambs->valvesTimes[valveNumber]=minutesToLeaveItOpen; 
}

byte ammBus_hasValveBeenOpenEnough( struct ammBusState * ambs , byte valveNumber)
{
   unsigned int runningTime =  getValveRunningTimeMinutes(
                                                           ambs->valveStartedTimestamp[valveNumber],
                                                           ambs->valvesState[valveNumber],
                                                           ambs->currentTime  
                                                          );
  if ( runningTime > ambs->valvesTimes[valveNumber] )  { return 1; }
  return 0;       
}



void ammBus_stopValve(
                        struct ammBusState * ambs,
                        byte valveNumber 
                       )
{
    ambs->valvesScheduled[valveNumber]=0; 
    ambs->valvesState[valveNumber]=0;
    ambs->valveStoppedTimestamp[valveNumber]=ambs->currentTime; 
}


void ammBus_enableAutopilot(struct ammBusState * ambs)
{
  ambs->autopilotCreateNewJobs=1;
}


void ammBus_disableAutopilot(struct ammBusState * ambs)
{
  ambs->autopilotCreateNewJobs=0;
}

byte ammBus_getAutopilotState(struct ammBusState * ambs)
{
  return ambs->autopilotCreateNewJobs;
}




void ammBus_scheduleAllValves(struct ammBusState * ambs)
{
  unsigned int i=0; 
  for (i=0; i<NUMBER_OF_SWITCHES; i++)
  {
   byte valveIsNotUsedInThisSetup = (valveLabels[i][0]=='-');
   if(!valveIsNotUsedInThisSetup) 
    {
     if ( (ambs->ACRelayExists) && (i==ambs->ACRelayPort) )
     {
      //Skip setting auto relay port..
     } else
     {
      ambs->valvesScheduled[i]=1; 
     }
    }
  } 
}


void ammBus_stopAllValves(struct ammBusState * ambs)
{
  unsigned int i=0; 
  for (i=0; i<NUMBER_OF_SWITCHES; i++)
  {
    ambs->valvesState[i]=0; 
    ambs->valvesScheduled[i]=0;  
  } 
 ambs->armedTimes=0;
}
