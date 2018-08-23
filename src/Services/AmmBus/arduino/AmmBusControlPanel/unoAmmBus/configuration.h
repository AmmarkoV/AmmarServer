#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H


#include <Arduino.h>

//LCD -----------------------------------------------------
/* 
  The circuit:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 8
 * LCD D4 pin to digital pin 9
 * LCD D5 pin to digital pin 10
 * LCD D6 pin to digital pin 11
 * LCD D7 pin to digital pin 12
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */
#include <LiquidCrystal.h>
#include <Wire.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int lcdPowerPin=6;
//-----------------------------------------------------------

//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------


//Joystick -----------------------------------------------------
const int SW_pin = 2; // digital pin connected to switch output
const int X_pin = 0; // analog pin connected to X output
const int Y_pin = 1; // analog pin connected to Y output 
//-----------------------------------------------------------

//74HC595 -----------------------------------------------------
int tDelay = 1000;
int latchPin = 4;      // (4) ST_CP [RCK] on 74HC595
int clockPin = 3;      // (3) SH_CP [SCK] on 74HC595
int dataPin = 5;     // (5) DS [S1] on 74HC595
byte leds = 0;
//-----------------------------------------------------------

//DHT11 -----------------------------------------------------
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 13
#include "SimpleDHT.h"
int pinDHT11 = 13;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;
byte dataDHT11[40] = {0};
uint32_t lastDHT11SampleTime=0;
//-----------------------------------------------------------


#endif
