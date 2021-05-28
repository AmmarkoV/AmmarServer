
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

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";


// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;


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
  Serial.begin(115200);
  
  pinMode (ledPin, OUTPUT);
  // Initialize the output variables as outputs

  for (int i=0; i<NUMBER_OF_RELAYS; i++)
  {
    pinMode(RELAY_ADDRESS[i], OUTPUT);
    digitalWrite(RELAY_ADDRESS[i], LOW);
  }
   

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
  
   
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
    if (!valves[i] )  {  digitalWrite(port, HIGH);   } else
                      {  digitalWrite(port, LOW);    }
  }  


  //Have a dedicated AC relay force it on or off
  //--------------------------------------------
  if (AC_RELAY)
    {
     unsigned int activeRelays = 0;
     for (i=0; i<NUMBER_OF_RELAYS; i++) 
      {  
        if (!valves[i] )  { activeRelays=activeRelays+1; }
      }

      
      if (activeRelays>0) { digitalWrite(RELAY_ADDRESS[AC_RELAY_PORT], HIGH); } else
                          { digitalWrite(RELAY_ADDRESS[AC_RELAY_PORT], LOW);  }
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
      //Open first possible scheduled valve 
      if ( (ambs.valvesScheduled[i]) && (!ambs.valvesState[i]) && (valvesRunning<ambs.jobConcurrency) )
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
  /*
  if ( 
       ammBusUSB.newUSBCommands(
                                ambs.valvesState,
                                ambs.valvesScheduled,
                                &clock,
                                &dt,
                                &ambs.idleTicks
                               ) 
     )
     {  
      setRelayState(ambs.valvesState);
     }*/
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
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) 
  { 
    // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) 
    {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              output26State = "on";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State = "off";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 off");
              output27State = "off";
              digitalWrite(output27, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Ammar Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            // If the output26State is off, it displays the ON button       
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
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
  }
}
