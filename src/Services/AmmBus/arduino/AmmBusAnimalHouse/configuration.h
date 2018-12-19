#ifndef __CONFIGURATION_H
#define __CONFIGURATION_H

#include <Arduino.h>
 
#define RESET_CLOCK_ON_NEXT_COMPILATION 0 

#define NUMBER_OF_RELAYS_INSTALLED 6

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] =  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// The IP address you want for your Ethernet card. Better to have it fixed than assigned by the router
// so that you can find it with your browser etc.
IPAddress ip(192, 168, 1, 177);

// The gateway is the address of your router, usually 192.168.1.1 or 192.168.0.1
IPAddress gateway(192, 168, 1, 1);

// 3. Unless you have a very advanced setup this will always be this value.
IPAddress subnet(255, 255, 255, 0);

char mailServer[] = "email.yourdomain.com";

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
EthernetClient client;


#endif
