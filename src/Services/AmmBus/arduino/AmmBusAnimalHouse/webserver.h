#ifndef WEBSERVER_h
#define WEBSERVER_h

#include <SPI.h>
#include <Ethernet.h>


int ServeWebServerClient(
                         EthernetClient * client,
                         int temperature,
                         int humidity,
                         int tooHotCounter,
                         int tooColdCounter
                        );

#endif
