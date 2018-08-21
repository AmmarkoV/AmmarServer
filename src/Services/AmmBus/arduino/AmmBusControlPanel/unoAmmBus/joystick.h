#ifndef JOYSTICK_h
#define JOYSTICK_h

#include <Arduino.h>

#define JOYSTICK_NONE 0
#define JOYSTICK_LEFT 1
#define JOYSTICK_RIGHT 2
#define JOYSTICK_UP 3
#define JOYSTICK_DOWN 4

void joystickHourTimeHandler(int * joystickDirection,
                             byte * idleTicks,
                             byte * output, 
                             byte minimum, byte maximum);


void joystick24HourTimeHandler(
                               int * joystickDirection,
                               byte * idleTicks,
                               byte * outputH, 
                               byte * outputM
                              );

#endif


