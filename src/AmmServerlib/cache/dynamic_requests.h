/** @file dynamic_requests.h
* @brief Dynamic request handler , one of the most important parts of this library
* @author Ammar Qammaz (AmmarkoV)
* @bug Compression should be improved
*/

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


/**
* @brief Save Dynamic request to a file
* @ingroup dynamicRequests
* @param ClientList
* @param ClientID we are talking about
* @param String of the resource
* @retval 1=Ok,0=Failed*/
int saveDynamicRequest(char* filename , struct AmmServer_Instance * instance , struct AmmServer_DynamicRequest * rqst);

#endif // DYNAMIC_REQUESTS_H_INCLUDED
