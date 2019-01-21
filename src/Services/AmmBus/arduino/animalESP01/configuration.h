#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H
 

#define USE_DHCP 0
#define USE_DNS 0
#define USE_ENCRYPTION 0

#define NUMBER_OF_TEMPERATURE_SAMPLES 10
//#define REQUEST_RATE_NORMAL 5*60*1000 // minutes to milliseconds
//#define REQUEST_RATE_CRITICAL 60*1000 // milliseconds

static unsigned long REQUEST_RATE_NORMAL=600000;
static unsigned long REQUEST_RATE_CRITICAL=60000;

//ENCRYPTION -------------------
const char serialNumber[] = "0002";
const char publicKey[] = "xxxx";
//This is the default private key currently not used and has to be manually updated for each device ..  
static byte privateKey[16] = {0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D}; 
//------------------------------


//SERVER CONFIGURATION -------------------
char websiteIP[] = "139.91.185.16"; //"192.168.1.49"; 
#define hisPort 8087

const char websiteA[] PROGMEM = "ammar.gr";
const char websiteB[] PROGMEM = "spiti.ammar.gr";
const char * website = websiteA;
//------------------------------

//DHT11 ----------------------------------------------------- 
#include "DHTesp.h"
#define pinDHT11 2
float temperature;
float humidity;
DHTesp dht;
//-----------------------------------------------------------


#endif
