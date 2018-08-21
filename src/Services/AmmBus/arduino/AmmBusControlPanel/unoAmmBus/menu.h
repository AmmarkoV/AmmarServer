#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <LiquidCrystal.h>


static const char ON_LOGO=126;
static const char OFF_LOGO='x';
 
void printAllValveState(
                        LiquidCrystal * lcd , 
                        byte * valvesState, 
                        byte * valvesScheduled 
                       );

#endif


