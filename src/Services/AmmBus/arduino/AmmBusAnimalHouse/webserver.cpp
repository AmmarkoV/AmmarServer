#include "webserver.h"
#include "ammBus.h"
 
char onStr[7]={"?X=on"};
char offStr[7]={"?X=off"}; 

int ServeHTTPHeader(
                    EthernetClient * client,
                    unsigned int refresh
                   )
{
  client->println("HTTP/1.1 200 OK");
  client->println("Content-Type: text/html");
  client->println("Connection: close");  // the connection will be closed after completion of the response
  if (refresh) 
                 { 
                   client->print("Refresh: "); 
                   client->println(int(refresh)); 
                 } // refresh the page automatically every 5 sec
  client->println();
}


int ServeWebServerClient(EthernetClient * client , int temperature , int humidity , int tooHotCounter , int tooColdCounter )
{
   #define MAX_GET_STRING 64
   char readString[MAX_GET_STRING+1]; //string for fetching data from address
   char * ptr=readString;
   char * ptrLimit=readString+MAX_GET_STRING;
  
  //listen for incoming clients
  if (client) 
  {
    Serial.println("new client");
 
    char c = 1;
    while (c!=0 && c!=10 && c!=13 && ptr<ptrLimit)
     {
        c = client->read();
        *ptr=c;
        ++ptr;
        *ptr=0;
     } 
    Serial.println("Requested:");
    Serial.println(readString);
    
    
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

          client->println("<!DOCTYPE HTML>");
          client->println("<html>"); 
          
          client->print("<h4>");   
          client->print(systemName);   
          client->print(" ");   
          client->print(systemVersion);   
          client->print("</h4>");
   
            //client->println("UnixTime:"); 
            //client->println(dt.unixtime); 
            //client->println("<br />"); 
            //------------------------
            client->print("DHT11: ");
            client->print((int)temperature);
            client->print(" *C ");
            client->print((int)humidity);
            client->println("%<br />"); 

            if ( tooHotCounter > 0 )
              {
                client->print("Hot Alarms : ");
                client->print((int)tooHotCounter); 
                client->println("<br />"); 
              }

            if ( tooColdCounter > 0 )
              {
                client->print("Cold Alarms : ");
                client->print((int)tooColdCounter); 
                client->println("<br />"); 
              }
      
      client->println("<br />"); 
      int i=0;
      for (i=0; i<8; i++)
      { 
       onStr[1]  = 'a'+i; 
       offStr[1]  = 'a'+i; 
       client->print("<a href='");
       client->print(offStr);
       client->print("'>Off</a>|");
       client->print("<a href='");
       client->print(onStr);
       client->print("'>On</a><br>"); 
      }
    client->print("<a href='?all=off'>All Off</a></body></html>");
          
          client->println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client->stop();
    Serial.println("client disconnected");
  }
  
}
