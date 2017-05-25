/** @file dynamic_requests.h
* @brief Dynamic request handler , one of the most important parts of this library
* @author Ammar Qammaz (AmmarkoV)
* @bug Compression should be improved
*/

#ifndef DYNAMIC_REQUESTS_H_INCLUDED
#define DYNAMIC_REQUESTS_H_INCLUDED

#include "../AmmServerlib.h"

/**
* @brief Ask if dynamic content is available for this cache index
* @ingroup dynamicRequests
* @param An AmmarServer Instance
* @param Index of cache we want to ask for
* @retval 1=Availiable , 0=Not Availiable*/
int  dynamicRequest_ContentAvailiable(struct AmmServer_Instance * instance,unsigned int index);

/**
* @brief Handles and serves a dynamic request
* @ingroup dynamicRequests
* @param An AmmarServer Instance
* @param HTTPHeader containing the request done
* @param Resource Context for specific dynamic Request
* @param The filename that got requested , that might get rewritten
* @param Index of cache item , containing this dynamic request
* @param Memory Size allocated by the new dynamic request
* @param Outputs if compression was supported ( and used ) by client
* @param Outputs if client wants to free buffer on it's own or it should be handled automatically
* @param Outputs if client wants to allow other origins ( cross-site )
* @retval Pointer To New Content or ,0=Failed
* @bug Current implementation waits for new content , should add content double buffering to always have a valid buffer and serve it instantly , https://github.com/AmmarkoV/AmmarServer/issues/28
*/
char * dynamicRequest_serveContent
           (struct AmmServer_Instance * instance,
            struct HTTPHeader * request,
            struct AmmServer_RH_Context * shared_context,
            char * verified_filename,
            unsigned int verified_filenameLength,
            unsigned int index,
            unsigned long * memSize,
            unsigned char * compressionSupported,
            unsigned char * freeContentAfterUsingIt,
            unsigned char * contentContainsPathToFileToBeStreamed,
            unsigned char * allowOtherOrigins
          );

/**
* @brief Execute callback function associated with dynamic content , providing it with the http header it needs to output data to
* @ingroup dynamicRequests
* @param An AmmarServer Instance
* @param HTTPHeader containing the output of the request
* @retval 1=Ok,0=Failed*/
int callClientRequestHandler(struct AmmServer_Instance * instance,struct HTTPHeader * output);

/**
* @brief Save Dynamic request to a file ( for debugging it )
* @ingroup dynamicRequests
* @param ClientList
* @param ClientID we are talking about
* @param String of the resource
* @retval 1=Ok,0=Failed*/
int saveDynamicRequest(const char* filename , struct AmmServer_Instance * instance , struct AmmServer_DynamicRequest * rqst);

#endif // DYNAMIC_REQUESTS_H_INCLUDED
