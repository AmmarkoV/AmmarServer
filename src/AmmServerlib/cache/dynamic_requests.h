#ifndef DYNAMIC_REQUESTS_H_INCLUDED
#define DYNAMIC_REQUESTS_H_INCLUDED

#include "../AmmServerlib.h"

int  dynamicRequest_ContentAvailiable(struct AmmServer_Instance * instance,unsigned int index);
char * dynamicRequest_serveContent
           (struct AmmServer_Instance * instance,
            struct HTTPRequest * request,
            struct AmmServer_RH_Context * shared_context
          );


#endif // DYNAMIC_REQUESTS_H_INCLUDED
