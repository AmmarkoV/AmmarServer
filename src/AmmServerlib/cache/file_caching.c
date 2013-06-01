#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../AmmServerlib.h"
#include "../server_configuration.h"
#include "file_caching.h"
#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../tools/time_provider.h"

#include "../hashmap/hashmap.h"

#define USE_HASHMAP_IN_CACHE 0

/*
unsigned long instance->instance->loaded_cache_items_Kbytes=0;
unsigned int instance->loaded_cache_items=0;
struct cache_item * cache=0;*/
//The cache consists of cache items.. Each cache item Gets Added and Removed with calls defined beneath..
//The cache must be Created and then Destroyed because on program execution its pointer has no memory allocated for the content..!

int WeCanCommitMoreMemoryForCaching(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes)
{
  if (MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB*1024*1024<additional_mem_to_malloc_in_bytes) { fprintf(stderr,"This file exceedes the maximum cache size for individual files , it will not be cached\n");  return 0;  }
  if (MAX_CACHE_SIZE_IN_MB*1024<instance->loaded_cache_items_Kbytes+additional_mem_to_malloc_in_bytes/1024)  { fprintf(stderr,"We have reached the soft cache limit of %u MB\n",MAX_CACHE_SIZE_IN_MB);  return 0; }
  return 1;
}


int AddNewMallocOpToCacheCounter(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes)
{
  instance->loaded_cache_items_Kbytes+=(additional_mem_to_malloc_in_bytes/1024);
  return 1;
}

int AddFreeOpToCacheCounter(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes)
{
  instance->loaded_cache_items_Kbytes-=(additional_mem_to_malloc_in_bytes/1024);
  return 1;
}



/*
     INDEXING for the cache
     The following calls manage Indexing for the cache..
     We can create a cache item in many ways ( an empty one , a real one , a virtual one)
     In order to reduce code repetitions all the create/delete/find indexing routines ( that are the same among all the above ways to create a cache itm )
     are implemented here
     ------------------------------------------------------------------
*/



int cache_ChangeRequestIfTemplateRequested(struct AmmServer_Instance * instance,char * request,char * templates_root)
{
  if (!ENABLE_INTERNAL_RESOURCES_RESOLVE)  { return 0; }
  //The role of request caching is to intercept incoming requests and if they are referring
  //to an internal resource using the TemplatesInternalURI URI we want to redirect the request
  //to our templates folder ..!
  //If the request was indeed a change request returns 1 else 0
  if ( strlen(request)>strlen(TemplatesInternalURI)+64 )
   {
       fprintf(stderr,"\nWARNING : Skipping cache_ChangeRequestIfTemplateRequested due to a very large request\n");
       return 0;
   }

  char tmp_cmp[MAX_FILE_PATH]={0};
  char * res=strstr(request,TemplatesInternalURI);
  char * res_skipped=res;
  unsigned int template_size = strlen(TemplatesInternalURI);

  if ( res!=0 )
   {
      res_skipped=res+template_size;
      fprintf(stderr,"We've got a result , %s ( skipped %s )\n",res,res_skipped);
      if (strlen(res_skipped)+strlen(templates_root)<MAX_FILE_PATH)
       {
         strcpy(tmp_cmp,templates_root);
         strcat(tmp_cmp,res_skipped);
       } else
       {
           fprintf(stderr,"Internal request too long , not thoroughly tested\n");
           return 0;
       }
      fprintf(stderr,"Internal request to string %s -> %s \n",request,tmp_cmp);
      strcpy(request,tmp_cmp);
      return 1;
   }
  return 0;
}


int freeMallocIfNeeded(char * mem,unsigned char free_is_needed)
{
    if ( (free_is_needed)&&(mem!=0) ) { free(mem); return 1; }
    return 0;
}



/*
 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------



 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------
*/

/*This is the Search Index Function , It is basically fully inefficient O(n) , it will be replaced by some binary search implementation*/
unsigned int cache_FindResource(struct AmmServer_Instance * instance,char * resource,unsigned int * index)
{
  fprintf(stderr,"Serial slow searching for resource in cache %s ..\n",resource);

  struct cache_item * cache = (struct cache_item *) instance->cache;

  if (cache==0) { warning("Cache hasn't been allocated yet\n"); return 0; }

  unsigned long file_we_are_looking_for = hashFunction(resource);
  unsigned long i=0;

  #if USE_HASHMAP_IN_CACHE
  if ( hashMap_GetULongPayload((struct hashMap *) instance->cacheHashMap,resource,&i) )
    {
      warning("HashMap Found");
      fprintf(stderr,"HashMap Found %lu \n",i);
      *index=i;
      return 1;
    }else
    {
      return 0;
    }
  #else
  for (i=0; i<instance->loaded_cache_items; i++)
   {
     if ( cache[i].filename_hash == file_we_are_looking_for )
      {
        ++cache[i].hits;
        if (cache[i].filesize!=0)
         { //Filesize gets pulled directly from the client.. For this reason we want to check if the client has allocated a value to prevent the possible segfault
          fprintf(stderr,"..found it here %lu , %u hits , %0.2f Kbytes\n",i,cache[i].hits,(double) (*cache[i].filesize/1024) );
         }
        *index=i;
        return 1;
      }
   }
  #endif

 return 0;
}

/*This is the Create Index Function , Nothing is sorted so we just append our cache item list with another one..
  Also for now only the hash is used to retrieve it ( talk about a sloppy implementation ) */
int cache_CreateResource(struct AmmServer_Instance * instance,char * resource,unsigned int * index)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  if (cache==0) { fprintf(stderr,"Cache hasn't been allocated yet\n"); return 0; }


  if (MAX_CACHE_SIZE_IN_MB<=instance->loaded_cache_items+1) { fprintf(stderr,"Cache is full , Could not Create_CacheItem(%s)",resource); return 0; }
  *index=instance->loaded_cache_items++;

  #if USE_HASHMAP_IN_CACHE
   if ( hashMap_AddULong((struct hashMap *) instance->cacheHashMap ,resource,(unsigned long) *index) )
   {
    warning("hashMap_AddULong adding New Resource to HashMap");
    fprintf(stderr,"hashMap_AddULong adding %s \n",resource);
   }
  #else
    cache[*index].filename_hash = hashFunction(resource);
  #endif



  return 1;
}

/*This is the Destroy Index Function , it isn't implemented , as one can see.. */
int cache_DestroyResource(unsigned int * index)
{
  fprintf(stderr,"cache_DestroyResource(%u) hasn't been implemented yet\n",*index);
  return 0;
}


/*
   ------------------------------------------------------------------
   ------------------------------------------------------------------
   ------------------------------------------------------------------
*/


int cache_LoadResourceFromDisk(struct AmmServer_Instance * instance,char *filename,unsigned int * index)
{
  /*Now we will do the following things
    1) Open the file and find how large it is
    2) Allocate a large enough chunk of memory
    3) Read it all in memory and close the file */

  struct cache_item * cache = (struct cache_item *) instance->cache;

  FILE * pFile = fopen (filename, "rb" );
  if (pFile==0) { fprintf(stderr,"Could not open file to cache it.. %s\n",filename); return 0;}
  /*We have opened the file.. */

  if ( fseek (pFile , 0 , SEEK_END)!=0 ) { fprintf(stderr,"Could not find file size to cache client..!\nUnable to serve client\n"); fclose(pFile); return 0; }
  unsigned long lSize = ftell (pFile);
  //lSize now holds the size of the file..

  //We check if the file size is ok with our configuration limits
  if (!WeCanCommitMoreMemoryForCaching(instance,lSize)) { fclose(pFile); return 0; }

  //We are ok with the file size , we will now rewind the file to start reading it from the beginning..!
  rewind (pFile);
  //We will allocate the required memory..!
  char * buffer = (char*) malloc ( sizeof(char) * (lSize));
  if (buffer == 0 ) { fprintf(stderr,"Could not allocate enough memory to cache this file..!\n"); fclose(pFile); return 0;  }
  //We have allocated the new memory chunk so we will update our loaded cache counter..!
  AddNewMallocOpToCacheCounter(instance,lSize);

  // copy the file into the buffer:
  size_t result;
  result = fread (buffer,1,lSize,pFile);
  if (result != lSize) { fprintf(stderr,"Reading error , while filling in newly allocated cache item %s \n",filename); free (buffer); fclose (pFile); return 0; }

  fprintf(stderr,"File %s has %u bytes cached with index %u \n",filename,(unsigned int ) lSize,*index);
  fclose(pFile);
  // file has been cached , so time to fill in its details..
  cache[*index].mem = buffer;
  if ( cache[*index].filesize!= 0 ) { free (cache[*index].filesize); cache[*index].filesize=0; }
  cache[*index].filesize = (unsigned long * ) malloc(sizeof (unsigned long));
  *cache[*index].filesize = lSize;

  /*This could be a good place to make the gzipped version of the buffer..!*/
   char content_type_str[128]={0};
   if ( GetContentType(filename,content_type_str) )
    {
        //This will fill content_type with a value from enum FileType ( declared at http_tools.h )
        //if the value is TEXT we are good to go :P
        cache[*index].content_type =  GetExtentionType(content_type_str);
    }

  cache[*index].compressed_mem_filesize=0;
  cache[*index].compressed_mem=0;
  if (!CreateCompressedVersionofStaticContent(instance,*index)) {  fprintf(stderr,"Could not create a gzipped version of the file..\n"); }

  return 1;
}


int cache_AddFile(struct AmmServer_Instance * instance,char * filename,unsigned int * index,struct stat * last_modification)
{
  if (!cache_CreateResource(instance,filename,index)) { /*We couldn't allocate a new index for this file */ return 0; }

  struct cache_item * cache = (struct cache_item *) instance->cache;

  fprintf(stderr,"Adding file %s to cache ( %0.2f / %u MB )\n",filename,(float) instance->loaded_cache_items_Kbytes/1048576 ,  MAX_CACHE_SIZE_IN_MB);

  //first to clear this cache item
  /* We clear everything using memset ! */
  memset((void*) &cache[*index],0,sizeof(struct cache_item));

  if (!cache_LoadResourceFromDisk(instance,filename,index))
   {
       fprintf(stderr,"Could not read file %s into memory\n",filename);
       fprintf(stderr,"Erasing index from memory\n");
       cache_DestroyResource(index);
       *index=0;
       return 0;
   }

   //Save modification time..! These are not used yet.. !
   if (last_modification!=0)
   {
    struct tm * ptm = gmtime ( &last_modification->st_mtime ); //This is not a particularly thread safe call , must add a mutex or something here..!
    cache[*index].hour=ptm->tm_hour;
    cache[*index].minute=ptm->tm_min;
    cache[*index].second=ptm->tm_sec;
    cache[*index].wday=ptm->tm_wday;
    cache[*index].day=ptm->tm_mday;
    cache[*index].month=ptm->tm_mon;
    cache[*index].year=EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year;
   }

  return 1;
}



int cache_AddMemoryBlock(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
{
  if ( ! DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED )
   {
     fprintf(stderr,"Dynamic content is disabled..!\n");
     return 0;
   }

  //Create the full path to distinguish from different root_paths ( virutal servers ) ..!
  char full_filename[(MAX_RESOURCE*2)+1]={0};
  //These direct resource functions come from inside our program so we can cpy/cat them here
  //( they dont pass through http_header_analysis.c so this can't be done another way..!

  struct cache_item * cache = (struct cache_item *) instance->cache;

  strncpy(full_filename,context->web_root_path,MAX_RESOURCE);
  strncat(full_filename,context->resource_name,MAX_RESOURCE);
  ReducePathSlashes_Inplace(full_filename);

  unsigned int index=0;
  if (! cache_CreateResource(instance,full_filename,&index) ) { return 0; }

  cache[index].context = context;
  cache[index].mem = context->requestContext.content;
  cache[index].filesize = &context->requestContext.content_size;
  cache[index].compressed_mem_filesize=0;
  cache[index].compressed_mem=0;

  cache[index].hits = 0;
  cache[index].prepare_mem_callback = context->prepare_content_callback;

  return 1;
}


int cache_AddDoNOTCacheRuleForResource(struct AmmServer_Instance * instance,char * filename)
{
   struct cache_item * cache = (struct cache_item *) instance->cache;
   unsigned int index=0;
   if (cache_FindResource(instance,filename,&index))  { cache[index].doNOTCache=1; }
    else
     {
       //File Doesn't exist, we have to create a cache index for it , and then mark it as uncachable..!
       if (!cache_CreateResource(instance,filename,&index) ) { return 0; }
       if (cache_FindResource(instance,filename,&index)) { cache[index].doNOTCache=1; } else
                                             { return 0; } //Could not set doNOTCache..!
     }
   return 1;
}




int cache_RemoveResource(struct AmmServer_Instance * instance,unsigned int index)
{
    struct cache_item * cache = (struct cache_item *) instance->cache;
    unsigned int i = index;
    if ( cache[i].prepare_mem_callback !=0)
     {
        //This cache item is "dynamic content" as a result
        //it has its own memory handler , so we just dereference it..
        cache[i].prepare_mem_callback=0;
        cache[i].mem=0;
        cache[i].filesize=0;
     }
       else
     {
      if ( cache[i].mem != 0 )
       {
          free(cache[i].mem);
          cache[i].mem=0;
          if ( cache[i].filesize != 0 ) { AddFreeOpToCacheCounter(instance,*cache[i].filesize); } //If we have a filesize we subtract it from the cache malloc counter
       }
      if ( cache[i].filesize != 0 )
       {
          free(cache[i].filesize);
          cache[i].filesize=0;
       }
     }

     //Compressed mem is not controlled by the callback so we can deallocate it at our convinience (not inside the if then else scope )!
      if ( cache[i].compressed_mem != 0 )
       {
          free(cache[i].compressed_mem);
          cache[i].compressed_mem=0;
          if ( cache[i].filesize != 0 ) { AddFreeOpToCacheCounter(instance,*cache[i].compressed_mem_filesize); } //If we have a filesize we subtract it from the cache malloc counter
       }
      if ( cache[i].compressed_mem_filesize != 0 )
       {
          free(cache[i].compressed_mem_filesize);
          cache[i].compressed_mem_filesize=0;
       }
   return 0;
}


int cache_RemoveContextAndResource(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem)
{
       context->requestContext.MAX_content_size=0;
       if ((free_mem)&&(context->requestContext.content!=0)) { free(context->requestContext.content); context->requestContext.content=0; }

       unsigned int index;
       if (!cache_FindResource(instance,context->resource_name,&index) )
          {
            warning("Could not remove direct resource ( it does not exist ) ..\n");
            return 0;
          }
       return cache_RemoveResource(instance,index);
}

unsigned int cache_GetHashOfResource(struct AmmServer_Instance * instance,unsigned int index)
{
    struct cache_item * cache = (struct cache_item *) instance->cache;
    return cache[index].filename_hash;
}



int cache_Initialize(struct AmmServer_Instance * instance,unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB)
{
   MAX_CACHE_SIZE_IN_MB=max_total_allocation_MB;
   MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB=max_allocation_per_entry_MB;
   MAX_SEPERATE_CACHE_ITEMS=max_seperate_items;

   unsigned int cache_memory_size = sizeof(struct cache_item) * (MAX_SEPERATE_CACHE_ITEMS+1);
  if (instance->cache==0)
   {
     instance->cache = (struct cache_item *) malloc(cache_memory_size);
     if (instance->cache == 0) { fprintf(stderr,"Unable to allocate initial cache memory\n"); return 0; }
     memset(instance->cache , 0 , cache_memory_size);
   }


   #if USE_HASHMAP_IN_CACHE
    instance->cacheHashMap = (void*) hashMap_Create(max_seperate_items,1000,0);
   #endif // USE_HASHMAP_IN_CACHE

   return 1;
}

int cache_Destroy(struct AmmServer_Instance * instance)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  fprintf(stderr,"Destroying cache..\n");

//   if (!DestroyVariableCache()) { fprintf(stderr,"Failed destroying Variable Cache\n"); }

   if (cache==0)
    {
       fprintf(stderr,"Cache already destroyed \n");
       instance->loaded_cache_items=0;
       instance->loaded_cache_items_Kbytes=0;
      return 1;
    }

   unsigned int i=0;
   for (i=0; i<instance->loaded_cache_items; i++)
   {
      cache_RemoveResource(instance,i);
   }

   free(cache);
   cache = 0;
   instance->loaded_cache_items=0;
   instance->loaded_cache_items_Kbytes=0;


   #if USE_HASHMAP_IN_CACHE
    hashMap_Destroy((struct hashMap *) instance->cacheHashMap);
    instance->cacheHashMap=0;
   #endif // USE_HASHMAP_IN_CACHE

   return 1;
}


/*

  ----------------------------------------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------------------------------------

     This ( cache_GetResource ) is the most heavily used and complex call in this file , it is what the file server
     calls when a new file is requested.. and it both serves ready cache items but also creates them if we need them..
                 It also has to deal with calling the callback function for dynamic content

  ----------------------------------------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------------------------------------
*/

int cache_ResourceExists(struct AmmServer_Instance * instance,char * verified_filename,unsigned int * index)
{
    if (cache_FindResource(instance,verified_filename,index)) { return 1; }
    return 0;
}


char * cache_GetResource(struct AmmServer_Instance * instance,struct HTTPRequest * request,char * verified_filename,unsigned int * index,unsigned long *filesize,struct stat * last_modification,unsigned char * compression_supported,unsigned char * free_after_use)
{
     //By default we dont want to free the memory allocation after use..
      *free_after_use=0;

      if (instance==0) { fprintf(stderr,"Instance is not allocated..\n"); return 0;  }

      struct cache_item * cache = (struct cache_item *) instance->cache;
      if (cache==0) { fprintf(stderr,"Cache is not allocated..\n"); return 0;  }

      if (!CACHING_ENABLED)
      {
        fprintf(stderr,"Caching deactivated..!\n");
        *compression_supported=0;
        return 0;
      }


       if (cache_FindResource(instance,verified_filename,index)) //This can be avoided by adding an index as a parameter to this function call
        {
           //Initially we would like to work with the memory block allocated when the dynamic call
           //was first registered..
           char * cache_memory = cache[*index].mem;

           //if doNOTCache is set and this is a real file..
           fprintf(stderr,"index = %u\n",*index);
           fprintf(stderr,"doNotCache = %u \n",cache[*index].doNOTCache);
           fprintf(stderr,"mem_callback = %p \n",cache[*index].prepare_mem_callback);

           if ((cache[*index].doNOTCache)&&(cache[*index].prepare_mem_callback==0))
            {
              fprintf(stderr,"We do not want to serve a cached version of this file..\n");
              *compression_supported=0;
              return 0;
            }  else
         {
           /*We want to serve a cached version of the file START*/

             /*Before returning any pointers we will have to ask ourselves.. Is this a Dynamic Content Cache instance ?
             if cache[*index].prepare_mem_callback is set then it means we will have to call it first to load data in cache[*index].mem  */
             if (cache[*index].prepare_mem_callback!=0)
              {
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



                   /*Do callback here*/
                   shared_context->callback_cooldown=0;
                   shared_context->last_callback = now;
                   void ( *DoCallback) ( struct AmmServer_DynamicRequestContext * )=0 ;
                   DoCallback = cache[*index].prepare_mem_callback;


                   /*If we have GET or POST request variables , lets pass them through to our shared context.. */
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
                     DoCallback(rqst);
                     free(rqst);
                    }


                  //This means we can call the callback to prepare the memory content..! END
                   CreateCompressedVersionofDynamicContent(instance,*index);
                }
              }


           /*We want to serve a cached version of the file START*/
           if ( (cache[*index].compressed_mem!=0) && (*compression_supported!=0) && (ENABLE_COMPRESSION) )
           {
             *compression_supported=1; // The response is compressed ( already set but in the future it may need to distinguish differnet compression schemes!..!

              /* We can and will serve back a cached version of the page..! */
             *filesize=*cache[*index].compressed_mem_filesize;
             fprintf(stderr,"Cache Serving back a compressed buffer sized %lu bytes\n",*filesize);

             return cache[*index].compressed_mem;
           }
               else
           if (cache_memory!=0)
           {
             *compression_supported=0; // The response is not compressed..!

             *filesize=*cache[*index].filesize;
             fprintf(stderr,"Cache Serving back a buffer sized %lu bytes\n",*filesize);

             return cache_memory;
           }

          /*We want to serve a cached version of the file END*/
         }
        } else
        {
           /* A cached copy doesn't seem to exist , lets make one and then claim it exists! */
           if ( cache_AddFile(instance,verified_filename,index,last_modification) )
            {
              *compression_supported=0;
              *filesize=*cache[*index].filesize; //We return the filesize after the operation..
              return cache[*index].mem; //because cache_memory hasn't been declared here..
            }
        }
       //If we are here we are unlocky , our file wasn't in cache and to make things worse we also failed to load it so
       //regular file sending it as it is ..!

       *compression_supported=0;
       *filesize=0;
       fprintf(stderr,"Cache could not find file %s , filesize %u , compression support %u \n",verified_filename,(unsigned int) *filesize,*compression_supported);

       return 0;
}

