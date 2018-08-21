#include "joystick.h"










void joystickHourTimeHandler(
                             int * joystickDirection,
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
                               int * joystickDirection,
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

