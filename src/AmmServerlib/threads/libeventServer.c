#include "libeventServer.h"

#if USE_LIBEVENT
#warning "Please note that LibEvent support is under construction"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #include <windows.h>
 #include <io.h>
 #include <fcntl.h>
 #ifndef S_ISDIR
 #define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#else
 #include <sys/stat.h>
 #include <sys/socket.h>
 #include <signal.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <dirent.h>
#endif

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>

#ifdef EVENT__HAVE_NETINET_IN_H
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#endif


int StartLibEventHTTPServer(struct AmmServer_Instance * instance,const char * ip,unsigned int port,const char * root_path,const char * templates_path)
{
  return 0;
}

int StopLibEventHTTPServer(struct AmmServer_Instance * instance)
{
  return 0;
}

int LibEventHTTPServerIsRunning(struct AmmServer_Instance * instance)
{
  return 0;
}

unsigned int GetActiveLibEventHTTPServerThreads(struct AmmServer_Instance * instance)
{
  return 0;
}

#endif
