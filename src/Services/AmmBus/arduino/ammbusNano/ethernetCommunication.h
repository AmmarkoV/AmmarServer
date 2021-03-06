#ifndef __ETHERNETCOMMUNICATION_H
#define __ETHERNETCOMMUNICATION_H

#define byte uint8_t
#define ON 1
#define OFF 0

#include <Arduino.h>
#include <etherShield.h>
#include <ETHER_28J60.h>
#include "DS3231.h"

class AmmBusEthernetProtocol {
  
public:

int receiveEthernetRequests(ETHER_28J60 * e,
                            DS3231 *clock,
                            RTCDateTime * dt);

void sendPage(ETHER_28J60 * e); 

};

#endif
