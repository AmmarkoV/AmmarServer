#include "temperature.h"
#include "timeCalculations.h"

int logTemperature(
                    struct ammBusState * ambs,
                    uint32_t  timestamp,
                    byte temperature
                   )
{
  byte week,day,hour,minute,second;
                    
  unixtimeToWDHMS(
                  timestamp,
                  &week,
                  &day,
                  &hour, 
                  &minute,
                  &second
                 );

   if (hour<24)
   {
    ambs->lastWeek.day[0].hour[hour]=temperature;         
    return 1;          
   }              
 return 0;  
}
