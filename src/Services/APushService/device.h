#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

#define MAXIMUM_NUMBER_OF_DEVICES 100

typedef  unsigned int deviceID;

struct informationList
{
  char alarm;
  char sensors[8];
  char temperatures[8];
  char relays[8];
};


struct deviceObject
{
  unsigned int lastContact;
  char deviceID[32];
  char devicePrivateKey[32];
  char devicePublicKey[32];
  char email[64];
  struct informationList info;

};

struct deviceList
{
   unsigned int numberOfDevices;
   struct deviceObject device[MAXIMUM_NUMBER_OF_DEVICES ];
};

int getDeviceID(const char * serialNumber,deviceID * devID);

int isDeviceAutheticated(const char * deviceID, const char * devicePublicKey);

int markDeviceIDAsUpdated(deviceID * devID);

#endif // DEVICE_H_INCLUDED
