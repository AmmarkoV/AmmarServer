
#include <UIPEthernet.h>

// **** ETHERNET SETTING ****
byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x31 };                                      
IPAddress ip(192, 168, 1, 179);                        
EthernetServer server(80);

String readString;

#define POWER_ON 0
#define POWER_OFF 1

void setup() 
{
  Serial.begin(9600);

  Serial.print("Starting Up! \n");
  // start the Ethernet connection and the server:
  Serial.print("Ethernet.begin(mac, ip);\n");
  Ethernet.begin(mac, ip);
  Serial.print("server.begin();\n");
  server.begin();

  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
  
  pinMode(2, OUTPUT);  digitalWrite(2, HIGH); 
  pinMode(3, OUTPUT);  digitalWrite(3, HIGH); 
  pinMode(4, OUTPUT);  digitalWrite(4, HIGH);   
  pinMode(5, OUTPUT);  digitalWrite(5, HIGH);   
  pinMode(6, OUTPUT);  digitalWrite(6, HIGH);   
  pinMode(7, OUTPUT);  digitalWrite(7, HIGH); 
  pinMode(8, OUTPUT);  digitalWrite(8, HIGH); 
  pinMode(9, OUTPUT);  digitalWrite(9, HIGH); 
  
  readString="";
}

void loop() 
{
  // listen for incoming clients
  EthernetClient client = server.available();

  if (client)
  {  
    Serial.println("-> New Connection");

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

    readString="";
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        
        if (readString.length() < 200) 
        {
          //store characters to string
          readString += c;
          Serial.print(c);
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          Serial.print("Got request : `"); //print to serial monitor for debuging
          Serial.print(readString); //print to serial monitor for debuging
          Serial.print("`\n"); //print to serial monitor for debuging
          
          /* Start OF HTML Section. Here Keep everything as it is unless you understands its working */
          client.println("HTTP/1.1 200 OK"); //send new page
          client.println("Content-Type: text/html");
          client.println();
          //client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
          client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
          client.println("<h1> Home Automation </h1>");
          client.println("</HEAD>");
          client.println("<body bgcolor=\"#D0D0D0\">");    
          client.println("<hr/>");
          client.println("<hr/>");
          client.println("<hr/>");
          
          // for green LED
          client.print("<h4> Green LED </h4>");
          
          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"ON "));
          client.print(F("\" onClick=\"parent.location='/?relay1on"));
          client.print(F("'\"></td>\n"));
          
          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"OFF "));
          client.print(F("\" onClick=\"parent.location='/?relay1off"));
          client.print(F("'\"></td>\n"));
          
            // for red LED
          client.print("<h4> Red LED </h4>");
          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"ON "));
          client.print(F("\" onClick=\"parent.location='/?relay2on"));
          client.print(F("'\"></td>\n"));
  
          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"OFF "));
          client.print(F("\" onClick=\"parent.location='/?relay2off"));
          client.print(F("'\"></td>\n"));

          // for yellow LED
          client.print("<h4> Yellow LED </h4>");
          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"ON "));
          client.print(F("\" onClick=\"parent.location='/?relay3on"));
          client.print(F("'\"></td>\n"));

          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"OFF "));
          client.print(F("\" onClick=\"parent.location='/?relay3off"));
          client.print(F("'\"></td>\n"));
          //client.println("<br />");
  
// for blue LED
          client.print("<h4> Blue LED </h4>");
          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"ON "));
          client.print(F("\" onClick=\"parent.location='/?relay4on"));
          client.print(F("'\"></td>\n"));

          client.print(F("<td>"));
          client.print(F("<INPUT TYPE=\"button\" VALUE=\"OFF "));
          client.print(F("\" onClick=\"parent.location='/?relay4off"));
          client.print(F("'\"></td>\n"));
          
          
          
          client.print(F("</body></html>\n"));
          break;
        }

        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
        
         // control arduino pin via ethernet Start //
          if(readString.indexOf("?relay1on") >0)//checks for on
          {
            digitalWrite(2, POWER_ON);    // set pin 4 high
            Serial.println("0 On"); 
            readString="";
          } 
          else
          {
          if(readString.indexOf("?relay1off") >0)//checks for off
           {
            digitalWrite(2, POWER_OFF);    // set pin 4 low
            Serial.println("0 Off");
            readString="";
           }
          }        
        
                 // control arduino pin via ethernet Start //
          if(readString.indexOf("?relay2on") >0)//checks for on
          {
            digitalWrite(3, POWER_ON);    // set pin 4 high
            Serial.println("1 On"); 
            readString="";
          } 
          else
          {
          if(readString.indexOf("?relay2off") >0)//checks for off
           {
            digitalWrite(3, POWER_OFF);    // set pin 4 low
            Serial.println("1 Off");
            readString="";
           }
          }        
        
          // control arduino pin via ethernet Start //
          if(readString.indexOf("?relay3on") >0)//checks for on
          {
            digitalWrite(4, POWER_ON);    // set pin 4 high
            Serial.println("2 On"); 
            readString="";
          } 
          else
          {
          if(readString.indexOf("?relay3off") >0)//checks for off
           {
            digitalWrite(4, POWER_OFF);    // set pin 4 low
            Serial.println("2 Off");
            readString="";
           }
          }
        
        
          // control arduino pin via ethernet Start //
          if(readString.indexOf("?relay4on") >0)//checks for on
          {
            digitalWrite(5, POWER_ON);    // set pin 4 high
            Serial.println("3 On"); 
            readString="";
          } 
          else
          {
          if(readString.indexOf("?relay4off") >0)//checks for off
           {
            digitalWrite(5, POWER_OFF);    // set pin 4 low
            Serial.println("3 Off");
            readString="";
           }
          }
        
       
         // control arduino pin via ethernet Start //
          if(readString.indexOf("?relay5on") >0)//checks for on
          {
            digitalWrite(6, POWER_ON);    // set pin 4 high
            Serial.println("4 On"); 
            readString="";
          } 
          else
          {
          if(readString.indexOf("?relay5off") >0)//checks for off
           {
            digitalWrite(6, POWER_OFF);    // set pin 4 low
            Serial.println("4 Off");
            readString="";
           }
          }
          
          if(readString.indexOf("?relay6on") >0)//checks for on
          {
            digitalWrite(7, POWER_ON);    // set pin 4 high
            Serial.println("5 On");
            readString="";
          }
          else{
          if(readString.indexOf("?relay6off") >0)//checks for off
          {
            digitalWrite(7, POWER_OFF);    // set pin 4 low
            Serial.println("5 Off");
            readString="";
          }
          }
          
          if(readString.indexOf("?relay7on") >0)//checks for on
          {
            digitalWrite(8, POWER_ON);    // set pin 4 high
            Serial.println("6 On");
            readString="";
          }
          else{
          if(readString.indexOf("?relay7off") >0)//checks for off
          {
            digitalWrite(8, POWER_OFF);    // set pin 4 low
            Serial.println("6 Off");
            readString="";
          }
          }
          
           if(readString.indexOf("?relay8on") >0)//checks for on
          {
            digitalWrite(9, POWER_ON);    // set pin 4 high
            Serial.println("7 On");
            readString="";
          }
          else{
          if(readString.indexOf("?relay8off") >0)//checks for off
          {
            digitalWrite(9, POWER_OFF);    // set pin 4 low
            Serial.println("7 Off");
            readString="";
          }
          }
        
         
         
      }
    }

    // give the web browser time to receive the data
    delay(10);

    // close the connection:
    client.stop();
    Serial.println("   Disconnected\n");
  }
  
}

