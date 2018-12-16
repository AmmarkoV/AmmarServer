

#include <SPI.h>
#include <Ethernet.h>

#define RESET_CLOCK_ON_NEXT_COMPILATION 0 

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 177);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);


/*
Please note as I have used the android app you told , the string inputs are taken as
A,B,C,D,E,F,G and a,b,c,d,e,f,g.
I have also written code for voice on and off which has all on and all off function
but since the android app we are using dosent have such key so i didnt took them
*/
String inputs;
#define relay1 3 //Connect relay1 to pin 9
#define relay2 4 //Connect relay2 to pin 8
#define relay3 5 //Connect relay3 to pin 7
#define relay4 6 //Connect relay4 to pin 6
#define relay5 7 //Connect relay5 to pin 5
#define relay6 8 //Connect relay6 to pin 4
#define relay7 9 //Connect relay7 to pin 3 

unsigned int tooColdCounter=0;
unsigned int minimumTemperature=10;
unsigned int tooHotCounter=0;
unsigned int maximumTemperature=25;

//DHT11 -----------------------------------------------------
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 9
#include "SimpleDHT.h"
int pinDHT11 = 9;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;
byte dataDHT11[40] = {0};
uint32_t lastDHT11SampleTime=0;
//-----------------------------------------------------------


//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------


void setup()
{
Serial.begin(9600); //Set rate for communicating with phone
pinMode(relay1, OUTPUT); //Set relay1 as an output
pinMode(relay2, OUTPUT); //Set relay2 as an output
pinMode(relay3, OUTPUT); //Set relay1 as an output
pinMode(relay4, OUTPUT); //Set relay2 as an output
pinMode(relay5, OUTPUT); //Set relay1 as an output
pinMode(relay6, OUTPUT); //Set relay2 as an output 
digitalWrite(relay1, HIGH); //Switch relay1 off
digitalWrite(relay2, HIGH); //Swtich relay2 off
digitalWrite(relay3, HIGH); //Switch relay1 off
digitalWrite(relay4, HIGH); //Swtich relay2 off
digitalWrite(relay5, HIGH); //Switch relay1 off
digitalWrite(relay6, HIGH); //Swtich relay2 off 

   Serial.print("Starting Up Clock! \n");
   clock.begin();

   // Set sketch compiling time
   if (RESET_CLOCK_ON_NEXT_COMPILATION)
       { clock.setDateTime(__DATE__, __TIME__); } 
   dt = clock.getDateTime();   

  Serial.println("Ethernet WebServer Example");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());


}



void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) 
        {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>"); 
          
            client.println("UnixTime:"); 
            client.println(dt.unixtime); 
            client.println("<br />"); 
            //------------------------
            client.print("DHT11: ");
            client.print((int)temperature);
            client.print(" *C ");
            client.print((int)humidity);
            client.println("%<br />"); 

            if ( tooHotCounter > 0 )
              {
                client.print("Hot Alarms : ");
                client.print((int)tooHotCounter); 
                client.println("<br />"); 
              }

            if ( tooColdCounter > 0 )
              {
                client.print("Cold Alarms : ");
                client.print((int)tooColdCounter); 
                client.println("<br />"); 
              }
          
          client.println("</html>");
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
    client.stop();
    Serial.println("client disconnected");
  }
  
  if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  
   {
     //Serial.write("\nDHT11 error\n"); 
   } else
   { 
     Serial.print("DHT11 OK: ");
     Serial.print((int)temperature); Serial.print(" *C, ");
     Serial.print((int)humidity); Serial.println(" %");

     if (temperature<minimumTemperature) 
      { 
        if (tooColdCounter<10000) { tooColdCounter++; }
      }  

     if (temperature>maximumTemperature) 
      { 
        if (tooHotCounter<10000) { tooHotCounter++; }
      }   
      
   } 
   
  
while(Serial.available()) //Check if there are available bytes to read
{
delay(10); //Delay to make it stable
char c = Serial.read(); //Conduct a serial read
if (c == '#'){
break; //Stop the loop once # is detected after a word
}
inputs += c; //Means inputs = inputs + c
}
if (inputs.length() >0)
{
Serial.println(inputs);


if(inputs == "*")
{
 digitalWrite(relay1, LOW);
 digitalWrite(relay2, LOW);
 digitalWrite(relay3, LOW);
 digitalWrite(relay4, LOW);
 digitalWrite(relay5, LOW);
 digitalWrite(relay6, LOW); 
}
else 
if(inputs == "$")
{
 digitalWrite(relay1, HIGH);
 digitalWrite(relay2, HIGH);
 digitalWrite(relay3, HIGH);
 digitalWrite(relay4, HIGH);
 digitalWrite(relay5, HIGH);
 digitalWrite(relay6, HIGH); 
} 
else
if(inputs == "A")
{
digitalWrite(relay1, LOW);
}
else if(inputs == "a")
{
digitalWrite(relay1, HIGH);
}
else if(inputs == "B")
{
digitalWrite(relay2, LOW);
}
else if(inputs == "b")
{
digitalWrite(relay2, HIGH);
}
else if(inputs == "C")
{
digitalWrite(relay3, LOW);
}
else if(inputs == "c")
{
digitalWrite(relay3, HIGH);
}
else if(inputs == "D")
{
digitalWrite(relay4, LOW);
}
else if(inputs == "d")
{
digitalWrite(relay4, HIGH);
}
else if(inputs == "E")
{
digitalWrite(relay5, LOW);
}
else if(inputs == "e")
{
digitalWrite(relay5, HIGH);
}
else if(inputs == "F")
{
digitalWrite(relay6, LOW);
}
else if(inputs == "f")
{
digitalWrite(relay6, HIGH);
}
else
{
 Serial.print("No Input\n");
 }
inputs="";
}
}
