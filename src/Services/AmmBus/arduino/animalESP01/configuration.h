#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H
 

#define USE_DHCP 0
#define USE_DNS 0
#define USE_ENCRYPTION 0

#define NUMBER_OF_TEMPERATURE_SAMPLES 10
//#define REQUEST_RATE_NORMAL 5*60*1000 // minutes to milliseconds
//#define REQUEST_RATE_CRITICAL 60*1000 // milliseconds
#define EXPECTED_RESPONSE_LATENCY_MILLISECONDS 80

//Given that the rate for a DHT11 sensor is 1.1 Hz and that we now reset count on success 
//the number of failed attempts should be 1 per loop thus restart in one hour from consistent failures  
#define NUMBER_OF_FAILED_ATTEMPTS_TO_RESET 60

static unsigned long REQUEST_RATE_NORMAL=600000;
static unsigned long REQUEST_RATE_CRITICAL=60000;

static const char ssid[]     = "hol - NetFasteR WLAN 3"; // Your ssid
static const char password[] = "opsec:P"; // Your Password 
//static const char ssid[]     = "AmmarNetKriti"; // Your ssid
//static const char password[] = "opsec:P"; // Your Password Spacepapo9

static const char deviceName[] = "animalESP01-0002"; //000559567C12-9681

//ENCRYPTION -------------------
const char serialNumber[] = "0002";
const char publicKey[] = "xxxx";
//This is the default private key currently not used and has to be manually updated for each device ..  
static byte privateKey[16] = {0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D}; 
//------------------------------


//NETWORK CONFIGURATION ................
#define STATIC_IP 1
#if STATIC_IP 
  IPAddress ip(147,52,74,88);  
  IPAddress gateway(147,52,74, 1);  
  IPAddress dns(147,52,72,200);  
  IPAddress subnet(255,255,0,0);  
//  IPAddress ip(192,168,1,99);  
//  IPAddress gateway(192,168,1, 20);  
//  IPAddress dns(192,168,1,20);  
//  IPAddress subnet(255,255,255,0);  
#endif


//SERVER CONFIGURATION -------------------
static char websiteIP[] = "147.52.74.57"; //"192.168.1.49"; 
#define hisPort 8087

static const char websiteA[] PROGMEM = "147.52.74.57";
static const char websiteB[] PROGMEM = "spiti.ammar.gr";
static const char * website = websiteA;
//------------------------------

//DHT11 ----------------------------------------------------- 
#include "DHTesp.h"
#define pinDHT11 2
float temperature;
float humidity;
DHTesp dht;
//-----------------------------------------------------------


#endif
