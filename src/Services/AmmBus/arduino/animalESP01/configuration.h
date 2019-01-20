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
const char serialNumber[] = "0001";
const char publicKey[] = "xxxx";
//This is the default private key currently not used and has to be manually updated for each device ..  
static byte privateKey[16] = {0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D}; 
//------------------------------


//ETHERNET CONFIGURATION -------------------
#define ETHERNET_BUFFER 420 //bytes 
// ethernet interface mac address
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x33 };

//Home
//static byte myip[] = { 192,168,1,179 };
//static byte gwip[] = { 192,168,1,20 }; //ip route | grep default
//static byte dnsip[] = { 192,168,1,20 };
//static byte subnet[] = { 255,255,255,0 };

//UNI
static byte myip[] = { 147,52,73,203 };
static byte gwip[] = { 147,52,73,1 }; //ip route | grep default
static byte dnsip[] = { 147,52,80,1 };
static byte subnet[] = { 255,255,255,0 }; 
//------------------------------

//SERVER CONFIGURATION -------------------
char websiteIP[] = "139.91.185.16"; //"192.168.1.49"; 
#define hisPort 8087

const char websiteA[] PROGMEM = "ammar.gr";
const char websiteB[] PROGMEM = "spiti.ammar.gr";
const char * website = websiteA;
//------------------------------

//DHT11 -----------------------------------------------------
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 9
#include "SimpleDHT.h"
int pinDHT11 = 2;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;
byte dataDHT11[40] = {0};
uint32_t lastDHT11SampleTime=0;
//-----------------------------------------------------------
 


#endif
