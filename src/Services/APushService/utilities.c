#include "utilities.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define DEACTIVATE_EMAIL 0



void generalFailureResponseToRequest(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><body>FAILED</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  rqst->content[rqst->contentSize]=0;
}

void generalSuccessResponseToRequest(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><body>OK</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  rqst->content[rqst->contentSize]=0;
}


int sendEmail(
               const char * receipient,
               const char * subject,
               const char * message
             )
{
  #if DEACTIVATE_EMAIL
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("            NOT SENDING MAIL \n");
      AmmServer_Error(" Recpt  : %s \n" , receipient);
      AmmServer_Error(" Subject  : %s \n" , subject);
      AmmServer_Error(" Message  : %s \n" , message);
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("----------------------------------------\n");
      AmmServer_Error("----------------------------------------\n");
  #else
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
  #endif // DEACTIVATE_EMAIL



 AmmServer_Error("Failed to send message to %s..\n",receipient);
 return 0;
}
