#include "ammBus.h"

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



void ammBus_startValve(
                        struct ammBusState * ambs,
                        byte valveNumber,
                        byte minutesToLeaveItOpen
                       )
{

}


