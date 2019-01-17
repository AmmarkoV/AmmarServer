#include <stdio.h>
#include <string.h>
#include "device.h"

#include "utilities.h"

int printDeviceList(struct deviceList * dl)
{
  return 0;
}


int readDeviceAuthorizationList(struct deviceList * dl,const char * filename)
{
   unsigned int dev=0;
   dl->device[dev].deviceIsDead=0;
   dl->device[dev].deviceCommunicatingProperly=1;
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
   snprintf(dl->device[dev].email,512,"local");


   ++dev;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_UNKOWN;
   snprintf(dl->device[dev].deviceID,32,"DUMMY");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"Dummy Sensor");
   snprintf(dl->device[dev].email,512,"local");

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
  device->deviceCommunicatingProperly=1;
  device->deviceIsDead=0;
 return 1;
}


int checkForDeadDevices(struct deviceList *dl)
{
   if (dl==0) { return 0; }

   time_t clock = time(NULL);
   struct tm * ptm = gmtime ( &clock );

   char message[512];


   unsigned int i=0;
   for (i=0; i<dl->numberOfDevices; i++)
   {
      struct deviceObject * device = &dl->device[i];

      if (clock - device->lastContact > TIME_IN_SECONDS_TO_PRONOUNCE_A_DEVICE_LOST )
      {
          if ( (device->deviceCommunicatingProperly) && (!device->deviceIsDead) )
          {
            //We just lost this device..
            device->deviceCommunicatingProperly=0;

            snprintf(message,512,"The sensor `%s` has been disconnected  @ %u/%u/%u %u:%u:%u Hopefully this is just a power black-out or temporary network problem.",
                    device->deviceLabel,ptm->tm_mday,1+ptm->tm_mon,EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

            if ( sendEmail( device->email, "Sensor Disconnected", message ) )
               {
                device->lastEMailNotification=clock;
               }
          } else
          {
            //We have already lost the device, do not keep spamming
            if (clock - device->lastContact > TIME_IN_SECONDS_TO_PRONOUNCE_A_DEVICE_DEAD )
            {
              //If it is so much time
              device->deviceIsDead=1;

              snprintf(message,512,"The sensor `%s` has been disconnected  @ %u/%u/%u %u:%u:%u Hopefully this is just a power black-out or temporary network problem.",
                    device->deviceLabel,ptm->tm_mday,1+ptm->tm_mon,EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

              if ( sendEmail( device->email, "Sensor Dead?", message ) )
               {
                device->lastEMailNotification=clock;
               }
            }
          }
      }
   }

   return 1;
}


int isDeviceAutheticated(const char * deviceID, const char * devicePublicKey)
{
  return 1;
}
