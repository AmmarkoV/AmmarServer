#ifndef __SERIALCOMMUNICATION_H
#define __SERIALCOMMUNICATION_H

#define byte uint8_t
#define ON 1
#define OFF 0

#include <Arduino.h> 

class AmmBusUSBProtocol {
  
public:
int newUSBCommands(
                    byte * valvesState,
                    byte * valvesScheduled, 
                    byte * idleTicks
                  );

};

#endif
