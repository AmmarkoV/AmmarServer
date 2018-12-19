#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "ammBus.h"

int logTemperature(
                    struct ammBusState * ambs,
                    uint32_t timestamp,
                    byte temperature
                   );

#endif
