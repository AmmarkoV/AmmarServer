#ifndef JOYSTICK_h
#define JOYSTICK_h

#include <Arduino.h>

#define JOYSTICK_NONE 0
#define JOYSTICK_LEFT 1
#define JOYSTICK_RIGHT 2
#define JOYSTICK_UP 3
#define JOYSTICK_DOWN 4

#define byte uint8_t

const byte joystickDeadZone=100;

struct joystickState
{
  int joystickX;
  int joystickY;
  byte joystickButton;
  byte joystickDirection;
};

int joystickValveTimeHandler(
                             byte * joystickDirection,
                             byte * joystickButton,
                             int valve,
                             uint32_t timestamp,
                             byte * idleTicks,
                             byte * valvesState, 
                             byte * valvesTimes,
                             byte * valvesScheduled,
                             uint32_t * valveStartedTimestamp,
                             uint32_t * valveStoppedTimestamp 
                            );

void joystickHourTimeHandler(
                             byte * joystickDirection,
                             byte * idleTicks,
                             byte * output, 
                             byte minimum, 
                             byte maximum
                            );


void joystick24HourTimeHandler(
                               byte * joystickDirection,
                               byte * idleTicks,
                               byte * outputH, 
                               byte * outputM
                              );


int joystickMenuHandler(
                        byte * joystickDirection,
                        byte * idleTicks,
                        byte *currentMenu,
                        const byte numberOfMenus
                       );

#endif


