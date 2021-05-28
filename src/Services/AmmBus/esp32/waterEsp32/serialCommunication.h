#ifndef __SERIALCOMMUNICATION_H
#define __SERIALCOMMUNICATION_H

#define byte uint8_t
#define ON 1
#define OFF 0

#include "DS3231.h"

class AmmBusUSBProtocol 
{
  
public:
int newUSBCommands(
                    byte * valvesState,
                    byte * valvesScheduled,
                    DS3231 * clock,
                    RTCDateTime * dt,
                    byte * idleTicks
                  );

};

#endif
