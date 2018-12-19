#ifndef WEBSERVER_h
#define WEBSERVER_h

#include "ammBus.h"
#include <SPI.h>
#include <Ethernet.h>


int ServeWebServerClient(
                         struct ammBusState * ambs,
                         EthernetClient * client,
                         int temperature,
                         int humidity,
                         int tooHotCounter,
                         int tooColdCounter
                        );

#endif
