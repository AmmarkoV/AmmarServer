#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H


#include <Arduino.h>


//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------

 
//Ethernet -----------------------------------------------------
#include <etherShield.h>
#include <ETHER_28J60.h>
// Define MAC address and IP address - both should be unique in your network
static uint8_t mac[6] = {0x54, 0x55, 0x58, 0x10, 0x00, 0x24};  
static uint8_t ip[4] = {192, 168, 1, 179};
static uint16_t port = 80; // Use port 80 - the standard for HTTP  

ETHER_28J60 e;
//-----------------------------------------------------------     

#endif
