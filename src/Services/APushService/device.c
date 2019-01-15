#include <stdio.h>
#include <string.h>
#include "device.h"


int printDeviceList(struct deviceList * dl)
{

}


int readDeviceAuthorizationList(struct deviceList * dl,const char * filename)
{


   unsigned int dev=0;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_THERMOMETER;
   snprintf(dl->device[dev].deviceID,32,"0001");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"Animal Sensor 1");
   snprintf(dl->device[dev].email,512,"gregoriou@uoc.gr,ammarkov@gmail.com");


   ++dev;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_UNKOWN;
   snprintf(dl->device[dev].deviceID,32,"DUMMY");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"Dummy Sensor");
   snprintf(dl->device[dev].email,512,"ammarkov@gmail.com");


   ++dev;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_UNKOWN;
   snprintf(dl->device[dev].deviceID,32,"DUMMY");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"Dummy Sensor");
   snprintf(dl->device[dev].email,512,"ammarkov@gmail.com");

   dl->numberOfDevices=dev+1;
/*
  unsigned int ;
  char [32];
  char devicePrivateKey[32];
  char devicePublicKey[32];
  char email[64];
  struct informationList info;*/

  return 0;
}

int getDeviceID(struct deviceList * dl,const char * serialNumber,deviceID * devID)
{
   unsigned int i=0;
   for (i=0; i<dl->numberOfDevices; i++)
   {
     if (strcmp(dl->device[i].deviceID,serialNumber)==0)
     {
       *devID=i;
       return 1;
     }
   }

   return 0;
}



int updateDeviceHeartbeat(struct deviceObject *device,char alarmed,float temperature,float humidity)
{
 if (device==0) { return 1; }
  device->lastContact = time(NULL);
  device->info.alarm=alarmed;
  device->info.sensors[0]=humidity;
  device->info.temperatures[0]=temperature;
 return 1;
}


int isDeviceAutheticated(const char * deviceID, const char * devicePublicKey)
{
  return 1;
}
