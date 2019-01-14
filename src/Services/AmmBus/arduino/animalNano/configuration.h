#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H


#include <Arduino.h>

#define REQUEST_RATE_NORMAL 5*60*1000 // minutes to milliseconds
#define REQUEST_RATE_CRITICAL 10000 // milliseconds
#define USE_DHCP 0
#define USE_DNS 0

//ENCRYPTION -------------------
const char serialNumber[] PROGMEM = "000001";
//This is the default private key currently not used and has to be manually updated for each device .. 
static byte privateKey[16] = {0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D,0x30,0x33,0x74,0x69,0x69,0x2D}; 
//------------------------------


//ETHERNET CONFIGURATION -------------------
#define ETHERNET_BUFFER 320 //bytes
// ethernet interface mac address
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x33 };
static byte myip[] = { 192,168,1,179 };
static byte gwip[] = { 192,168,1,20 }; //ip route | grep default
static byte dnsip[] = { 192,168,1,20 };
static byte subnet[] = { 255,255,255,0 };
//------------------------------


//SERVER CONFIGURATION -------------------
char websiteIP[] = "139.91.185.16"; //"192.168.1.49";
static byte hisip[] =  {139,91,185,16}; // {192,168,1,49};//
#define hisPort 8087

const char website[] PROGMEM = "ammar.gr";
//------------------------------



   
//DHT11 -----------------------------------------------------
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 13
#include "SimpleDHT.h"
int pinDHT11 = 9;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;
byte dataDHT11[40] = {0};
uint32_t lastDHT11SampleTime=0;
//-----------------------------------------------------------




//RGB LED -----------------------------------------------------
int pinR_LED = 8;
int pinG_LED = 7;
int pinB_LED = 6;
//-----------------------------------------------------------



//BUTTON -----------------------------------------------------
int buttonPin = 5;
//-----------------------------------------------------------





#endif
