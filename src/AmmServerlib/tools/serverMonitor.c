#include "serverMonitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "../network/file_server.h"
#include "../threads/threadedServer.h"


//This function prepares the content of  stats context , ( stats.content )
void * serveMonitorPage(struct AmmServer_DynamicRequest  * rqst)
{
  char buffer[4096]={0};
  struct AmmServer_Instance * instance = rqst->instance;
  snprintf(buffer,4096,
  "<html><head><meta http-equiv=\"refresh\" content=\"1;URL='monitor.html'\" /></head><body>\
   <h1>AMMARSERVER <a target=\"_new\" href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/tools/serverMonitor.c\">MONITOR PAGE</a></h1><hr> \
   active threads : %u <br>\
   active clients : %lu <br>\
   started recv calls : %u <br>\
   finished recv calls : %u <br>\
   actively served requests: %u <br>\
   current memory consumption ( cache ) : %u KB<br>\
   data sent/recvd : %lu KB/%lu KB<br>\
   <hr>",
   GetActiveHTTPServerThreads(instance),
   instance->statistics.filesCurrentlyOpen ,
   (unsigned int) instance->statistics.recvOperationsStarted,
   (unsigned int) instance->statistics.recvOperationsFinished ,
   (unsigned int) instance->statistics.recvOperationsStarted - instance->statistics.recvOperationsFinished ,
   cache_GetCacheSizeKB(instance),
   instance->statistics.totalUploadKB,
   (unsigned long) instance->statistics.totalDownloadBytes/1024
   );
  strcpy(rqst->content,buffer);


  unsigned int i=0;

  for (i=0; i<GetActiveHTTPServerThreads(instance); i++)
  {
   snprintf(buffer,4096," %u - <a href=\"monitor.html?stop=%u\">STOP</a> \n",i,i);
   strcat(rqst->content,buffer);
  }



  snprintf(buffer,4096,"</body></html>");
  strcat(rqst->content,buffer);


  rqst->contentSize=strlen(rqst->content);




   char command[1024]={0};
  if ( _GET(instance,rqst,(char*) "stop",command,1024) )
      {
         fprintf(stderr,"stop %u requested\n",atoi(command));
      }



  return 0;
}

