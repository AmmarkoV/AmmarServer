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
  struct AmmServer_Instance * instance = rqst->instance;
  snprintf(rqst->content,rqst->MAXcontentSize,
  "<html><head><meta http-equiv=\"refresh\" content=\"1;URL='monitor.html'\" /></head><body>\
   <h1>AMMARSERVER <a target=\"_new\" href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/tools/serverMonitor.c\">MONITOR PAGE</a></h1><hr> \
   active threads : %u <br>\
   active clients : %u <br>\
   active recv calls : %u <br>\
   served requests: %u <br>\
   current memory consumption ( cache ) : %u KB<br>\
   data sent/recvd : %lu KB/%lu KB<br>\
   <hr>\
   </body></html>",
   GetActiveHTTPServerThreads(instance),
   instance->statistics.filesCurrentlyOpen ,
   (unsigned int) instance->statistics.recvOperationsStarted-instance->statistics.recvOperationsFinished ,
   cache_GetCacheSizeKB(instance),
   instance->statistics.totalUploadKB,
   instance->statistics.totalDownloadKB
   );
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

