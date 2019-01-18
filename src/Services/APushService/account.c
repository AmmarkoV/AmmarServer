#include "account.h"
#include <stdio.h>

int readAccountList(struct accountList * acc,const char * filename)
{
   unsigned int accNumber=0;
   //-------------------------------------------------------------------
   struct accountObject * account = &acc->account[accNumber];
   ++accNumber;
   account->lastLogin=0;
   snprintf(account->accountUsername,32,"admin");
   snprintf(account->accountPassword,32,"admin");
   snprintf(account->email,512,"local");
   account->numberOfAssociatedDevices=1;
   snprintf(account->device[0].serialNumber,8,"0001");
   //-------------------------------------------------------------------

   acc->numberOfAccounts=accNumber;
  return 1;
}
