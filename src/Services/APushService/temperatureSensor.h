#ifndef TEMPERATURESENSOR_H_INCLUDED
#define TEMPERATURESENSOR_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"
#include "device.h"


int logTemperature(
                   const char * filename,
                   const char * deviceID,
                   float temperature,
                   float  humidity,
                   char   alarmed
                  );

int getTemperatureAndHumidityFromRequest(struct AmmServer_DynamicRequest  * rqst,float * temperature , float * humidity);

void * temperatureSensorTestCallback(struct deviceObject *device,struct AmmServer_DynamicRequest  * rqst);
void * temperatureSensorAlarmCallback(struct deviceObject *device,struct AmmServer_DynamicRequest  * rqst);
void * temperatureSensorHeartBeatCallback(struct deviceObject *device,struct AmmServer_DynamicRequest  * rqst);

#endif // DEVICE_H_INCLUDED
