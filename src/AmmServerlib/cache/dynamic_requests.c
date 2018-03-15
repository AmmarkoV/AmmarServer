#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dynamic_requests.h"
#include "file_caching.h"
#include "session_list.h"
#include "../server_configuration.h"
#include "../tools/logs.h"
#include "../tools/time_provider.h"


/**
* @brief An enumerator that lists the types of requests fields availiable for a POST / GET / COOKIE or FILE request
*/
enum TimeStates
{
   TIME_IS_OK_SERVE_FRESH               = 0 ,
   TIME_IS_TOO_SLOW_SERVE_NOTHING           ,
   TIME_IS_TOO_FAST_SERVE_CACHED
};



int  dynamicRequest_ContentAvailiable(struct AmmServer_Instance * instance,unsigned int index)
{
  if (instance==0) { return 0; }
  struct cache_item * cache = (struct cache_item *) instance->cache;
  if (cache==0) { return 0; }

  return (cache[index].dynamicRequestCallbackFunction!=0);
}

int saveDynamicRequest(const char * filename , struct AmmServer_Instance * instance , struct AmmServer_DynamicRequest * rqst)
{
  if (instance==0) { return 0; }
  if (rqst==0)     { return 0; }

  AmmServer_Stub("saveDynamicRequest is a stub..");
  FILE *fp=0;

  fp = fopen(filename,"w");
  if (fp!=0)
  {
    //fprintf(fp,"%s",rqst->GET_request);
    fclose(fp);
    return 1;
  }
  return 0;
}

int callClientRequestHandler(struct AmmServer_Instance * instance,struct HTTPHeader * output)
{
  if ( instance->clientRequestHandlerOverrideContext == 0 )  { return 0; }
  struct AmmServer_RequestOverride_Context * clientOverride = instance->clientRequestHandlerOverrideContext;
  if ( clientOverride->request_override_callback == 0 ) { return 0; }

  clientOverride->request = output;

  fprintf(stderr,"doing callClientRequestHandler \n");
  void ( *DoCallback) ( struct AmmServer_RequestOverride_Context * ) = 0 ;
  DoCallback = clientOverride->request_override_callback;

  DoCallback(clientOverride);

  //After getting back the override and whatnot , keep the client from using a potentially bad
  //memory chunk
  clientOverride->request = 0;

  return 1;
}


void printRequestData(struct AmmServer_DynamicRequest * rqst)
{
 fprintf(stderr,"Request for a maximum of %lu characters\n",rqst->MAXcontentSize);
 if ( rqst->GETItemNumber!=0 )    { fprintf(stderr,"GETItems : %p , %u items\n",rqst->GETItem , rqst->GETItemNumber );          }
 if ( rqst->POSTItemNumber!=0 )   {
                                    fprintf(stderr,"POSTItems : %p , %u items\n",rqst->POSTItem , rqst->POSTItemNumber );
                                    fprintf(stderr,CYAN "Using the new POST system.. may our client have mercy on our POST request..\n" NORMAL);
                                  }
 if ( rqst->COOKIEItemNumber!=0 ) { fprintf(stderr,"COOKIEItems : %p , %u items\n",rqst->COOKIEItem , rqst->COOKIEItemNumber ); }
}

int checkRequestFrequency(
                          struct AmmServer_Instance * instance,
                          struct HTTPHeader * request,
                          struct AmmServer_RH_Context * shared_context,
                          unsigned long * now
                         )
{
     *now=0; //If there is no callback limits the time of the call will always be 0
     if (
          (shared_context-> callback_every_x_msec!=0) &&
          (shared_context->needsSamePageForAllClients)
        )
     { //Only Dynamic pages with time limits have to call the "expensive" GetTickCountAmmServ
       *now=GetTickCountAmmServ();
       if ( *now-shared_context->last_callback < shared_context-> callback_every_x_msec )
        {
         unsigned int waitTime=0;
         unsigned int maxWaitTime=0;
         /* ------------------------------------------
           TODO:
           Instead of waiting here have a double buffer content and serve the old one ..!
           along with a not modified header..!
         */
         if (CLIENT_SLEEP_TIME_WHEN_DYNAMIC_REQUEST_CALLBACK_IS_BUSY_NSEC>0)
         {
          maxWaitTime= (unsigned int) CLIENT_SLEEP_TIME_WHEN_DYNAMIC_REQUEST_CALLBACK_IS_BUSY_NSEC / CLIENT_SLEEP_TIME_INTERVAL_NSEC ;
          fprintf(stderr,"Hit while another thread executing callback , waiting %u intervals of %u nsec..",maxWaitTime,(unsigned int) CLIENT_SLEEP_TIME_INTERVAL_NSEC );
          while ( (shared_context->executedNow) && (waitTime < maxWaitTime) )
          {
           usleep(CLIENT_SLEEP_TIME_INTERVAL_NSEC );
           ++waitTime;
           fprintf(stderr,"_");
          }
         }

         if (waitTime>=maxWaitTime)
         {
           AmmServer_Error("Request requests for a callback that is TOO slow , returning nothing back :( ..\n");
           return TIME_IS_TOO_SLOW_SERVE_NOTHING;
         } else
         {
          AmmServer_Success("Request gets canned dynamic request page ( size %u , callback every %u msec )..\n",shared_context->requestContext.contentSize , shared_context-> callback_every_x_msec);
          //The request came too fast.. We will serve our existing file..!
          return TIME_IS_TOO_FAST_SERVE_CACHED;
         }
        } else
        {
         fprintf(stderr,"Request deserves fresh page , %u last gen, %lu now , %u cooldown\n",shared_context->last_callback,*now,shared_context-> callback_every_x_msec);
         return TIME_IS_OK_SERVE_FRESH;
        }
      }
}









char * dynamicRequest_serveContent
          (
            struct AmmServer_Instance * instance,
            struct HTTPHeader * request,
            struct AmmServer_RH_Context * shared_context ,
            char * verified_filename,
            unsigned int verified_filenameLength,
            unsigned int index,
            unsigned long * memSize,
            unsigned char * compressionSupported,
            unsigned char * freeContentAfterUsingIt,
            unsigned char * contentContainsPathToFileToBeStreamed,
            unsigned char * allowOtherOrigins
          )
{
  #if DISABLE_DYNAMIC_REQUESTS
   error("Dynamic requests are disabled until further notice .. \n");
   return 0;
  #endif // DISABLE_DYNAMIC_REQUESTS

  //Before returning any pointers we will have to ask ourselves.. Is this a Dynamic Content Cache instance ?
  *contentContainsPathToFileToBeStreamed=0;

  if (shared_context->dynamicRequestCallbackFunction==0)
  {
    struct cache_item * cache = (struct cache_item *) instance->cache;
    if (cache[index].dynamicRequestCallbackFunction==0)
    {
     errorID(ASV_ERROR_NO_CALLBACK_REGISTERED);
     return 0;
    } else
    {
     errorID(ASV_ERROR_CALLBACK_POINTER_CORRUPTION);
     shared_context->dynamicRequestCallbackFunction = cache[index].dynamicRequestCallbackFunction;
     //We were able to find the original callback function pointer ID through the cache so we can continue, we also emitted an error..
    }
  }

  *allowOtherOrigins = shared_context->allowCrossRequests;

  char * cacheMemory=0; // <- this will hold the resulting page
  //cacheMemory =  shared_context->requestContext.content;

  //Before doing callback we might want to allocate a different response space dedicated to this callback instead to using
  //one common memory buffer for every client...!
  if ( (shared_context->needsDifferentPageForEachClient) )
    {
     unsigned int size_to_allocate =  sizeof(char) * ( shared_context->requestContext.MAXcontentSize ) ;
     if (size_to_allocate==0)
     { warningID(ASV_WARNING_RESOURCE_HAS_ZERO_ACCOMODATION_SIZE); }
     else
     {
      fprintf(stderr,"Allocating an additional %u bytes for this request \n",size_to_allocate);
      cacheMemory = (char *) malloc( size_to_allocate );
      if (cacheMemory!=0) { *freeContentAfterUsingIt=1; } else //Allocation was successfull , we would like parent procedure to free it after use..
                          { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); } //Lets work with our default buffer till the end..!
     }
    } else
   if ( (shared_context->needsSamePageForAllClients) )
    {
       cacheMemory =  shared_context->requestContext.content;
    } else
    {
      errorID(ASV_ERROR_INVALID_RH_SCENARIO);
    }

  //In case mem doesnt point to a proper buffer calling the mem_callback function will probably segfault for all we know
  //So we bail out and emmit an error message..!
  if ( (cacheMemory==0) || (shared_context->requestContext.MAXcontentSize==0) )
    {
     warningID(ASV_WARNING_NOT_CALLING_CALLBACK_WITH_EMPTY_BUFFER);
     return 0;
    } else
    {
     //This means we can call the callback to prepare the memory content..! START
     unsigned long now=0; //If there is no callback limits the time of the call will always be 0
     //That doesnt bother anything or anyone..

     int requestFrequencyResult = checkRequestFrequency(
                                                         instance,
                                                         request,
                                                         shared_context,
                                                         &now
                                                        );
     switch (requestFrequencyResult)
     {
        case TIME_IS_TOO_SLOW_SERVE_NOTHING :
          //Request requests for a callback that is TOO slow , returning nothing back :( ..\n
          return 0;
        break;
        case TIME_IS_TOO_FAST_SERVE_CACHED  :
          *compressionSupported=0;
          shared_context->callback_cooldown=1;
          *memSize=shared_context->requestContext.contentSize;
          return cacheMemory;
        break;
        //case TIME_IS_OK_SERVE_FRESH       :
        //  fprintf(stderr,"Serving fresh page\n");
        //break;
     };

       //fprintf(stderr,"Internal mem_callback = %p \n",shared_context->dynamicRequestCallbackFunction);
       shared_context->callback_cooldown=0;
       shared_context->last_callback = now;

        struct AmmServer_DynamicRequest * rqst = (struct AmmServer_DynamicRequest * ) malloc(sizeof(struct AmmServer_DynamicRequest));
        if (rqst!=0)
                    {
                     memcpy(rqst , &shared_context->requestContext , sizeof( struct AmmServer_DynamicRequest ));

                     rqst->POSTItemNumber = request->POSTItemNumber;
                     rqst->POSTItem       = request->POSTItem;       //<- NEVER free this here since it is stack allocated..

                     rqst->GETItemNumber  = request->GETItemNumber;
                     rqst->GETItem        = request->GETItem;        //<- NEVER free this here since it is stack allocated..

                     rqst->COOKIEItemNumber = request->COOKIEItemNumber;
                     rqst->COOKIEItem       = request->COOKIEItem;   //<- NEVER free this here since it is stack allocated..

                     rqst->sizeOfExtraDataThatWillNeedToBeDeallocated = request->sizeOfExtraDataThatWillNeedToBeDeallocated;
                     rqst->extraDataThatWillNeedToBeDeallocated       = request->extraDataThatWillNeedToBeDeallocated;

                     rqst->content=cacheMemory;

                     if (rqst->useSessionLifecycle)
                      {
                        rqst->sessionID = getSessionFromHeader(
                                                               instance->sessionList,
                                                               0,//const char * connectionIP ,
                                                               0,//const char * cookieValue ,
                                                               0//const char * BrowserIdentifier
                                                               );
                      }

                     ///--------------------------------------------
                     printRequestData(rqst);


                     void ( *DoCallback) ( struct AmmServer_DynamicRequest * )=0 ;
                     DoCallback = shared_context->dynamicRequestCallbackFunction;

                     shared_context->executedNow=1;
                     struct time_snap callbackTimer;
                     startTimer (&callbackTimer);
                     ///--------------------------------------------
                                   DoCallback(rqst);
                     ///--------------------------------------------
                     unsigned long elapsedCallbackTimeMS=endTimer (&callbackTimer);
                     fprintf(stderr,"Callback done in %lu microseconds \n",elapsedCallbackTimeMS);
                     shared_context->executedNow=0;

                     if (rqst->extraDataThatWillNeedToBeDeallocated!=0)
                     {
                       safeFree(rqst->extraDataThatWillNeedToBeDeallocated,rqst->sizeOfExtraDataThatWillNeedToBeDeallocated);
                       rqst->sizeOfExtraDataThatWillNeedToBeDeallocated=0;
                       rqst->extraDataThatWillNeedToBeDeallocated=0;
                     }

                     if (rqst->contentContainsPathToFileToBeStreamed)
                     {
                        *contentContainsPathToFileToBeStreamed=1;
                        snprintf(verified_filename,verified_filenameLength,"%s",rqst->content);
                     }
                     //Keep the new content size so if the next call has a callback_every_x_msec attribute we know how much data to serve
                     shared_context->requestContext.contentSize = rqst->contentSize;
                     *memSize = rqst->contentSize;
                     fprintf(stderr,"After callback we got back %lu bytes @ pointer %p \n",rqst->contentSize,rqst->content);
                     safeFree(rqst,sizeof(struct AmmServer_DynamicRequest));

                     //This means we can call the callback to prepare the memory content..! END
                     //CreateCompressedVersionofDynamicContent(instance,index);
                    } else
                    {
                      errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY);
                    }
   }
 return cacheMemory;
}
