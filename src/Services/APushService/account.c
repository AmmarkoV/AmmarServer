#include "account.h"
#include <stdio.h>

int readAccountList(struct accountList * acc,const char * filename)
{
   unsigned int accNumber=0;
   acc->account[accNumber].lastLogin=0;
   snprintf(acc->account[accNumber].accountUsername,32,"admin");
   snprintf(acc->account[accNumber].accountPassword,32,"admin");
   snprintf(acc->account[accNumber].email,512,"local");

   acc->account[accNumber].numberOfAssociatedDevices=1;
   snprintf(acc->account[accNumber].device[0].serialNumber,8,"0001");


  return 1;
}
