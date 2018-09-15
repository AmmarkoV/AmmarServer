
#include <UIPEthernet.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x78, 0xE0 }; // <------- PUT YOUR MAC Address Here
byte ip[] = { 192, 168, 1, 179 }; //                    <------- PUT YOUR IP Address Here
byte gateway[] = { 192, 168, 1, 1 }; //               <------- PUT YOUR ROUTERS IP Address to which your shield is connected Here
byte subnet[] = { 255, 255, 255, 0 }; //                <------- It will be as it is in most of the cases
EthernetServer server(80); //                           <------- It's Defaulf Server Port for Ethernet Shield

String readString;

//////////////////////

void setup() 
{  
  Serial.begin(9600);
  Serial.println("Server test 1.0"); // so that we can know what is getting loaded
  pinMode(6, OUTPUT); // Pin Assignment through which relay/LEDS will be controlled
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  //start Ethernet 
  Serial.println("Ethernet.begin(mac, ip, gateway, subnet);");
  //Ethernet.begin(mac, ip, gateway, subnet);
  Ethernet.begin(mac, ip);
   Serial.println("server.begin();");
  server.begin();
  //enable serial data print
}

void loop() 
{   
  // Create a client connection
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
        }
        //if HTTP request has ended
        if (c == '\n') {
          ///////////////
          Serial.print("Got request : "); //print to serial monitor for debuging
          Serial.println(readString); //print to serial monitor for debuging
          /* Start OF HTML Section. Here Keep everything as it is unless you understands its working */
          client.println("HTTP/1.1 200 OK"); //send new page
          client.println("Content-Type: text/html");
          client.println();
          //client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
          client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
          // http://www.himix.lt/arduino/autohome.css
          client.println("<link rel=\"stylesheet\" type=\"text/css\" href=\"http://www.himix.lt/arduino/autohome.css\" />");   
          client.println("<h1> Home Automation </h1>");
          client.println("<h2> Visit http://www.himix.lt for more tutorials!</h2>");  
          client.println("</HEAD>");
          client.println("<body bgcolor=\"#D0D0D0\">");    
          client.println("<hr/>");
          client.println("<hr/>");
          //http://www.himix.lt/arduino/autohome.jpg
          client.println("<h4><center><img border=\"0\" src=\"http://www.himix.lt/arduino/autohome.gif\" /></center></h4>");
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
          //client.println("<br />");

         // control arduino pin via ethernet Start //
          if(readString.indexOf("?relay1on") >0)//checks for on
          {
            digitalWrite(6, HIGH);    // set pin 4 high
            Serial.println("Green Led On");
            
        }
          else{
          if(readString.indexOf("?relay1off") >0)//checks for off
          {
            digitalWrite(6, LOW);    // set pin 4 low
            Serial.println("Green Led Off");
        }
          }
          
          if(readString.indexOf("?relay2on") >0)//checks for on
          {
            digitalWrite(7, HIGH);    // set pin 4 high
            Serial.println("Led On");
          }
          else{
          if(readString.indexOf("?relay2off") >0)//checks for off
          {
            digitalWrite(7, LOW);    // set pin 4 low
            Serial.println("Led Off");
          }
          }
          
           if(readString.indexOf("?relay3on") >0)//checks for on
          {
            digitalWrite(8, HIGH);    // set pin 4 high
            Serial.println("Led On");
          }
          else{
          if(readString.indexOf("?relay3off") >0)//checks for off
          {
            digitalWrite(8, LOW);    // set pin 4 low
            Serial.println("Led Off");
          }
          }
           if(readString.indexOf("?relay4on") >0)//checks for on
          {
            digitalWrite(9, HIGH);    // set pin 4 high
            Serial.println("Led On");
          }
          else{
          if(readString.indexOf("?relay4off") >0)//checks for off
          {
            digitalWrite(9, LOW);    // set pin 4 low
            Serial.println("Led Off");
          }
          }
          // control arduino pin via ethernet End //
          // Relay Status Display
          client.println("<center>");
          client.println("<table border=\"5\">");
          client.println("<tr>");
          
          if (digitalRead(6)) { 
           client.print("<td>Green LED is ON</td>");
           
          }
          else
          {
            client.print("<td>Green LED is OFF</td>");
          }
          
          
          client.println("<br />");
          
          if (digitalRead(7)) { 
           client.print("<td>Red LED is ON</td>");
          }
          else
          {
            client.print("<td>Red LED is OFF</td>");
          }
          
          client.println("</tr>");
          client.println("<tr>");
          if (digitalRead(8)) { 
           client.print("<td>Yellow LED is ON</td>");
          }
          else
          {
            client.print("<td>Yellow LED is OFF</td>");
          }
          
          if (digitalRead(9)) { 
           client.print("<td>Blue LED is ON</td>");
          }
          else
          {
            client.print("<td>Blue LED is OFF</td>");
          }
          client.println("</tr>");
          client.println("</table>");
          client.println("</center>");
          //clearing string for next read
          readString="";
          client.println("</body>");
          client.println("</HTML>");
          delay(1);
          //stopping client
          client.stop();
        }
      }
    }
  }
}
