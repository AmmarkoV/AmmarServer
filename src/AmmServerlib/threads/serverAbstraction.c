#include "serverAbstraction.h"
// --------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --------------------------------------------
#include "threadedServer.h"
#include "libeventServer.h"



int ASRV_StartHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path)
{
 return StartThreadedHTTPServer(instance,ip,port,root_path,templates_path);
}

int ASRV_StopHTTPServer(struct AmmServer_Instance * instance)
{
 return StopThreadedHTTPServer(instance);
}

int ASRV_HTTPServerIsRunning(struct AmmServer_Instance * instance)
{
 return ThreadedHTTPServerIsRunning(instance);
}

unsigned int ASRV_GetActiveHTTPServerThreads(struct AmmServer_Instance * instance)
{
 return GetActiveThreadedHTTPServerThreads(instance);
}

