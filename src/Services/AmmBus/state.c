#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "state.h"
#include "serialIO.h"


int setAmmBusState(int deviceID , const char * dev , int state)
{
  char d=tolower(dev[0]);
  char D=toupper(dev[0]);


  if  ( (d>='a') && (d<='h') )
  {
    if (state) { serialport_writebyte(deviceID,D); } else
               { serialport_writebyte(deviceID,d); }
    return 1;
  }
 return 0;
}



int setAmmBusStateAll(int deviceID , int state)
{
   if (state) { return  serialport_writebyte(deviceID,'*'); } else
              { return  serialport_writebyte(deviceID,'$'); }


 return 0;
 }
