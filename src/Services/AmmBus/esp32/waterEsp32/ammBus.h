#ifndef AMMBUS_h
#define AMMBUS_h

#include <stdint.h>
#define byte uint8_t

#define RECENT_UNIX_TIME 1534944924
#define  NUMBER_OF_SWITCHES 8

#define LOW_TIME 15
#define NORMAL_TIME 30
#define HIGH_TIME 60

static const char * systemName =    { "AmmBus Waterchip" };
static const char * systemVersion = { "     v0.34      " };
static const char * valveLabels[] =
{
    "-",
    "-" ,
    "Mantra",
    "Tetragwna",
    "Trigwna",
    "Pisw      ",
    "-",
    "Metasximatistis      ", 
    //-----------------
    "Unknown"
};

static const char * valveSpeedLabels[] =
{
    "Normal",
    "Extra" ,
    "Fast", 
    //-----------------
    "Unknown"
};


struct ammBusState
{  
 uint32_t currentTime;
 
 byte valvesTimesNormal[NUMBER_OF_SWITCHES];
 byte valvesTimesHigh[NUMBER_OF_SWITCHES];
 byte valvesTimesLow[NUMBER_OF_SWITCHES];
 byte *armedTimes;
 byte *valvesTimes;
 
 byte valvesState[NUMBER_OF_SWITCHES];
 byte valvesScheduled[NUMBER_OF_SWITCHES];
 uint32_t valveStartedTimestamp[NUMBER_OF_SWITCHES];
 uint32_t valveStoppedTimestamp[NUMBER_OF_SWITCHES];

 byte ACRelayExists;
 byte ACRelayPort;
 
 uint32_t lastBootTime;

 byte errorDetected;
 byte idleTicks;

 byte powerSaving;
 byte autopilotCreateNewJobs; 
 
 uint32_t lastJobRunTimestamp;
 byte jobRunEveryXHours;
 byte jobRunAtXHour;
 byte jobRunAtXMinute;
 byte jobConcurrency;
};

byte ammBus_getRunningValves(struct ammBusState * ambs);
byte ammBus_getScheduledValves(struct ammBusState * ambs);


int ammBus_automaticTriggerStart(struct ammBusState * ambs,uint32_t unixtimestamp);

void initializeAmmBusState(struct ammBusState * ambs);

void ammBus_startValve(struct ammBusState * ambs,
                       byte valveNumber,
                       byte minutesToLeaveItOpen);
                       
byte ammBus_hasValveBeenOpenEnough( struct ammBusState * ambs , byte valveNumber);
void ammBus_stopValve(struct ammBusState * ambs,byte valveNumber);


void ammBus_enableAutopilot(struct ammBusState * ambs);
void ammBus_disableAutopilot(struct ammBusState * ambs);
byte ammBus_getAutopilotState(struct ammBusState * ambs);

void ammBus_scheduleAllValves(struct ammBusState * ambs);
void ammBus_stopAllValves(struct ammBusState * ambs);

#endif
