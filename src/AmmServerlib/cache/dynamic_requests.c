#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_requests.h"
#include "file_caching.h"

int  dynamicRequest_ContentAvailiable(struct AmmServer_Instance * instance,unsigned int index)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  return (cache[index].prepare_mem_callback!=0);
}

char * dynamicRequest_serveContent
           (struct AmmServer_Instance * instance,
            struct HTTPRequest * request,
            struct AmmServer_RH_Context * shared_context
          )
{
  struct cache_item * cache = (struct cache_item *) instance->cache;


  //Before returning any pointers we will have to ask ourselves.. Is this a Dynamic Content Cache instance ?


  if (shared_context->prepare_content_callback==0)
  {
    fprintf(stderr,"No dynamic request content registered\n");
    return 0;
  }

  //if cache[*index].prepare_mem_callback is set then it means we will have to call it first to load data in cache[*index].mem

/*
  //Before doing callback we might want to allocate a different response space dedicated to this callback instead to using
  //one common memory buffer for every client...!
  if ( (shared_context->RH_Scenario == DIFFERENT_PAGE_FOR_EACH_CLIENT) && (ENABLE_SEPERATE_MALLOC_FOR_CHANGING_DYNAMIC_PAGES) )
                      {
                        unsigned int size_to_allocate =  sizeof(char) * ( shared_context->requestContext.MAX_content_size ) ;

                        if (size_to_allocate==0)
                         {
                          fprintf(stderr,"BUG : We should allocate additional space for this request.. Unfortunately it appears to be zero.. \n ");
                         }
                         else
                         {
                          fprintf(stderr,"Allocating an additional %u bytes for this request \n",size_to_allocate);
                          cache_memory = (char *) malloc( size_to_allocate );
                          if (cache_memory!=0) { *free_after_use=1; } else //Allocation was successfull , we would like parent procedure to free it after use..
                                               { cache_memory=cache[*index].mem; } //Lets work with our default buffer till the end..!
                         }
                       }

  //In case mem doesnt point to a proper buffer calling the mem_callback function will probably segfault for all we know
  //So we bail out and emmit an error message..!
  if ( (cache_memory==0) || (cache[*index].filesize==0) )
                {
                  fprintf(stderr,"Not going to call callback function with an empty buffer..!\n");
                } else
                {
                  //This means we can call the callback to prepare the memory content..! START
                  struct AmmServer_RH_Context * shared_context = cache[*index].context;

                  unsigned long now=0; //If there is no callback limits the time of the call will always be 0
                  //That doesnt bother anything or anyone..

                  if (shared_context-> callback_every_x_msec!=0)
                   { //Dynamic pages without time limits dont have to call the "expensive" GetTickCountAmmServ
                     now=GetTickCountAmmServ();
                     if ( now-shared_context->last_callback < shared_context-> callback_every_x_msec )
                          {
                            //The request came too fast.. We will serve our existing file..!
                            *compression_supported=0;
                            shared_context->callback_cooldown=1;
                            *filesize=*cache[*index].filesize;
                            return cache_memory;
                          } else
                          {
                           fprintf(stderr,"Request deserves fresh page , %u last gen, %lu now , %u cooldown\n",shared_context->last_callback,now,shared_context-> callback_every_x_msec);
                          }
                   }






                   //Do callback here
                   shared_context->callback_cooldown=0;
                   shared_context->last_callback = now;
                   void ( *DoCallback) ( struct AmmServer_DynamicRequestContext * )=0 ;
                   DoCallback = cache[*index].prepare_mem_callback;


                   //If we have GET or POST request variables , lets pass them through to our shared context..
                   shared_context->requestContext.GET_request = request->GETquery;
                   if (shared_context->requestContext.GET_request!=0) { shared_context->requestContext.GET_request_length = strlen(shared_context->requestContext.GET_request); } else
                                                        { shared_context->requestContext.GET_request_length = 0; }

                   shared_context->requestContext.POST_request = request->POSTquery;
                   if (shared_context->requestContext.POST_request!=0) { shared_context->requestContext.POST_request_length = strlen(shared_context->requestContext.POST_request); } else
                                                         { shared_context->requestContext.POST_request_length = 0; }


                   struct AmmServer_DynamicRequestContext * rqst = (struct AmmServer_DynamicRequestContext * ) malloc(sizeof(struct AmmServer_DynamicRequestContext));
                   if (rqst!=0)
                    {
                     memcpy(rqst->content , &shared_context->requestContext , sizeof( struct AmmServer_DynamicRequestContext ));
                     rqst->content=cache_memory;
                     //They are an id ov the var_caching.c list so that the callback function can produce information based on them..!
                     warning("Callbacks , are currently broken Doing Callback\n");
                     DoCallback(rqst);
                     fprintf(stderr,"After callback we got back %u bytes\n",rqst->content_size);
                     free(rqst);
                    }


                  //This means we can call the callback to prepare the memory content..! END
                   CreateCompressedVersionofDynamicContent(instance,*index);
                }
              }
*/
 return 0;

}
