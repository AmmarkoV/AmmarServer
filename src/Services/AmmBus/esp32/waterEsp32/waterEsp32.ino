
// Load Wi-Fi library
#include <WiFi.h>
#include "timeCalculations.h" 
#include "configuration.h"

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#include "serialCommunication.h"
AmmBusUSBProtocol ammBusUSB;

//#include "wirelessCommunication.h"
//AmmBusWirelessProtocol ammBusEthernet;


#include "ammBus.h"
struct ammBusState ambs={0};

// Set web server port number to 80
WiFiServer server(serverPort);

// Variable to store the HTTP request
String header;
  

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void setup() 
{
  pinMode (ledPin, OUTPUT);
  // Initialize the output variables as outputs

  //Make sure on boot all valves are off..
  for (int i=0; i<NUMBER_OF_RELAYS; i++)
  {
    pinMode(RELAY_ADDRESS[i], OUTPUT);
    digitalWrite(RELAY_ADDRESS[i], LOW);
  }
   

  Serial.begin(115200);
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  //-----------------------------------------------
  while (WiFi.status() != WL_CONNECTED) 
  {
    digitalWrite (ledPin, LOW);  // turn off the LED
    delay(250);
    Serial.print(".");
    digitalWrite (ledPin, HIGH);  // turn on the LED
    delay(250);
  }
  digitalWrite (ledPin, LOW);  // turn off the LED
  //-----------------------------------------------
  
   setRelayState(ambs.valvesState);
  
   Serial.println("\nStarting Up Clock! \n");
   realtime_clock.begin();
   if (RESET_CLOCK_ON_NEXT_COMPILATION)
       { realtime_clock.setDateTime(__DATE__, __TIME__); } 
   dt = realtime_clock.getDateTime();   
   
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Website: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8080");


  initializeAmmBusState(&ambs);

  ambs.ACRelayExists=AC_RELAY;
  ambs.ACRelayPort=AC_RELAY_PORT;
  
  //2 dead relays
  //---------------------------
  ambs.valvesTimesNormal[0]=1;
  ambs.valvesTimesHigh[0]=1;
  ambs.valvesTimesLow[0]=1;
  //---------------------------
  ambs.valvesTimesNormal[1]=1;
  ambs.valvesTimesHigh[1]=1;
  ambs.valvesTimesLow[1]=1;

  //4 active relays
  //---------------------------
  ambs.valvesTimesNormal[2]=15;
  ambs.valvesTimesHigh[2]=20;
  ambs.valvesTimesLow[2]=1;
  //---------------------------
  ambs.valvesTimesNormal[3]=10;
  ambs.valvesTimesHigh[3]=15;
  ambs.valvesTimesLow[3]=1;
  //---------------------------
  ambs.valvesTimesNormal[4]=10;
  ambs.valvesTimesHigh[4]=15;
  ambs.valvesTimesLow[4]=1;
  //---------------------------
  ambs.valvesTimesNormal[4]=5;
  ambs.valvesTimesHigh[4]=7;
  ambs.valvesTimesLow[4]=1;

  //2 dead relays
  //---------------------------
  ambs.valvesTimesNormal[5]=1;
  ambs.valvesTimesHigh[5]=1;
  ambs.valvesTimesLow[5]=1;
  //---------------------------
  ambs.valvesTimesNormal[6]=1;
  ambs.valvesTimesHigh[6]=1;
  ambs.valvesTimesLow[6]=1;
  //---------------------------
  ambs.valvesTimesNormal[7]=1;
  ambs.valvesTimesHigh[7]=1;
  ambs.valvesTimesLow[7]=1;
    
  server.begin();
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


void setRelayState( byte * valves )
{ 
  int i=0;
  byte port ;
  for (i=0; i<NUMBER_OF_RELAYS; i++) 
  {    
     port=RELAY_ADDRESS[i];
     if ( (AC_RELAY) && (port==AC_RELAY_PORT) )
     {
      //Skip setting auto relay port..
     } else
     { 
      if (!valves[i] )  {  digitalWrite(port, HIGH);   } else
                        {  digitalWrite(port, LOW);    }
     }
  }  


  //Have a dedicated AC relay force it on or off
  //--------------------------------------------
  if (AC_RELAY)
    {
     unsigned int activeRelays = 0;
     for (i=0; i<NUMBER_OF_RELAYS; i++) 
      {  
        if (valves[i] )  { activeRelays=activeRelays+1; }
      }

      Serial.print(activeRelays);
      Serial.print(" active relays, setting Relay address ");
      Serial.println(RELAY_ADDRESS[AC_RELAY_PORT]);
      if (activeRelays>0) 
                          { digitalWrite(RELAY_ADDRESS[AC_RELAY_PORT], LOW);  } else
                          { digitalWrite(RELAY_ADDRESS[AC_RELAY_PORT], HIGH); } 
    }


}



void valveAutopilot()
{
  unsigned int changes=0;
  unsigned int i=0;
  byte valvesRunning=ammBus_getRunningValves(&ambs);
  byte valvesRemaining=ammBus_getScheduledValves(&ambs);
  
   
  if (valvesRunning>0)
  {
   //Check if a valve needs to be closed.. 
   for (i=0; i<NUMBER_OF_SWITCHES; i++)
   {
    if (ambs.valvesState[i])
    {
      //This valve is running, should it stop?  
      unsigned int runningTime =  getValveRunningTimeMinutes(
                                                             ambs.valveStartedTimestamp[i],
                                                             ambs.valvesState[i],
                                                             dt.unixtime  
                                                            );
      if ( runningTime > ambs.valvesTimes[i] ) 
         {
          
           //This valve has run its course so we stop it
           ammBus_stopValve(&ambs,i);
           ++changes;
         }
    }
   }
  }

  //If autopilotCreateNewJobs is set
  if (ammBus_getAutopilotState(&ambs))
  {
   //Check which valves should be scheduled for start
   for (i=0; i<NUMBER_OF_SWITCHES; i++)
    {
        if (
             getValveOfflineTimeSeconds(
                                        ambs.valveStoppedTimestamp[i],
                                        ambs.valvesState[i],
                                        dt.unixtime
                                       ) 
                                        >  
              getTimeAfterWhichWeNeedToReactivateValveInSeconds(
                                                                ambs.jobRunEveryXHours
                                                               ) 
            )
           {  
              if (
                   currentTimeIsCloseEnoughToHourMinute(
                                                        dt.unixtime,
                                                        ambs.jobRunAtXHour,
                                                        ambs.jobRunAtXMinute
                                                       )
                 )
              {
                ambs.valvesScheduled[i]=1;
              }
           }
    }

  //Recount valve status  
  valvesRunning=ammBus_getRunningValves(&ambs);
  valvesRemaining=ammBus_getScheduledValves(&ambs);

  //We can accomodate more jobs..
  if ( (valvesRunning<ambs.jobConcurrency) && (valvesRemaining>0)  )
  {
    for (i=0; i<NUMBER_OF_SWITCHES; i++)
    {
      byte valveIsNotUsedInThisSetup = (valveLabels[i][0]=='-');
      //Open first possible scheduled valve 
      if ( (!valveIsNotUsedInThisSetup) && (ambs.valvesScheduled[i]) && (!ambs.valvesState[i]) && (valvesRunning<ambs.jobConcurrency) )
      {    
            ++changes;
            ++valvesRunning;
            ambs.valvesState[i]=1;
            ambs.valveStartedTimestamp[i]=dt.unixtime; 
      }
    }
  }
  }

  //Changes made/update relays  
  if (changes) 
    { setRelayState(ambs.valvesState); }
}


void checkForSerialInput()
{
  
  if ( 
       ammBusUSB.newUSBCommands(
                                ambs.valvesState,
                                ambs.valvesScheduled,
                                &realtime_clock,
                                &dt,
                                &ambs.idleTicks
                               ) 
     )
     {  
      setRelayState(ambs.valvesState);
     }
}



void checkForEthernetInput()
{
  /*
  if (  
       ammBusEthernet.receiveEthernetRequests(&e,&clock,&dt)
     )
     {  
      //setRelayState(ambs.valvesState);
     }*/
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


void loop()
{

  if (ambs.idleTicks==100) 
   {  
    ambs.idleTicks=0;   
    dt = realtime_clock.getDateTime();   
    ambs.currentTime = dt.unixtime;

    //Autostart stuff that needs to be autostarted
    ammBus_automaticTriggerStart(&ambs,dt.unixtime);
    
    //Valve Autopilot..
    valveAutopilot(); 
   } 
   

  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) 
  {
    digitalWrite (ledPin, HIGH);  // turn on the LED
    // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) 
    {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) 
      {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) 
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            
            if (header.indexOf("GET /auto/on") >= 0) 
                   {
                     ammBus_enableAutopilot(&ambs);
                     ammBus_scheduleAllValves(&ambs);
                   } else
           if (header.indexOf("GET /auto/off") >= 0) 
                   {
                     ammBus_stopAllValves(&ambs); 
                     ammBus_disableAutopilot(&ambs);
                   } else
            if (header.indexOf("GET /auto/low") >= 0) 
                   {
                      ambs.valvesTimes = ambs.valvesTimesLow;
                   }    else
            if (header.indexOf("GET /auto/normal") >= 0) 
                   {
                      ambs.valvesTimes = ambs.valvesTimesNormal;
                   }      else
            if (header.indexOf("GET /auto/high") >= 0) 
                   {
                      ambs.valvesTimes = ambs.valvesTimesHigh;
                   }         

            char receivedANewCommand=0;
            char switchPattern[64];
            for (int i=0; i<NUMBER_OF_SWITCHES; i++)
              { 
                snprintf(switchPattern,64,"GET /%u/on",i);
                if (header.indexOf(switchPattern) >= 0) 
                   {
                     Serial.print("GPIO ");
                     Serial.print(i);
                     Serial.print("-");
                     Serial.print(RELAY_ADDRESS[i]);
                     Serial.println(" on");
                     //ambs.valvesState[i]=ON; ambs.valvesScheduled[i]=OFF;  
                     ammBus_startValve(
                                       &ambs,
                                       i,
                                       ambs.valvesTimes[i]
                                      );
                     receivedANewCommand=1;
                   } 
                   //-------------------------------
                 snprintf(switchPattern,64,"GET /%u/off",i);
                if (header.indexOf(switchPattern) >= 0) 
                   {
                     Serial.print("GPIO ");
                     Serial.print(i);
                     Serial.print("-");
                     Serial.print(RELAY_ADDRESS[i]);
                     Serial.println(" off");
                     //ambs.valvesState[i]=OFF; ambs.valvesScheduled[i]=OFF;  
                     ammBus_stopValve(
                                      &ambs,
                                      i
                                     );
                     receivedANewCommand=1;
                   } 
              }
              
             if (receivedANewCommand)
             {
               setRelayState(ambs.valvesState);
             }



            //--------------------
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta http-equiv=\"refresh\" content=\"5; url=/\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>");
            client.print(systemName);
            client.print("<br>");
            client.print(systemVersion); 
            client.println("</h1>");

            

             byte week, day, month, hour, minute, second;
             unsigned short year;


             unixtimeToDate(
                             dt.unixtime,
                             &second,
                             &minute,
                             &hour,
                             &day,
                             &month,
                             &year
                            );

              client.print(" ");
              client.print(day);
              client.print("/");
              client.print(month);
              client.print("/");
              client.print(year);
              client.print(" ");
              if (hour<10) { client.print("0"); }
              client.print(hour);
              client.print(":");
              if (minute<10) { client.print("0"); }
              client.print(minute);
              client.print(":");
              if (second<10) { client.print("0"); }
              client.println(second);
              client.print("<br><br>");

         
                
            client.print(" Auto is : ");
            if (ammBus_getAutopilotState(&ambs))
             { 
               client.print("Enabled <br>"); 

               if (ambs.lastJobRunTimestamp>0)
               {
               client.print("Last Run : "); 
               unixtimeToDate(
                              ambs.lastJobRunTimestamp,
                              &second,
                              &minute,
                              &hour,
                              &day,
                              &month,
                              &year
                            );
               client.print(day);
               client.print("/");
               client.print(month);
               client.print("/");
               client.print(year);
               client.print(" ");
               if (hour<10) { client.print("0"); }
               client.print(hour);
               client.print(":");
               if (minute<10) { client.print("0"); }
               client.print(minute);
               client.print(":");
               if (second<10) { client.print("0"); }
               client.println(second);
               client.print("<br>");
               }
             } else
             { client.print("Disabled <br>"); }
            
            client.print(ambs.jobConcurrency); 
            client.print(" valves at a time<br>");

            client.print("Run every ");
            client.print(ambs.jobRunEveryXHours); 
            client.print(" hours<br>");

            
            client.print("Run @ ");
            client.print(ambs.jobRunAtXHour); 
            client.print(":");
            client.print(ambs.jobRunAtXMinute);  
            client.print("<br><br>");


             //VALVE STATE TABLE
             //--------------------------------------
             client.print("<center><table><tr>");
             for (int i=0; i<NUMBER_OF_SWITCHES; i++)
              { 
                client.print("<td>");
                client.print(valveLabels[i]);
                client.print("</td>");
              }
             client.print("</tr><tr>");

             for (int i=0; i<NUMBER_OF_SWITCHES; i++)
              { 
                client.print("<td>"); 
                 client.print(ambs.valvesTimes[i]); 
                client.print("</td>");
              }
 
             client.print("</tr><tr>");
             
             for (int i=0; i<NUMBER_OF_SWITCHES; i++)
              { 
                client.print("<td>");
                if (ambs.valvesState[i])
                {
                 client.print("ON");
                } else
                if (ambs.valvesScheduled[i]) 
                {
                 client.print("Sch");
                } else
                {
                 client.print("OFF");
                } 
                client.print("</td>");
              }
             client.print("</tr></table></center><br>");
             //--------------------------------------

             
            client.print("<p>Auto ");
              if (ammBus_getAutopilotState(&ambs))
               {
                client.println("on </p>");
                client.print("<p><a href=\"/auto/off\"><button class=\"button button2\">OFF</button></a></p>");
               } else
               {
                client.println("off </p>");
                client.print("<p><a href=\"/auto/on\"><button class=\"button\">ON</button></a></p>");
               }


            //Print our switches as buttons..
            //---------------------------------------------
            for (int i=0; i<NUMBER_OF_SWITCHES; i++)
              { 
               client.print("<p>GPIO ");
               client.print(RELAY_ADDRESS[i]);
               client.print(" - ");
               client.print(valveLabels[i]);
               client.print(" - ");

               if (ambs.valvesState[i])
               {
                client.println("on </p>");
                client.print("<p><a href=\"/");
                client.print(i);
                client.println("/off\"><button class=\"button button2\">OFF</button></a></p>");
               } else
               {
                client.println("off </p>");
                client.print("<p><a href=\"/");
                client.print(i);
                client.println("/on\"><button class=\"button\">ON</button></a></p>");
               }
              }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else 
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') 
        {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
    
    digitalWrite (ledPin, LOW);  // turn off the LED
  }


  ++ambs.idleTicks;
  delay(10);
}
