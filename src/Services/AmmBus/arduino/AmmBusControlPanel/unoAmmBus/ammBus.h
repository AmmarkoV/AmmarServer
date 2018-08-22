#ifndef AMMBUS_h
#define AMMBUS_h

#include <stdint.h>
#define byte uint8_t

#define RECENT_UNIX_TIME 1534944924

#define  NUMBER_OF_SWITCHES 8


static const char * systemName =    { "AmmBus Waterchip" };
static const char * systemVersion = { "     v0.31      " };
static const char * valveLabels[] =
{
    "Mikri Skala    ",
    "Leyland/Porta  " ,
    "Elato/Garage   ",
    "Triantafylla  ",
    "Agalma/Lemonia",
    "Gazon A      ",
    "Gazon B      ",
    "Gazon C      ", 
    //-----------------
    "Unknown"
};

static const char * valveSpeeds[] =
{
    "Normal",
    "Extra" ,
    "Fast", 
    //-----------------
    "Unknown"
};


struct ammBusState
{  
 byte valvesTimesNormal[NUMBER_OF_SWITCHES];
 byte valvesTimesHigh[NUMBER_OF_SWITCHES];
 byte valvesTimesLow[NUMBER_OF_SWITCHES];
 byte *armedTimes;
 byte *valvesTimes;

 byte valvesState[NUMBER_OF_SWITCHES];
 byte valvesScheduled[NUMBER_OF_SWITCHES];
 uint32_t valveStartedTimestamp[NUMBER_OF_SWITCHES];
 uint32_t valveStoppedTimestamp[NUMBER_OF_SWITCHES];

 uint32_t lastBootTime;

 byte errorDetected;
 byte idleTicks;

 byte powerSaving;
 byte autopilotCreateNewJobs;
 byte runningWork;
 
 byte jobRunEveryXHours;
 byte jobRunAtXHour;
 byte jobRunAtXMinute;
 byte jobConcurrency;
};

void initializeAmmBusState(struct ammBusState * ambs);

#endif
