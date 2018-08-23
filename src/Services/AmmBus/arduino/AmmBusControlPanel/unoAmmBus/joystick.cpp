#include "joystick.h"


int getJoystickState(
                      int joystickAnalogueXPin,
                      int joystickAnalogueYPin,
                      int joystickDigitalButtonPin,
                      struct joystickState * js
                    )
{
  //Joystick Reading
 js->jX=analogRead(joystickAnalogueXPin);
 js->jY=analogRead(joystickAnalogueYPin);
 js->jButton = digitalRead(joystickDigitalButtonPin);
 if (js->jButton) { js->jButton=0; } else
                  { js->jButton=1; }
  
 if (js->jX<512-joystickDeadZone)  {  js->jDirection=JOYSTICK_LEFT;  } else
 if (js->jX>512+joystickDeadZone)  {  js->jDirection=JOYSTICK_RIGHT; } else
 if (js->jY<512-joystickDeadZone)  {  js->jDirection=JOYSTICK_UP;    } else
 if (js->jY>512+joystickDeadZone)  {  js->jDirection=JOYSTICK_DOWN;  } else
                                   {  js->jDirection=JOYSTICK_NONE;  } 
 
}



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
                            )
{ 
  switch (*joystickDirection)
  {
    case JOYSTICK_NONE : break;  
    case JOYSTICK_UP : 
     valvesTimes[valve]+=5;
     *idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_DOWN : 
     valvesTimes[valve]-=5; 
     *idleTicks=0;
    break;
    //-------------------------
  }
   
  if (*joystickButton) 
  {
   if ( valvesState[valve] ) { 
                               valvesState[valve]=0; 
                               valvesScheduled[valve]=0; 
                               valveStoppedTimestamp[valve]=timestamp;
                             } else
                             { 
                               valvesState[valve]=1;   
                               valvesScheduled[valve]=1;
                               valveStartedTimestamp[valve]=timestamp;
                             } 
   return 1;
  } 
 return 0; 
}





void joystickHourTimeHandler(
                             byte * joystickDirection,
                             byte * idleTicks,
                             byte * output, 
                             byte minimum, 
                             byte maximum
                            )
{ 
  switch (*joystickDirection)
  {
    case JOYSTICK_NONE : break;  
    case JOYSTICK_UP : 
     if (*output<maximum-1)
                   { *output+=1; }
     *idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_DOWN : 
     if (*output>minimum+1)
                   { *output-=1; } 
     *idleTicks=0;
    break;
    //-------------------------
  } 
}


void joystick24HourTimeHandler(
                               byte * joystickDirection,
                               byte * idleTicks,
                               byte * outputH, 
                               byte * outputM
                              )
{ 
  unsigned int minutes=(unsigned int) ((*outputH) * 60)  + (*outputM);
  
  const byte clockSpeed=15;//mins
  
  switch (*joystickDirection)
  {
    case JOYSTICK_NONE : 
     //return; 
    break;  
    case JOYSTICK_UP : 
     if (minutes+clockSpeed<1440)
                   { minutes+=clockSpeed; }else
                   { minutes=0; }
     *idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_DOWN : 
     if (minutes>clockSpeed)
                   { minutes-=clockSpeed; } else
                   { minutes=1440-clockSpeed; }
     *idleTicks=0;
    break;
    //-------------------------
  } 
  
  unsigned int tmp = (unsigned int) minutes/60; 
  *outputH = (byte) tmp;
   tmp = (unsigned int) minutes%60; 
  *outputM = (byte) tmp;
}






int joystickMenuHandler(
                        byte * joystickDirection,
                        byte * idleTicks,
                        byte *currentMenu,
                        const byte numberOfMenus
                       )
{
  switch (*joystickDirection)
  {
    case JOYSTICK_NONE : break;
    //-------------------------
    case JOYSTICK_RIGHT : 
     if (*currentMenu==numberOfMenus-1) { *currentMenu=0; } else
                                       { ++*currentMenu; }     
    *idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_LEFT : 
     if (*currentMenu==0) { *currentMenu=numberOfMenus-1; } else
                         { --*currentMenu; }
    *idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_UP : 
    
    *idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_DOWN : 
    
    *idleTicks=0;
    break;
    //-------------------------
  }
   
   
 if ( (*idleTicks==0) && (*joystickDirection!=JOYSTICK_NONE))
  {
   //turnLCDOn();
   return 1;
  } 
 return 0; 
}


