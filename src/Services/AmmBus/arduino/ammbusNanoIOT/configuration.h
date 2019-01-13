#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H


#include <Arduino.h>

 
//Ethernet -----------------------------------------------------
#include <etherShield.h>
#include <ETHER_28J60.h>
// Define MAC address and IP address - both should be unique in your network
static uint8_t mac[6] = {0x54, 0x55, 0x58, 0x10, 0x00, 0x24};  
static uint8_t ip[4] = {192, 168, 1, 179};
static uint16_t port = 80; // Use port 80 - the standard for HTTP  

ETHER_28J60 e;
//-----------------------------------------------------------     



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
