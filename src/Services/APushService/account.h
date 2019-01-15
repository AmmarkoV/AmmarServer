#ifndef ACCOUNT_H_INCLUDED
#define ACCOUNT_H_INCLUDED

#define MAXIMUM_NUMBER_OF_ACCOUNTS 100

typedef  unsigned int deviceID;

struct accountAssociatedDevices
{
  char serialNumber[8];
};


struct accountObject
{
  unsigned int lastLogin;
  char accountID[32];
  char accountPass[32];
  char email[64];

  unsigned int numberOfAssociatedDevices;
  struct accountAssociatedDevices device[10];
};

struct accountList
{
   unsigned int numberOfAccounts;
   struct accountObject account[MAXIMUM_NUMBER_OF_ACCOUNTS];
};


#endif // DEVICE_H_INCLUDED
