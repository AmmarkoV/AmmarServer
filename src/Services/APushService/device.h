#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

#define MAXIMUM_NUMBER_OF_DEVICES 100

#include <time.h>

typedef  unsigned int deviceID;

struct informationList
{
  char alarm;
  float sensors[8];
  float temperatures[8];
  char relays[8];
};


struct deviceObject
{
  time_t  lastContact;
  char deviceID[32];
  char devicePrivateKey[32];
  char devicePublicKey[32];
  char email[512];
  struct informationList info;

};

struct deviceList
{
   unsigned int numberOfDevices;
   struct deviceObject device[MAXIMUM_NUMBER_OF_DEVICES];
};


int readDeviceAuthorizationList(struct deviceList * dl,const char * filename);

int getDeviceID(struct deviceList * dl,const char * serialNumber,deviceID * devID);

int isDeviceAutheticated(const char * deviceID, const char * devicePublicKey);

int updateDeviceHeartbeat(struct deviceList * dl,const char * serialNumber,char alarmed,float temperature,float humidity);

#endif // DEVICE_H_INCLUDED
