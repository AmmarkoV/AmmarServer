#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H


#include <Arduino.h>

  


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
