#ifndef __TIMECALCULATIONS_H
#define __TIMECALCULATIONS_H

#define byte uint8_t


  
 /**
  * @brief Convert Unix timestamp to date
  * @param[in] t Unix timestamp
  * @param[out] date Pointer to a structure representing the date and time
  **/
  
 static void unixtimeToDate(
                     uint32_t t,
                     byte * seconds,
                     byte * minutes,
                     byte * hours,
                     byte * day,
                     byte * month,
                     unsigned short * year
                    )
 {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint32_t e;
    uint32_t f;
  
    //Negative Unix time values are not supported
    if(t < 1)
       t = 0;
   
  
    //Retrieve hours, minutes and seconds
    *seconds = t % 60;
    t /= 60;
    *minutes = t % 60;
    t /= 60;
    *hours = t % 24;
    t /= 24;
  
    //Convert Unix time to date
    a = (uint32_t) ((4 * t + 102032) / 146097 + 15);
    b = (uint32_t) (t + 2442113 + a - (a / 4));
    c = (20 * b - 2442) / 7305;
    d = b - 365 * c - (c / 4);
    e = d * 1000 / 30601;
    f = d - e * 30 - e * 601 / 1000;
  
    //January and February are counted as months 13 and 14 of the previous year
    if(e <= 13)
    {
       c -= 4716;
       e -= 1;
    }
    else
    {
       c -= 4715;
       e -= 13;
    }
  
    //Retrieve year, month and day
    *year = c;
    *month = e;
    *day = f;

 }
  


static void unixtimeToWDHMS(
                     uint32_t unixtimestamp,
                     byte * week,
                     byte * day,
                     byte * hour, 
                     byte * minute,
                     byte * second
                    )
{
  unixtimestamp -= 946681200;
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
  unsigned int runningTime = getValveRunningTimeMinutes(vStartTime,vState,currentTime);
  if (runningTime>=vDuration) { return 0; }
  
  unsigned int remainingTime = vDuration-runningTime;

  return remainingTime;
}


static unsigned int getValveRemainingTimeSeconds(uint32_t vStartTime,byte vDuration , byte vState,uint32_t currentTime)
{ 
  return getValveRemainingTimeMinutes(vStartTime,vDuration,vState,currentTime)*60;
}








#endif
