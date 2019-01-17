#ifndef ACCOUNT_H_INCLUDED
#define ACCOUNT_H_INCLUDED

#define MAXIMUM_NUMBER_OF_ACCOUNTS 100

#include <time.h>

typedef  unsigned int accountID;

struct accountAssociatedDevices
{
  char serialNumber[8];
};


struct accountObject
{
  time_t lastLogin;
  char accountUsername[32];
  char accountPassword[32];
  char email[128];

  unsigned int numberOfAssociatedDevices;
  struct accountAssociatedDevices device[10];
};

struct accountList
{
   unsigned int numberOfAccounts;
   struct accountObject account[MAXIMUM_NUMBER_OF_ACCOUNTS];
};


int readAccountList(struct accountList * acc,const char * filename);

#endif // DEVICE_H_INCLUDED
