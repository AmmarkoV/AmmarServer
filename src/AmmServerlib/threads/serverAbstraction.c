#include "serverAbstraction.h"
// --------------------------------------------
#include "threadedServer.h"
#include "libeventServer.h"

int ASRV_StartHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path)
{
 #if USE_LIBEVENT
  return StartLibEventHTTPServer(instance,ip,port,root_path,templates_path);
 #else
  return StartThreadedHTTPServer(instance,ip,port,root_path,templates_path);
 #endif // USE_LIBEVENT
}

int ASRV_StopHTTPServer(struct AmmServer_Instance * instance)
{
 #if USE_LIBEVENT
  return StopLibEventHTTPServer(instance);
 #else
  return StopThreadedHTTPServer(instance);
 #endif // USE_LIBEVENT
}

int ASRV_HTTPServerIsRunning(struct AmmServer_Instance * instance)
{
 #if USE_LIBEVENT
  return LibEventHTTPServerIsRunning(instance);
 #else
  return ThreadedHTTPServerIsRunning(instance);
 #endif // USE_LIBEVENT
}

unsigned int ASRV_GetActiveHTTPServerThreads(struct AmmServer_Instance * instance)
{
 #if USE_LIBEVENT
  return GetActiveLibEventHTTPServerThreads(instance);
 #else
  return GetActiveThreadedHTTPServerThreads(instance);
 #endif // USE_LIBEVENT
}
