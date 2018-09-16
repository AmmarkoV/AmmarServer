#include "ethernetCommunication.h"

#include "timeCalculations.h" 

unsigned int clientsServiced=0;
char onStr[7]={"?X=on"};
char offStr[7]={"?X=off"}; 


void AmmBusEthernetProtocol::sendPage(ETHER_28J60 * e)
{      
  ++clientsServiced;
   byte i=0; 
   e->print("<html><body><h1>Ammbus</h1>");
   e->print("<h4>Rqst ");  
   e->print(clientsServiced);
   e->print("</h4>");
      for (i=0; i<8; i++)
      { 
       onStr[1]  = 'a'+i; 
       offStr[1]  = 'a'+i; 
       e->print("<a href='");
       e->print(offStr);
       e->print("'>Off</a>|");
       e->print("<a href='");
       e->print(onStr);
       e->print("'>On</a><br>"); 
      }
    e->print("<a href='?all=off'>All Off</a></body></html>");
    e->respond();
}


void sendState(ETHER_28J60 * e, 
                                       RTCDateTime * dt)
{      
  ++clientsServiced;
   byte i=0; 
   e->print("<html><body><h1>Ammbus State</h1>");
   e->print("<h4>Rqst ");  
   e->print(clientsServiced);
   e->print("</h4>");
       
   byte week, day, hour, minute, second;
                    
       unixtimeToWDHMS(
                        dt->unixtime,
                        &week,
                        &day,
                        &hour, 
                        &minute,
                        &second
                       );
       
    e->print(" Week ");
    e->print(week);
    e->print(" Day ");
    e->print(day);
    e->print(" ");
    e->print(hour);
    e->print(":");
    e->print(minute);
    e->print(":");
    e->print(second);
    e->respond();
}



int  AmmBusEthernetProtocol::receiveEthernetRequests(ETHER_28J60 * e,
                                                     DS3231 *clock,
                                                     RTCDateTime * dt)
{
  char* params;
  if (params = e->serviceRequest())
  { 
    //Serial.print("!");  
    if (strcmp(params,"state.html") == 0)
       { 
         sendState(e,dt); 
       } else
       {
         sendPage(e); 
       }
    
    byte port;
    byte i;
    
    if (strcmp(params,"?all=off") == 0)
       { 
         for (i=0; i<8; i++)
         { 
          port=2+i;
          digitalWrite(port, HIGH);  
          return 1;
         }
       } else
    if (strcmp(params,"?all=on") == 0)
       { 
         for (i=0; i<8; i++)
         { 
          port=2+i;
          digitalWrite(port, LOW);  
          return 1;
         }
       } else
    {  
    for (i=0; i<8; i++)
      { 
       onStr[1]  = 'a'+i; 
       offStr[1]  = 'a'+i; 
       port=2+i;
       
       if (strcmp(params, offStr) == 0)
       { 
        digitalWrite(port, HIGH); 
        return 1; 
       } else 
       if (strcmp(params, onStr) == 0) 
       { 
        digitalWrite(port, LOW);  
        return 1;
       }
      }
    } 
  }
  return 0;
}

