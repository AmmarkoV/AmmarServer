#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

#define MAXIMUM_NUMBER_OF_DEVICES 100

#include <time.h>

typedef  unsigned int deviceID;


#define TIME_IN_SECONDS_BETWEEN_EMAILS 3600
#define TIME_IN_SECONDS_TO_PRONOUNCE_A_DEVICE_LOST 1200
#define TIME_IN_SECONDS_TO_PRONOUNCE_A_DEVICE_DEAD 36000

enum DEVICE_CLASSES
{
    DEVICE_UNKOWN  = 0,
    DEVICE_THERMOMETER,
   //--------------------------------------------
    NUMBER_OF_DEVICE_CLASSES
};

extern const char * deviceClassName[];



struct informationList
{
  char alarm;
  float sensors[8];
  float temperatures[8];
  char relays[8];
};


struct deviceObject
{
  char    deviceNeedsPlot;
  char    deviceIsDead;
  char    deviceCommunicatingProperly;
  time_t  lastContact;
  time_t  lastEMailNotification;
  char deviceClass;
  char deviceLabel[32];
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


int updateDeviceHeartbeat(struct deviceObject *device,char alarmed,float temperature,float humidity);


int checkForDeadDevices(struct deviceList *dl);

int readDeviceAuthorizationList(struct deviceList * dl,const char * filename);

int getDeviceID(struct deviceList * dl,const char * serialNumber,deviceID * devID);

int isDeviceAutheticated(const char * deviceID, const char * devicePublicKey);


#endif // DEVICE_H_INCLUDED
