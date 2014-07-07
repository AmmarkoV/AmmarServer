#ifndef DYNAMIC_REQUESTS_H_INCLUDED
#define DYNAMIC_REQUESTS_H_INCLUDED

#include "../AmmServerlib.h"

int  dynamicRequest_ContentAvailiable(struct AmmServer_Instance * instance,unsigned int index);
char * dynamicRequest_serveContent
           (struct AmmServer_Instance * instance,
            struct HTTPHeader * request,
            struct AmmServer_RH_Context * shared_context,
            unsigned int index,
            unsigned long * memSize,
            unsigned char * compressionSupported,
            unsigned char * freeContentAfterUsingIt
          );


int callClientRequestHandler(struct AmmServer_Instance * instance,struct HTTPHeader * output);


int saveDynamicRequest(char* filename , struct AmmServer_Instance * instance , struct AmmServer_DynamicRequest * rqst);

#endif // DYNAMIC_REQUESTS_H_INCLUDED
