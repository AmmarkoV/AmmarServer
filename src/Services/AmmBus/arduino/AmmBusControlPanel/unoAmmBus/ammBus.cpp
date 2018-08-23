#include "ammBus.h"
#include "timeCalculations.h"

void initializeAmmBusState(struct ammBusState * ambs)
{
  unsigned int i=0;
  
  for (i=0; i<NUMBER_OF_SWITCHES; i++)
  {
    //Set initial times..
    ambs->valvesTimesNormal[i]=30;
    ambs->valvesTimesHigh[i]=60;
    ambs->valvesTimesLow[i]=15;
    
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
 ambs->autopilotCreateNewJobs=0;
 ambs->runningWork=0;
 
 ambs->jobRunEveryXHours=5*24;
 ambs->jobRunAtXHour=20;
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
  ambs->autopilotCreateNewJobs;
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
   ambs->valvesScheduled[i]=1;
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



