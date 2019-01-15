#include "utilities.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../AmmServerlib/AmmServerlib.h"


int sendEmail(
               const char * receipient,
               const char * subject,
               const char * message
             )
{
  char messageBuffer[1024]={0};
  char result[1024]={0};

  snprintf(messageBuffer,1024,"printf \"Subject:%s\n\n%s\" | ssmtp -v %s",subject,message,receipient);
  if (1)//filterStringForShellInjection(messageBuffer,512))
  {
   if ( AmmServer_ExecuteCommandLine(messageBuffer,result,512) )
   {
     fprintf(stderr,"Successfully sent message to %s..\n",receipient);
     return 1;
   }
  }


 AmmServer_Error("Failed to send message to %s..\n",receipient);
 return 0;
}
