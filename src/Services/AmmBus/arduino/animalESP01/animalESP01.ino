#include <ESP8266WiFi.h>
//Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to File->Preferences->Additional Board Manager URLs
//And then Tools->Board: "********" -> Board Manager and type esp8266 in the search field to download the esp8266 board from the ESP8266 Community 
 

#if USE_ENCRYPTION  
#include <AESLib.h>
//cd ~/Arduino/libraries && git clone https://github.com/DavyLandman/AESLib
#include <aes128_enc.h> 
#endif

#include "configuration.h"
 
unsigned int incorrectRequests=0;
char requestsPending=0;


static unsigned long timer=0;
static unsigned long timerTemp=0;
static unsigned long timerLedOn=0;
static unsigned long timerTest=0;
static unsigned long timerConnectionError=0;


char sTmp[6]={0};
char sHum[6]={0};

//We begin by forcing update..!
char forceUpdate=1;

char  criticalTemperature=0;
float lowestAcceptableTemperature=19.0;
float highestAcceptableTemperature=27.0;

float temperatureAvg=0.0;
float humidityAvg=0.0;
float temperatureSum=0;
float humiditySum=0;
int   numberOfDHT11Samples=0;

 
 
WiFiServer server(80); 
 

int serverWebsite()
{ 
 WiFiClient client = server.available();
 client.println("HTTP/1.1 200 OK");
 client.println("Content-Type: text/html");
 client.println("Connection: close");  // the connection will be closed after completion of the response
 client.println("Refresh: 10");  // refresh the page automatically every 5 sec
 client.println();
 client.println("<!DOCTYPE html>");
 client.println("<html xmlns='http://www.w3.org/1999/xhtml'>");
 client.println("<head>\n<meta charset='UTF-8'>");
 client.println("<title>Temperature & Humidity Sensor</title>");
 client.println("</head>\n<body>");
 client.println("<H2>Temperature & Humidity Sensor</H2>");
 client.println("<H3>Humidity / Temperature</H3>");
 client.println("<pre>");
 client.print("Humidity (%)         : ");
 client.println(humidityAvg,2);
 client.print("Temperature (°C)  : ");
 client.println(temperatureAvg,2); 
 client.println("</pre><br>"); 
 client.println("<H4>Powered by <a href=\"https://github.com/AmmarkoV/AmmarServer/wiki\">AmmarServer</a></H4>");
 client.print("</body>\n</html>");
 return 1;
}


void initializeWirelessClient()
{  
 // Connect to WiFi network
 WiFi.mode(WIFI_STA);
 Serial.println();
 Serial.println();
 Serial.print("Connecting to ");
 Serial.println(ssid); 

 WiFi.hostname(deviceName);      // DHCP Hostname (useful for finding device for static lease)
 #if STATIC_IP
  WiFi.config(ip, dns, gateway, subnet);
 #endif
 
 WiFi.begin(ssid, password);

 
 
 while (WiFi.status() != WL_CONNECTED) 
  {
   delay(EXPECTED_RESPONSE_LATENCY);
   Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  return ;
}




void readTemperature(int coldRun)
{
  //DHT requires sampling at 1Hz
  humidity    = dht.getHumidity();
  temperature = dht.getTemperature();

  if ((temperature!=temperature) || ( humidity!=humidity))
   {
      //Bad Reading..
     // Serial.print("!");
   } 
    else 
   { 
    // Serial.print("Temperature : ");
    // Serial.print(temperature);
    // Serial.print(" Humidity : ");
    // Serial.println(humidity);
     //----------------------
     temperatureSum+=temperature;
     humiditySum+=humidity;
     ++numberOfDHT11Samples;

     if (numberOfDHT11Samples==NUMBER_OF_TEMPERATURE_SAMPLES)
     {
      temperatureAvg=(float) temperatureSum/numberOfDHT11Samples;
      humidityAvg=(float) humiditySum/numberOfDHT11Samples;
      temperatureSum=0;
      humiditySum=0;
      numberOfDHT11Samples=0;   
    
      Serial.print(F("Temperature Avg: "));
      Serial.print(temperatureAvg);
      Serial.print(F("oC\n"));

      Serial.print(F("Humidity Avg: "));
      Serial.print(humidityAvg);
      Serial.print(F("%\n"));
          
      Serial.print(F("Time until next update:"));
      Serial.print(millis()-timer);
      Serial.print("/");
      
      if (criticalTemperature)  { Serial.println(REQUEST_RATE_CRITICAL); } else
                                { Serial.println(REQUEST_RATE_NORMAL);   } 
      
      
     // notifyTemperature();
     }

     //First cold run will set the average temperature to keep thermometer from freaking out.. 
     if (coldRun)
      {
       temperatureAvg=(float) temperature;
       humidityAvg=(float) humidity;    
      }
     
     criticalTemperature = 0;
     if ( (lowestAcceptableTemperature>temperatureAvg) || (highestAcceptableTemperature<temperatureAvg) )
     { 
      criticalTemperature=1;
     }

     
   /*
    Serial.print("Temperature:"); Serial.print(temperature); Serial.print("oC\n");
    Serial.print("Humidity:");    Serial.print(humidity);    Serial.print("%\n");*/
  } 
  return ; 
}




int WifiGETRequest(const char * host,unsigned int port,const char * page,const char * variables)
{ 
  char success=0;
  WiFiClient client;
  Serial.printf("\n[Connecting to %s ... ", host);

  char locallyAllocatedHost[64]={0};
  snprintf(locallyAllocatedHost,63,host);
  if (client.connect(locallyAllocatedHost, port))
  {
    Serial.println("connected]");

    Serial.println("[Sending a request]");

    char request[256]={0};
    snprintf(request,255,"GET %s%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",page,variables,host);

    client.print(request);
    Serial.println(request);
    //------------------------------------------------------------------
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
        delay(1000);
        if (millis() - timeout > MAXIMUM_RESPONSE_LATENCY_MILLISECONDS) 
        {
            Serial.println(">>> Client Timeout !");
            Serial.print(incorrectRequests); 
            Serial.print("/"); 
            Serial.println(NUMBER_OF_FAILED_ATTEMPTS_TO_RESET); 
            ++incorrectRequests;
            client.stop();
            return 0;
        }
    }
    //------------------------------------------------------------------
    Serial.println("[Response:]");
    while(client.available()) 
    {
        String line = client.readStringUntil('\n');
        if (strstr(line.c_str(),"<body>OK</body>")!=0) { success=1; }
        Serial.println(line);
    }
    client.stop();
    Serial.println("\n[Disconnected]");

    if (success) 
     {
        Serial.println("Successfully sent\n"); 
        incorrectRequests=0; // If we succeeded sending it means everything is fine..
     } else
     {
        Serial.print("Failed to send "); 
        Serial.print(incorrectRequests); 
        Serial.print("/"); 
        Serial.println(NUMBER_OF_FAILED_ATTEMPTS_TO_RESET); 
        ++incorrectRequests;
     }
    return success;
  }
  else
  {
    Serial.print(incorrectRequests); 
    Serial.print("/"); 
    Serial.print(NUMBER_OF_FAILED_ATTEMPTS_TO_RESET); 
    Serial.println(" connection failed!]");
    client.stop(); 
    ++incorrectRequests;
  }
 return 0;
}


void setup() 
{
 Serial.begin(115200);
 delay(10);
 Serial.println();

 incorrectRequests=0;
 initializeWirelessClient();

 dht.setup(pinDHT11, DHTesp::DHT11); // Connect DHT sensor to GPIO  
 
 Serial.println("Everything ready, just waiting to sample DHT11 sensor..");
 delay(dht.getMinimumSamplingPeriod());
 delay(100);
 
 Serial.println("Getting 3 samples..");
 readTemperature(1);  //First cold run..
 delay(100+dht.getMinimumSamplingPeriod());     
 readTemperature(0);  //Second cold run..
 delay(100+dht.getMinimumSamplingPeriod());     
 readTemperature(0);  //Third cold run..
  
 Serial.println("Let's Go..");
 forceUpdate=1;
 return ;
}

void loop() 
{  
  delay(100+dht.getMinimumSamplingPeriod());     
  
  ///----------------------------------------------------------------------------------
  //                               READ TEMPERATURE READINGS
  ///----------------------------------------------------------------------------------
   //Read DHT11 Sensor -----------------------  
    if (millis() > timerTemp + dht.getMinimumSamplingPeriod() ) 
     {
      timerTemp = millis();
      readTemperature(0); 
     }    
  ///----------------------------------------------------------------------------------



 serverWebsite();
 

 char request[64]={0}; 
   ///----------------------------------------------------------------------------------
   //                         NOTIFY ABOUT TEMPERATURE READINGS
   ///----------------------------------------------------------------------------------
   if (criticalTemperature)
    {
       if ( (millis() > timer + REQUEST_RATE_CRITICAL) || (forceUpdate) )
        {  
         Serial.println(F("\n>>> REQ_CRITICAL"));
         
         /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
         dtostrf(temperatureAvg, 4, 2, sTmp);
         dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,63,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);

         #if USE_ENCRYPTION  
          aes128_enc_single(privateKey,request);
         #endif
       
         Serial.println(request);
          
         if ( WifiGETRequest(website,8087,"/alarm.html",request) )
         {
           timer = millis();  
           forceUpdate=0;
         }
        }  
    } 
      else
    {
     if ( (millis() > timer + REQUEST_RATE_NORMAL) || (forceUpdate) )
       {   
        Serial.println(F("\n>>> REQ_NORMAL"));
        
         /* 4 is mininum width, 2 is precision; float value is copied onto sTmp and sHum since avr doesn't support printf("%f")*/
         dtostrf(temperatureAvg, 4, 2, sTmp);
         dtostrf(humidityAvg, 4, 2, sHum);
         snprintf(request,63,"?s=%s&k=%s&t=%s&h=%s",serialNumber,publicKey,sTmp,sHum);
         
         #if USE_ENCRYPTION  
          aes128_enc_single(privateKey,request);
         #endif
       
         Serial.println(request);
         
          if ( WifiGETRequest(website,8087,"/push.html",request) )
          {
           timer = millis();  
           forceUpdate=0;
          }
       } 
    }
   ///----------------------------------------------------------------------------------


   if (incorrectRequests>NUMBER_OF_FAILED_ATTEMPTS_TO_RESET)
   {
     Serial.println(F("\n>>> REACHED MAXIMUM FAILED ATTEMPT LIMIT"));
     delay(1000);
     ESP.restart(); //ESP.reset();
   } 
   
}
