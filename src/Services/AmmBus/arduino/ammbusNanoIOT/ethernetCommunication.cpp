#include "ethernetCommunication.h"

#include "timeCalculations.h" 

unsigned int clientsServiced=0;
char onStr[7]={"?X=on"};
char offStr[7]={"?X=off"}; 
  
void AmmBusEthernetProtocol::sendPage(ETHER_28J60 * e,byte temperature , byte humidity)
{      
  ++clientsServiced;
   byte i=0; 
   e->print("<html><head><title>Sensor</title><meta http-equiv=\"refresh\" content=\"5\"></head><body><h1>Ammbus</h1>");
   e->print("<h4>Temperature: ");
   e->print(temperature);
   e->print("<br>");
   e->print("Humidity: ");
   e->print(humidity);
   e->print("<br>");
   e->print("</h4>");

   e->print("<br>#");
   e->print(clientsServiced);
   e->print("<br>");
   
   e->print("</body></html>");
   e->respond();
}
  


int  AmmBusEthernetProtocol::receiveEthernetRequests(ETHER_28J60 * e , byte temperature , byte humidity)
{
  char* params;
  if (params = e->serviceRequest())
  { 
    Serial.print("!");   
    sendPage(e,temperature,humidity);  
    
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
