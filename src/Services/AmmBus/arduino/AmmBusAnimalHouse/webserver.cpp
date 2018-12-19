#include "webserver.h"
 
char onStr[7]={"?X=on"};
char offStr[7]={"?X=off"}; 


int ServeErrorHTTPHeader(
                          EthernetClient * client 
                        )
{
  client->println(F("HTTP/1.1 500 ERROR"));
  client->println(F("Content-Type: text/html"));
  client->println(F("Connection: close"));  // the connection will be closed after completion of the response 
  client->println();
  client->println(F("Bad request"));  //A brief message
}


int ServeHTTPHeader(
                    EthernetClient * client,
                    unsigned int refresh
                   )
{
  client->println(F("HTTP/1.1 200 OK"));
  client->println(F("Content-Type: text/html"));
  client->println(F("Connection: close"));  // the connection will be closed after completion of the response
  if (refresh) 
                 { 
                   client->print(F("Refresh: ")); 
                   client->println(int(refresh)); 
                 } // refresh the page automatically every 5 sec
  client->println();
}

int terminateStringAtFirstSpace(char * request)
{
   char * ptrRequest=request;
   while (ptrRequest!=0)
    {
      if (*ptrRequest==' ') { *ptrRequest=0; return 1; }
      ++ptrRequest; 
    }
  return 0; 
}


int  resolveGETRequest(const char * request)
{/*
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
  }*/
  return 0;
}




int ServeWebServerClient(
                         struct ammBusState * ambs,
                         EthernetClient * client,
                         int temperature,
                         int humidity,
                         int tooHotCounter,
                         int tooColdCounter
                        )
{
   #define MAX_GET_STRING 64
   char readString[MAX_GET_STRING+1]; //string for fetching data from address
   char * ptr=readString;
   char * ptrLimit=readString+MAX_GET_STRING;
  
  //listen for incoming clients
  if (client) 
  {
    Serial.println(F("Client"));
 
    char c = 1;
    while (c!=0 && c!=10 && c!=13 && ptr<ptrLimit)
     {
        c = client->read();
        *ptr=c;
        ++ptr;
        *ptr=0;
     } 
    //Serial.println(F("Requested:"));
    //Serial.println(readString);

    char * GETRequest = readString;
    if (
        ( readString[0]=='G' ) && 
        ( readString[1]=='E' ) && 
        ( readString[2]=='T' ) && 
        ( readString[3]==' ' )
       )
       {
          terminateStringAtFirstSpace(readString+4);
          GETRequest+=4;
          Serial.println(F("Resolved:"));
          Serial.println(GETRequest); 
       } else
       {
        ServeErrorHTTPHeader(client);
       }
       
    
    
    
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (client->connected()) 
     {
      if (client->available()) 
      {
        char c = client->read();
        //Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) 
        {
          ServeHTTPHeader(client,5);
          // send a standard http response header

          client->println(F("<!DOCTYPE HTML>"));
          client->println(F("<html><h4>"));
          client->print(systemName);   
          client->print(F(" "));   
          client->print(systemVersion);   
          client->print(F("</h4>"));
   
            //client->println("UnixTime:"); 
            //client->println(dt.unixtime); 
            //client->println("<br />"); 
            //------------------------
            client->print(F("DHT11: "));
            client->print((int)temperature);
            client->print(F("*C "));
            client->print((int)humidity);
            client->println(F("%<br />")); 

            if ( tooHotCounter > 0 )
              {
                client->print(F("Hot"));
                client->print(F(" Alarms : "));
                client->print((int)tooHotCounter); 
                client->println("<br />"); 
              }

            if ( tooColdCounter > 0 )
              {
                client->print(F("Cold"));
                client->print(F(" Alarms : "));
                client->print((int)tooColdCounter); 
                client->println(F("<br />")); 
              }
      
      client->println(F("<br />")); 
      int i=0;
      for (i=0; i<8; i++)
      { 
       onStr[1]  = 'a'+i; 
       offStr[1]  = 'a'+i; 
       client->print(F("<a href='"));
       client->print(offStr);
       client->print(F("'>Off</a>|"));
       client->print(F("<a href='"));
       client->print(onStr);
       client->print(F("'>On</a><br>")); 
      }
     client->print(F("<a href='?all=off'>All Off</a></body></html>")); 
          break;
        }
        if (c == '\n') 
          {
           // you're starting a new line
           currentLineIsBlank = true;
          } else 
        if (c != '\r') 
          {
           // you've gotten a character on the current line
           currentLineIsBlank = false;
          }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client->stop();
    //Serial.println("client disconnected");
  }
  
}
