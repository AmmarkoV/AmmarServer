#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_requests.h"
#include "file_caching.h"
#include "../server_configuration.h"
#include "../tools/logs.h"
#include "../tools/time_provider.h"

int  dynamicRequest_ContentAvailiable(struct AmmServer_Instance * instance,unsigned int index)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  return (cache[index].dynamicRequestCallbackFunction!=0);
}

char * dynamicRequest_serveContent
          (
            struct AmmServer_Instance * instance,
            struct HTTPHeader * request,
            struct AmmServer_RH_Context * shared_context ,
            unsigned int index,
            unsigned long * memSize,
            unsigned char * compressionSupported,
            unsigned char * freeContentAfterUsingIt
          )
{
 // error("Dynamic requests are disabled until further notice .. \n");
  //return 0;
 // struct cache_item * cache = (struct cache_item *) instance->cache;


  //Before returning any pointers we will have to ask ourselves.. Is this a Dynamic Content Cache instance ?


  if (shared_context->dynamicRequestCallbackFunction==0)
  {
    error("dynamicRequest_serveContent : No Callback registered , cannot serve content without a function to call \n");
    error("dynamicRequest_serveContent should never get called without a function to call \n");
    fprintf(stderr,"Index item %u\n",index);
    fprintf(stderr,"content Resource name %s\n",shared_context->resource_name);

      //CRAZY DEBUGGING -------------
       struct cache_item * cache = (struct cache_item *) instance->cache;
       struct AmmServer_RH_Context * shc = cache[index].dynamicRequest;
       fprintf(stderr,"content pointer %p\n",shc->dynamicRequestCallbackFunction);
       fprintf(stderr,"content Resource name %s\n",shc->resource_name);

      //-----------------------------

    return 0;
  }

  char * cacheMemory=0; // <- this will hold the resulting page


  //Before doing callback we might want to allocate a different response space dedicated to this callback instead to using
  //one common memory buffer for every client...!
  if ( (shared_context->RH_Scenario == DIFFERENT_PAGE_FOR_EACH_CLIENT) )
    {
     unsigned int size_to_allocate =  sizeof(char) * ( shared_context->requestContext.MAXcontentSize ) ;
     if (size_to_allocate==0)
     { warning("BUG : We should allocate additional space for this request.. Unfortunately it appears to be zero.. \n "); }
     else
     {
      fprintf(stderr,"Allocating an additional %u bytes for this request \n",size_to_allocate);
      cacheMemory = (char *) malloc( size_to_allocate );
      if (cacheMemory!=0) { *freeContentAfterUsingIt=1; } else //Allocation was successfull , we would like parent procedure to free it after use..
                          { error("Could not allocate enough memory for responding to this request"); } //Lets work with our default buffer till the end..!
     }
    } else
   if ( (shared_context->RH_Scenario == SAME_PAGE_FOR_ALL_CLIENTS ) )
    {
       cacheMemory =  shared_context->requestContext.content;
    }

  //In case mem doesnt point to a proper buffer calling the mem_callback function will probably segfault for all we know
  //So we bail out and emmit an error message..!
  if ( (cacheMemory==0) || (shared_context->requestContext.MAXcontentSize==0) )
    {
     warning("Not going to call callback function with an empty buffer..!\n");
    } else
    {
     //This means we can call the callback to prepare the memory content..! START

     unsigned long now=0; //If there is no callback limits the time of the call will always be 0
     //That doesnt bother anything or anyone..



     //Check if request falls on callback limits!
     if (
          (shared_context-> callback_every_x_msec!=0) &&
          (shared_context->RH_Scenario == SAME_PAGE_FOR_ALL_CLIENTS)
        )
     { //Dynamic pages without time limits dont have to call the "expensive" GetTickCountAmmServ
       now=GetTickCountAmmServ();
       if ( now-shared_context->last_callback < shared_context-> callback_every_x_msec )
        {
         //The request came too fast.. We will serve our existing file..!
         *compressionSupported=0;
         shared_context->callback_cooldown=1;
         *memSize=shared_context->requestContext.contentSize;
         return cacheMemory;
        } else
        {
         fprintf(stderr,"Request deserves fresh page , %u last gen, %lu now , %u cooldown\n",shared_context->last_callback,now,shared_context-> callback_every_x_msec);
        }
      }

        fprintf(stderr,"Internal mem_callback = %p \n",shared_context->dynamicRequestCallbackFunction);
       //Do callback here
       shared_context->callback_cooldown=0;
       shared_context->last_callback = now;
       void ( *DoCallback) ( struct AmmServer_DynamicRequest * )=0 ;
       DoCallback = shared_context->dynamicRequestCallbackFunction;

       fprintf(stderr,"Trying to adjust GET_request length \n");

       //If we have GET or POST request variables , lets pass them through to our shared context..
       shared_context->requestContext.GET_request = request->GETquery;
       if (shared_context->requestContext.GET_request!=0) { shared_context->requestContext.GET_request_length = strlen(shared_context->requestContext.GET_request); } else
                                                          { shared_context->requestContext.GET_request_length = 0; }

       shared_context->requestContext.POST_request = request->POSTrequest;
       shared_context->requestContext.POST_request_length = request->POSTrequestSize;

       fprintf(stderr,"Survived adjusting GET_request length \n");

        struct AmmServer_DynamicRequest * rqst = (struct AmmServer_DynamicRequest * ) malloc(sizeof(struct AmmServer_DynamicRequest));
        if (rqst!=0)
                    {
                     memcpy(rqst , &shared_context->requestContext , sizeof( struct AmmServer_DynamicRequest ));

                     fprintf(stderr,"Request for a maximum of %u characters ( %lu ) \n",rqst->MAXcontentSize , shared_context->requestContext.MAXcontentSize );
                     fprintf(stderr,"POST : %p , %u bytes\n",rqst->POST_request , rqst->POST_request_length );
                     fprintf(stderr,"GET : %p , %u bytes\n",rqst->GET_request , rqst->GET_request_length );

                     rqst->content=cacheMemory;
                     //They are an id ov the var_caching.c list so that the callback function can produce information based on them..!
                     DoCallback(rqst);
                     *memSize = rqst->contentSize;
                     fprintf(stderr,"After callback we got back %lu bytes\n",rqst->contentSize);
                     free(rqst);

                     //This means we can call the callback to prepare the memory content..! END
                     //CreateCompressedVersionofDynamicContent(instance,index);
                    } else
                    {
                      error("Could not allocate enough memory to make the request \n");
                    }

   }


 return cacheMemory;

}







int callClientRequestHandler(struct AmmServer_Instance * instance,struct HTTPHeader * output)
{
  if ( instance->clientRequestHandlerOverrideContext == 0 )  { return 0; }
  struct AmmServer_RequestOverride_Context * clientOverride = instance->clientRequestHandlerOverrideContext;
  if ( clientOverride->request_override_callback == 0 ) { return 0; }

  clientOverride->request = output;


  void ( *DoCallback) ( struct AmmServer_RequestOverride_Context * ) = 0 ;
  DoCallback = clientOverride->request_override_callback;

  DoCallback(clientOverride);

  //After getting back the override and whatnot , keep the client from using a potentially bad
  //memory chunk
  clientOverride->request = 0;

  return 1;
}



int saveDynamicRequest(char * filename , struct AmmServer_Instance * instance , struct AmmServer_DynamicRequest * rqst)
{
  FILE *fp=0;

  fp = fopen(filename,"w");
  if (fp!=0)
  {
    fprintf(fp,"%s",rqst->GET_request);
    fclose(fp);
    return 1;
  }
  return 0;
}


