#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H



// RESET DS3231 --------------------------------------------
#define RESET_CLOCK_ON_NEXT_COMPILATION 0 
//----------------------------------------------------------


//Server ---------------------------------------------------
const int serverPort = 8080;

// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
//----------------------------------------------------------

//ESP32-Dev Onboard Indicator ------------------------------
const int ledPin = 2;
//----------------------------------------------------------

//WIFI -----------------------------------------------------
// Replace with your network credentials
const char* ssid = "VODAFONE_0365";
const char* password = "nikosnikos";
//-----------------------------------------------------------


//RELAY TO PORT RESOLUTION ----------------------------------
const int RELAY_ADDRESS[]={19,18,32,33,25,26,27,5};
const int NUMBER_OF_RELAYS = 8;

const int AC_RELAY = 1;
const int AC_RELAY_PORT = 7; //<- use last relay address for AC
//-----------------------------------------------------------


#include <Wire.h> 

//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 realtime_clock;
RTCDateTime dt;
//-----------------------------------------------------------



#endif
