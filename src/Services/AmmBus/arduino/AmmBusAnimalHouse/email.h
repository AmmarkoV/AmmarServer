#ifndef __EMAIL_H
#define __EMAIL_H

#include <SPI.h>
#include <Ethernet.h>


byte sendEmail(
                EthernetClient * client,
                uint32_t  timestamp,
                const char * mailServer,
                const char * fromAddress,
                const char * toAddress
              );

#endif
