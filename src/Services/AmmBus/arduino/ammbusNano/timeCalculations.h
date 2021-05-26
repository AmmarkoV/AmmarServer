#ifndef __TIMECALCULATIONS_H
#define __TIMECALCULATIONS_H

 

static void unixtimeToWDHMS(
                     uint32_t unixtimestamp,
                     byte * week,
                     byte * day,
                     byte * hour, 
                     byte * minute,
                     byte * second
                    )
{
  *week   = (unixtimestamp / 86400) / 7;
  *day    = (unixtimestamp / 86400) % 7;
  *hour   = (unixtimestamp / 3600) % 24;
  *minute = (unixtimestamp / 60) % 60; 
  *second = unixtimestamp % 60; 
} 

static byte currentTimeIsCloseEnoughToHourMinute(
                                           uint32_t unixtimestamp, 
                                           byte hourToCheck, 
                                           byte minuteToCheck
                                         )
{
  byte week,day,hour,minute,second;  
  unixtimeToWDHMS(
                   unixtimestamp,
                   &week,
                   &day,
                   &hour, 
                   &minute,
                   &second
                 );
                 
  if ( (hour==hourToCheck) && (minute==minuteToCheck) )
   {
     return 1;
   }
 return 0;  
}
 
static unsigned int getTimeAfterWhichWeNeedToReactivateValveInSeconds(byte jobRunEveryXHours)
{
 unsigned int timeThatNeedsToPass = jobRunEveryXHours * 60 * 60; 
 return timeThatNeedsToPass; 
}

static unsigned int getValveOfflineTimeSeconds(uint32_t vStopTime, byte vState,uint32_t currentTime)
{
  if (vState) { return 0; }
  return (unsigned int) currentTime - vStopTime; 
}


static unsigned int getValveRunningTimeSeconds(uint32_t vStartTime, byte vState,uint32_t currentTime)
{
  if (!vState) { return 0; }
  return (unsigned int) currentTime - vStartTime; 
}

static unsigned int getValveRunningTimeMinutes(uint32_t vStartTime, byte vState,uint32_t currentTime)
{
  return getValveRunningTimeSeconds(vStartTime,vState,currentTime)/60;
}


static unsigned int getValveRemainingTimeMinutes(uint32_t vStartTime,byte vDuration , byte vState,uint32_t currentTime)
{
  unsigned int runningTime = getValveRunningTimeMinutes(vStartTifile:///home/ammar/Documents/Programming/AmmarServer/src/Services/AmmBus/arduino/ammbusNano/serialCommunication.cppme,vState,currentTime);
  if (runningTime>=vDuration) { return 0; }
  
  unsigned int remainingTime = vDuration-runningTime;

  return remainingTime;
}


static unsigned int getValveRemainingTimeSeconds(uint32_t vStartTime,byte vDuration , byte vState,uint32_t currentTime)
{ 
  return getValveRemainingTimeMinutes(vStartTime,vDuration,vState,currentTime)*60;
}








#endif
