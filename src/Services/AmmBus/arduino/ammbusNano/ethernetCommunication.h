#ifndef __ETHERNETCOMMUNICATION_H
#define __ETHERNETCOMMUNICATION_H

#define byte uint8_t
#define ON 1
#define OFF 0

#include <Arduino.h>
#include "DS3231.h"

class AmmBusUSBProtocol {
  
public:
int newUSBCommands(
                    byte * valvesState,
                    byte * valvesScheduled,
                    DS3231 *clock,
                    RTCDateTime * dt,
                    byte * idleTicks
                  );

};

#endif
