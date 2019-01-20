#include <ESP8266WiFi.h>
//Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to File->Preferences->Additional Board Manager URLs
//And then Tools->Board ******** -> Board Manager
#include "configuration.h"
 


const char* ssid     = "AmmarNetCrete"; // Your ssid
const char* password = ""; // Your Password
 
WiFiServer server(80); 

int tick=0;

int send(char * host,float temp,float hum)
{ 
  WiFiClient client;
  Serial.printf("\n[Connecting to %s ... ", host);
  if (client.connect(host, 8087))
  {
    Serial.println("connected]");

    Serial.println("[Sending a request]");
    client.print(String("GET /") + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n" +
                 "\r\n"
                );

    Serial.println("[Response:]");
    while (client.connected() || client.available())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    client.stop();
    Serial.println("\n[Disconnected]");
  }
  else
  {
    Serial.println("connection failed!]");
    client.stop();
  }
}



void setup() 
{
Serial.begin(115200);
delay(10);
Serial.println();

// Connect to WiFi network
WiFi.mode(WIFI_STA);
Serial.println();
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("");
Serial.println("WiFi connected");

// Start the server
server.begin();
Serial.println("Server started");

// Print the IP address
Serial.println(WiFi.localIP());


 dht.setup(pinDHT11, DHTesp::DHT11); // Connect DHT sensor to GPIO  
}

void loop() 
{
int err;
float temp, humi; 
 
delay(dht.getMinimumSamplingPeriod());

float humidity = dht.getHumidity();
float temperature = dht.getTemperature();

/*
if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))
{
  temp=(float) temperature;
  humi=(float) humidity;
  Serial.print("temperature:");
  Serial.print(temp);
  Serial.print(" humidity:");
  Serial.print(humi);
  Serial.println();
}
   else
{
  Serial.println();
  Serial.print("Error Sampling DHT :"); 
  Serial.println();
}*/

  Serial.print("temperature:");
  Serial.print(temp);
  Serial.print(" humidity:");
  Serial.print(humi);
  Serial.println();



WiFiClient client = server.available();
client.println("HTTP/1.1 200 OK");
client.println("Content-Type: text/html");
client.println("Connection: close");  // the connection will be closed after completion of the response
client.println("Refresh: 5");  // refresh the page automatically every 5 sec
client.println();
client.println("<!DOCTYPE html>");
client.println("<html xmlns='http://www.w3.org/1999/xhtml'>");
client.println("<head>\n<meta charset='UTF-8'>");
client.println("<title>ESP8266 Temperature & Humidity DHT11 Sensor</title>");
client.println("</head>\n<body>");
client.println("<H2>ESP8266 & DHT11 Sensor</H2>");
client.println("<H3>Humidity / Temperature</H3>");
client.println("<pre>");
client.print("Humidity (%)         : ");
client.println((float)humi, 2);
client.print("Temperature (°C)  : ");
client.println((float)temp, 2); 
client.println("</pre>"); 
client.print("</body>\n</html>");
delay(1500); //delay for reread


if (tick%100==99)
{
//  send("ammar.gr",temp,humi);
}

  ++tick;

}
