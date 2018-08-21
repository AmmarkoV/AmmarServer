
#ifndef MENU_H
#define MENU_H


#define byte uint8_t
#include <Arduino.h>
#include <LiquidCrystal.h>

#define JOYSTICK_NONE 0
#define JOYSTICK_LEFT 1
#define JOYSTICK_RIGHT 2
#define JOYSTICK_UP 3
#define JOYSTICK_DOWN 4

static const char ON_LOGO=126;
static const char OFF_LOGO='x';

void joystickHourTimeHandler(int * joystickDirection,
                             byte * idleTicks,
                             byte * output, 
                             byte minimum, byte maximum);



void printAllValveState(
                        LiquidCrystal * lcd , 
                        byte * valvesState, 
                        byte * valvesScheduled 
                       );

#endif


