#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../AmmServerlib.h"
#include "../server_configuration.h"
#include "file_caching.h"
#include "file_compression.h"
#include "../tools/logs.h"
#include "../tools/http_tools.h"
#include "../tools/time_provider.h"

#include "../../Hashmap/hashmap.h"
#include "dynamic_requests.h"


//The cache consists of cache items.. Each cache item Gets Added and Removed with calls defined beneath..
//The cache must be Created and then Destroyed because on program execution its pointer has no memory allocated for the content..!
int cache_CountMemoryUsageFreeOperation(struct AmmServer_Instance * instance , unsigned long freedSize)
{
  if (instance==0) { return 0; }

  unsigned long freedSizeKB = (unsigned long) freedSize/1024;
  if (instance->loaded_cache_items_Kbytes > freedSizeKB ) { instance->loaded_cache_items_Kbytes -= freedSizeKB; } else
                                                          { instance->loaded_cache_items_Kbytes = 0; }
  return 1;
}

int cache_CountMemoryUsageAllocateOperation(struct AmmServer_Instance * instance , unsigned long allocatedSize)
{
  if (instance==0) { return 0; }
  unsigned long allocatedSizeKB = (unsigned long) allocatedSize/1024;
   instance->loaded_cache_items_Kbytes += allocatedSizeKB;
  return 1;
}

unsigned long cache_GetCacheSizeKB(struct AmmServer_Instance * instance)
{
  return instance->loaded_cache_items_Kbytes;
}

/*
     INDEXING for the cache
     The following calls manage Indexing for the cache..
     We can create a cache item in many ways ( an empty one , a real one , a virtual one)
     In order to reduce code repetitions all the create/delete/find indexing routines ( that are the same among all the above ways to create a cache itm )
     are implemented here
     ------------------------------------------------------------------
*/

int cache_ChangeRequestIfTemplateRequested(struct AmmServer_Instance * instance,char * request,unsigned int maxRequest/*,char * templates_root*/)
{
  if (instance==0) { return 0; }
  if (!ENABLE_INTERNAL_RESOURCES_RESOLVE)  { return 0; }
  //The role of request caching is to intercept incoming requests and if they are referring
  //to an internal resource using the TemplatesInternalURI URI we want to redirect the request
  //to our templates folder ..!
  //If the request was indeed a change request returns 1 else 0
  if ( strlen(request)>strlen(TemplatesInternalURI)+64 )
   {
       fprintf(stderr,"\nWARNING : Skipping cache_ChangeRequestIfTemplateRequested due to a very large request ( maxRequest = %u ) \n",maxRequest);
       return 0;
   }

  char * res=strstr(request,TemplatesInternalURI);

  if ( res!=0 )
   {
     /*
      char * res_skipped=res;
      unsigned int template_size = strlen(TemplatesInternalURI);
      char tmp_cmp[MAX_FILE_PATH]={0};
      res_skipped=res+template_size;
      fprintf(stderr,"We've got a result , %s ( skipped %s )\n",res,res_skipped);
      if (strlen(res_skipped)+strlen(templates_root)<MAX_FILE_PATH)
       {
         snprintf(tmp_cmp,MAX_FILE_PATH,"%s%s",templates_root,res_skipped);
         //strcpy(tmp_cmp,templates_root);
         //strcat(tmp_cmp,res_skipped);
       } else
       {
           fprintf(stderr,"Internal request too long\n");
           return 0;
       }
      fprintf(stderr,"Internal request to string %s -> %s \n",request,tmp_cmp);
      strncpy(request,tmp_cmp,maxRequest);
      */
      return 1;
   }
  return 0;
}

int freeMallocIfNeeded(char * mem,unsigned char free_is_needed)
{
    if ( (free_is_needed)&&(mem!=0) ) { free(mem); return 1; }
    return 0;
}

int cache_RandomizeETAG(struct AmmServer_Instance * instance)
{
  unsigned int oldRandomValue = instance->cacheVersionETag;
  srand(time(NULL));
  instance->cacheVersionETag = rand() % 10000;
  fprintf(stderr,"Randomizing cache ETag , ETags will from now on be %uXXXXXXXX \n",instance->cacheVersionETag);

  if (oldRandomValue == instance->cacheVersionETag)
     {
        warningID(ASV_WARNING_RANDOMIZER_IS_NOT_RANDOM);
        instance->cacheVersionETag = ( instance->cacheVersionETag +1 )% 10000;
     }
  return 1;
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

    instance->cacheHashMap = (void*) hashMap_Create(max_seperate_items,1000,0,USE_SORTING_IN_HASH_MAPS);
    instance->cacheVersionETag = 0;

    #if RANDOMIZE_ETAG_PER_LAUNCH
      cache_RandomizeETAG(instance);
    #endif // RANDOMIZE_ETAG_PER_LAUNCH

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


    hashMap_Destroy((struct hashMap *) instance->cacheHashMap);
    instance->cacheHashMap=0;

   return 1;
}

/*
 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------



 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------
*/

/*This is the Search Index Function , It is basically fully inefficient O(n) , it will be replaced by some binary search implementation*/
unsigned int cache_FindResource(struct AmmServer_Instance * instance,const char * resource,unsigned int * index)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  if (cache==0) { errorID(ASV_ERROR_CACHE_NOT_ALLOCATED); return 0; }

  unsigned long i=*index;

  if ( hashMap_GetULongPayload((struct hashMap *) instance->cacheHashMap,resource,&i) )
    {
      //fprintf(stderr,"HashMap Found %lu (%s=%s) \n",i,resource,hashMap_GetKeyAtIndex((struct hashMap *) instance->cacheHashMap,i));
      *index=i;
      return 1;
    }else
    {
      return 0;
    }
 return 0;
}

/*This is the Create Index Function , Nothing is sorted so we just append our cache item list with another one..
  Also for now only the hash is used to retrieve it ( talk about a sloppy implementation ) */
int cache_CreateResource(struct AmmServer_Instance * instance,const char * resource,unsigned int * index)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  if (cache==0) { errorID(ASV_ERROR_CACHE_NOT_ALLOCATED); return 0; }

  if ((unsigned int) MAX_CACHE_SIZE_IN_MB<=instance->loaded_cache_items+1) { fprintf(stderr,"Cache is full , Could not Create_CacheItem(%s)",resource); return 0; }
  *index=instance->loaded_cache_items++;

   if ( hashMap_AddULong((struct hashMap *) instance->cacheHashMap ,resource,(unsigned long) *index) )
   {
      AmmServer_Success("Added resource %s | %u items loaded",resource,instance->loaded_cache_items);
   }

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

int cache_LoadResourceFromDisk(struct AmmServer_Instance * instance,const char *filename,unsigned int * index)
{
  /*Now we will do the following things
    1) Open the file and find how large it is
    2) Allocate a large enough chunk of memory
    3) Read it all in memory and close the file */

  struct cache_item * cache = (struct cache_item *) instance->cache;

  FILE * pFile = fopen (filename, "rb" );
  if (pFile==0) { fprintf(stderr,"Could not open file to cache it.. %s\n",filename); return 0;}
  /*We have opened the file.. */

   unsigned long lSize = 0;
   struct stat statBuf;
   if (stat(filename,&statBuf)==0)
   {
      //Successfully stated file
      fprintf(stderr,"File size:                %lld bytes\n", (long long) statBuf.st_size);

      lSize = (unsigned long) statBuf.st_size;
      fprintf(stderr,"Last status change:       %s", ctime(&statBuf.st_ctime));
      fprintf(stderr,"Last file access:         %s", ctime(&statBuf.st_atime));
      fprintf(stderr,"Last file modification:   %s", ctime(&statBuf.st_mtime));
      #warning " Todo : populate cache[*index].modification with int fstat(pFile, struct stat *buf);"

     //We check if the file size is ok with our configuration limits
     if (!instance_WeCanCommitMoreMemory(instance,lSize)) { fclose(pFile); return 0; }
   } else
   {
     perror("stat returned error : ");

     AmmServer_Warning("Could not stat file %s , trying for alternative way to find filesize",filename);
     if ( fseek (pFile , 0 , SEEK_END)!=0 ) { fprintf(stderr,"Could not find file size to cache client..!\nUnable to serve client\n"); fclose(pFile); return 0; }
     lSize = ftell (pFile); //lSize now holds the size of the file..

     //We check if the file size is ok with our configuration limits
     if (!instance_WeCanCommitMoreMemory(instance,lSize)) { fclose(pFile); return 0; }

     //We are ok with the file size , we will now rewind the file to start reading it from the beginning..!
     rewind (pFile);
   }

  //We will allocate the required memory..!
  char * buffer = (char*) malloc ( sizeof(char) * (lSize+1));
  if (buffer == 0 ) { fprintf(stderr,"Could not allocate enough memory to cache this file..!\n"); fclose(pFile); return 0;  }
  //We have allocated the new memory chunk so we will update our loaded cache counter..!
  buffer[lSize]=0;
  instance_CountNewMallocOP(instance,lSize+1);

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize) { fprintf(stderr,"Reading error , while filling in newly allocated cache item %s \n",filename); free (buffer); fclose (pFile); return 0; }

  fprintf(stderr,"File %s has %u bytes cached with index %u \n",filename,(unsigned int ) lSize,*index);

  fclose(pFile);
  // file has been cached , so time to fill in its details..
  cache[*index].content = buffer;
  if ( cache[*index].contentSize!= 0 ) { free (cache[*index].contentSize); cache[*index].contentSize=0; }
  cache[*index].contentSize = (unsigned long * ) malloc(sizeof (unsigned long));
  *cache[*index].contentSize = lSize;



  /*This could be a good place to make the gzipped version of the buffer..!*/
   char content_type_str[128]={0};
   if ( GetContentType(filename,content_type_str,128) )
    {
        //This will fill content_type with a value from enum FileType ( declared at http_tools.h )
        //if the value is TEXT we are good to go :P
        cache[*index].contentTypeID =  GetExtentionType(content_type_str);
    }

  cache[*index].compressedContentSize=0;
  cache[*index].compressedContent=0;
  if (!CreateCompressedVersionofStaticContent(instance,*index))
     {  fprintf(stderr,"Did not create a gzipped version of the file..\n"); }

  return 1;
}

int cache_AddFile(struct AmmServer_Instance * instance,const char * filename,unsigned int * index,struct stat * last_modification)
{
  if (!cache_CreateResource(instance,filename,index)) { /*We couldn't allocate a new index for this file */ return 0; }

  struct cache_item * cache = (struct cache_item *) instance->cache;

  fprintf(stderr,"Adding file %s to cache ( %0.2f / %u MB )\n",filename,(float) instance->loaded_cache_items_Kbytes/1048576 ,(unsigned int)  MAX_CACHE_SIZE_IN_MB);

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
    cache[*index].modification.hour=ptm->tm_hour;
    cache[*index].modification.minute=ptm->tm_min;
    cache[*index].modification.second=ptm->tm_sec;
    cache[*index].modification.wday=ptm->tm_wday;
    cache[*index].modification.day=ptm->tm_mday;
    cache[*index].modification.month=ptm->tm_mon;
    cache[*index].modification.year=EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year;
   }

  return 1;
}

int cache_AddMemoryBlock(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
{

  //Create the full path to distinguish from different root_paths ( virutal servers ) ..!
  char full_filename[(MAX_RESOURCE*2)+1]={0};
  //These direct resource functions come from inside our program so we can cpy/cat them here
  //( they dont pass through http_header_analysis.c so this can't be done another way..!

  struct cache_item * cache = (struct cache_item *) instance->cache;

  strncpy(full_filename,context->web_root_path,MAX_RESOURCE);
  strncat(full_filename,context->resource_name,MAX_RESOURCE);
  ReducePathSlashes_Inplace(full_filename);

  unsigned int index=0;
  if (! cache_CreateResource(instance,full_filename,&index) ) { errorID(ASV_ERROR_CACHE_COULD_NOT_CREATE_RESOURCE); return 0; }

  cache[index].content = context->requestContext.content;
  cache[index].contentSize = &context->requestContext.contentSize;
  cache[index].compressedContent=0;
  cache[index].compressedContentSize=0;

  cache[index].dynamicRequest = context; // This should also pass the timeout value
  cache[index].dynamicRequestCallbackFunction = context->dynamicRequestCallbackFunction;

  return 1;
}

int cache_AddDoNOTCacheRuleForResource(struct AmmServer_Instance * instance,const char * filename)
{
   struct cache_item * cache = (struct cache_item *) instance->cache;
   unsigned int index=0;
   if (cache_FindResource(instance,filename,&index))
     {
       cache[index].doNOTCacheRule=1;
       fprintf(stderr,"Adding rule for %s => cache[%u].doNOTCacheRule=1; \n",filename,index);

       return 1;
     }
    else
     { //File Doesn't exist, we have to create a cache index for it , and then mark it as uncachable..!
       fprintf(stderr,"Resource %s did not exist we have to create a cache index for it , and then mark it as uncachable..! \n",filename);
       warningID(ASV_WARNING_CREATING_WORKAROUND_CACHE_ITEM);
       if (cache_CreateResource(instance,filename,&index) )
       {
        if (cache_FindResource(instance,filename,&index))
         {
          cache[index].doNOTCacheRule=1;
          fprintf(stderr,"Adding rule for %s => cache[%u].doNOTCacheRule=1; \n",filename,index);
          return 1;
         }
       }
     }

   errorID(ASV_ERROR_FAILED_TO_ADD_NOCACHE_RULE);
   return 0;
}

int cache_RemoveResource(struct AmmServer_Instance * instance,unsigned int index)
{
    struct cache_item * cache = (struct cache_item *) instance->cache;
    unsigned int i = index;
    if ( cache[i].dynamicRequestCallbackFunction !=0)
     {
        //This cache item is "dynamic content" as a result
        //it has its own memory handler , so we just dereference it..
        cache[i].dynamicRequestCallbackFunction=0;
        cache[i].content=0;
        cache[i].contentSize=0;
     }
       else
     {
      if ( cache[i].content != 0 )
       {
          free(cache[i].content);
          cache[i].content=0;
          if ( cache[i].contentSize != 0 ) { instance_CountFreeOP(instance,*cache[i].contentSize); } //If we have a filesize we subtract it from the cache malloc counter
       }
      if ( cache[i].contentSize != 0 )
       {
          free(cache[i].contentSize);
          cache[i].contentSize=0;
       }
     }

     //Compressed mem is not controlled by the callback so we can deallocate it at our convinience (not inside the if then else scope )!
      if ( cache[i].compressedContent != 0 )
       {
          free(cache[i].compressedContent);
          cache[i].compressedContent=0;
          if ( cache[i].contentSize != 0 ) { instance_CountFreeOP(instance,*cache[i].compressedContentSize); } //If we have a filesize we subtract it from the cache malloc counter
       }
      if ( cache[i].compressedContentSize != 0 )
       {
          free(cache[i].compressedContentSize);
          cache[i].compressedContentSize=0;
       }
   return 0;
}

int cache_RemoveContextAndResource(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem)
{
       context->requestContext.MAXcontentSize=0;
       if ((free_mem)&&(context->requestContext.content!=0)) { free(context->requestContext.content); context->requestContext.content=0; }

       unsigned int index=0;
       if (!cache_FindResource(instance,context->resource_name,&index) )
          {
            return 0;
          }
       return cache_RemoveResource(instance,index);
}

unsigned long cache_GetHashOfResource(struct AmmServer_Instance * instance,unsigned int index)
{
    return  hashMap_GetHashAtIndex(instance->cacheHashMap,index);
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


int cache_RefreshResource(
                           struct AmmServer_Instance * instance,
                           struct HTTPHeader * request,
                           char * verified_filename,
                           unsigned int * index,
                           unsigned long *filesize,
                           struct stat * last_modification
                         )
{
   if ( (instance==0) || (request==0) || (verified_filename==0) ||(index==0) || (filesize==0) || (last_modification==0)  ) { return 0; }
   AmmServer_Stub("cache_RefreshResource is a stub.. \n");
   #warning "TODO : we should check here for a potentially newer version of the file..";
 return 0;
}

char * cache_GetResource(
                          struct AmmServer_Instance * instance,
                          struct HTTPHeader * request,
                          unsigned int resourceCacheID,
                          char * verified_filename,
                          unsigned int verified_filenameSize,
                          unsigned int * index,
                          unsigned long *filesize,
                          struct stat * last_modification,
                          unsigned char * compressionSupported,
                          unsigned char * freeContentAfterUsingIt,
                          unsigned char * serveAsRegularFile,
                          unsigned char * allowOtherOrigins
                        )
{
 *freeContentAfterUsingIt=0; //By default we dont want to free the memory allocation after use..
 *serveAsRegularFile=0;      //By default we dont want to end up serving this as a regular file..!

 if (instance==0) { errorID(ASV_ERROR_INSTANCE_NOT_ALLOCATED); return 0;  }
 if (request==0)  { errorID(ASV_ERROR_REQUEST_NOT_ALLOCATED);  return 0;  }

 struct cache_item * cache = (struct cache_item *) instance->cache;
 if (cache==0)    { errorID(ASV_ERROR_CACHE_NOT_ALLOCATED); return 0;  }

 if (!CACHING_ENABLED)
 {
  fprintf(stderr,"Caching deactivated..!\n");
  *compressionSupported=0;
  return 0;
 }

//This can be avoided by adding an index as a parameter to this function call
*index  =  resourceCacheID;
if (cache_FindResource(instance,verified_filename,index))
 {
  //Initially we would like to work with the memory block allocated when the dynamic call
  //was first registered..
   char * cache_memory = cache[*index].content;
   if ( cache_RefreshResource(instance,request,verified_filename,index,filesize,last_modification) )
   {
     fprintf(stderr,"Updated Resource , to newest version \n");
   }

   //if doNOTCache is set and this is a real file..
   //fprintf(stderr,"Found Resource in our cache : \n");
   //fprintf(stderr,"index = %u\n",*index);
   //fprintf(stderr,"doNotCache = %u \n",cache[*index].doNOTCacheRule);
   //fprintf(stderr,"dynamicRequestCallbackFunction pointer = %p \n",cache[*index].dynamicRequestCallbackFunction);

   if ( (cache[*index].doNOTCacheRule)&& //If we forbid caching
        (cache[*index].dynamicRequestCallbackFunction==0)) //And we don't have a dynamic page to load!
    {
     fprintf(stderr,"We do not want to serve a cached version of this file..\n");
     *compressionSupported=0;
     return 0;
    }  else
     //We are allowed to search/use the cache!
    {
      //We want to check if a dynamic content is availiable  for this file
     unsigned char contentContainsPathToFileToBeStreamed=0;
     if ( dynamicRequest_ContentAvailiable(instance,*index) )
           {
             unsigned long memSize=0;
             char * mem = dynamicRequest_serveContent(
                                                       instance,
                                                       request,
                                                       cache[*index].dynamicRequest,
                                                       verified_filename,
                                                       verified_filenameSize,
                                                       *index,
                                                       &memSize,
                                                       compressionSupported,
                                                       freeContentAfterUsingIt,
                                                       &contentContainsPathToFileToBeStreamed,
                                                       allowOtherOrigins
                                                    );

             if ( contentContainsPathToFileToBeStreamed )
             {
                  AmmServer_Warning("Returning a file stream from something that started to be a dynamic request , filename is %s , lets hope it works" , verified_filename);
                  *serveAsRegularFile=1;

                  if (*freeContentAfterUsingIt)
                  {
                      AmmServer_Error("Please free content here..");
                  }

                  return 0;
             }

             if (mem==0)
             {
               warningID(ASV_WARNING_DYNAMIC_REQUEST_RETURNED_NOTHING);
               return 0;
             } else
             if (memSize==0)
             {
               warningID(ASV_WARNING_DYNAMIC_REQUEST_RETURNED_NOTHING);
               return 0;
             }


             //fprintf(stderr,"dynamicRequest_ContentAvailiable produced %lu bytes of usable content ",memSize);
             *compressionSupported=0;
             *filesize = memSize;
             return mem;
           }


           /*We want to serve a cached version of the file START*/
           if ( (cache[*index].compressedContent!=0) && (*compressionSupported!=0) && (ENABLE_COMPRESSION) )
           {
             *compressionSupported=1; // The response is compressed ( already set but in the future it may need to distinguish differnet compression schemes!..!

              /* We can and will serve back a cached version of the page..! */
             *filesize=*cache[*index].compressedContentSize;
             fprintf(stderr,"Cache Serving back a compressed buffer sized %lu bytes\n",*filesize);

             return cache[*index].compressedContent;
           }
               else
           if (cache_memory!=0)
           {
             *compressionSupported=0; // The response is not compressed..!

             *filesize=*cache[*index].contentSize;
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
              *compressionSupported=0;
              *filesize=*cache[*index].contentSize; //We return the filesize after the operation..
              return cache[*index].content; //because cache_memory hasn't been declared here..
            }
        }
       //If we are here we are unlocky , our file wasn't in cache and to make things worse we also failed to load it so
       //regular file sending it as it is ..!

       *compressionSupported=0;
       *filesize=0;
       fprintf(stderr,"Cache could not find file %s , filesize %u , compression support %u \n",verified_filename,(unsigned int) *filesize,*compressionSupported);

       return 0;
}
