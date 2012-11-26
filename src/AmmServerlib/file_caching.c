#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AmmServerlib.h"
#include "server_configuration.h"
#include "file_caching.h"
#include "http_tools.h"
#include "time_provider.h"

#if ENABLE_COMPRESSION
#include <zlib.h>
#endif


/*
unsigned long instance->instance->loaded_cache_items_Kbytes=0;
unsigned int instance->loaded_cache_items=0;
struct cache_item * cache=0;*/
//The cache consists of cache items.. Each cache item Gets Added and Removed with calls defined beneath..
//The cache must be Created and then Destroyed because on program execution its pointer has no memory allocated for the content..!





/*! djb2
This algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
Needless to say , this is our hash function..!
*/
    static unsigned long hash(char *str)
    {
        if (str==0) return 0;
        if (str[0]==0) return 0;

        unsigned long hash = 5381; //<- magic
        int c=1;

        while (c != 0)
        {
            c = *str++;
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

        return hash;
    }


/*
     INDEXING for the cache
     The following calls manage Indexing for the cache..
     We can create a cache item in many ways ( an empty one , a real one , a virtual one)
     In order to reduce code repetitions all the create/delete/find indexing routines ( that are the same among all the above ways to create a cache itm )
     are implemented here
     ------------------------------------------------------------------
*/

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
int compress2(Bytef * dest, uLongf * destLen, const Bytef * source, uLong sourceLen, int level);

Description

The compress2() function shall attempt to compress sourceLen bytes of data in the buffer source, placing the result in the buffer dest, at the level described by level. The level supplied shall be a value between 0 and 9, or the value Z_DEFAULT_COMPRESSION. A level of 1 requests
the highest speed, while a level of 9 requests the highest compression. A level of 0 indicates that no compression should be used, and the output shall be the same as the input.

On entry, destLen should point to a value describing the size of the dest buffer. The application should ensure that this value be at least (sourceLen � 1.001) + 12. On successful exit,
the variable referenced by destLen shall be updated to hold the length of compressed data in dest.

The compress() function is equivalent to compress2() with a level of Z_DEFAULT_LEVEL.
Return Value

On success, compress2() shall return Z_OK. Otherwise, compress2() shall return a value to indicate the error.

*/

/*
   This function should populate cache[*index].compressed_mem_filesize and cache[*index].compressed_mem with a compressed version
   of the file in  cache[*index].mem
*/
inline int CreateCompressedVersionofCachedResource(struct AmmServer_Instance * instance,unsigned int index,int compression_level)
{
  if (!ENABLE_COMPRESSION) { return 0; }

  struct cache_item * cache = (struct cache_item *) instance->cache;


  //Todo check file type , if it is jpg , zip etc it doesnt need compression..!
  if ( cache[index].content_type!=TEXT ) { fprintf(stderr,"The content is not text , so we wont go in the trouble of compressing it..\n"); return 0; }
  //If it is css html etc compression would be very nice..

  int return_value = 0;

  if ( (cache[index].filesize==0)||(cache[index].mem==0) )
     {
       fprintf(stderr,"Cannot create Compressed content for non-existant buffer..!\n");
       return 0;
     }
  if (*cache[index].filesize==0)
     {
       fprintf(stderr,"Cannot create Compressed content for existant but empty buffer ..!\n");
       return 0;
     }

  //We free compressed buffers outside of the ENABLE_COMPRESSION precompiler selector , aftere the next 2 steps we have a clean state and so we are ready to allocate
  //memory for the compressed content
  if (cache[index].compressed_mem!=0) { free(cache[index].compressed_mem); cache[index].compressed_mem=0;
                                        if (cache[index].compressed_mem_filesize!=0) { AddFreeOpToCacheCounter(instance,*cache[index].compressed_mem_filesize); }  }
  if (cache[index].compressed_mem_filesize!=0) { free(cache[index].compressed_mem_filesize); cache[index].compressed_mem_filesize=0; }


  #if ENABLE_COMPRESSION
  //When compression is disabled we shouldn't link with -lz so we remove all calls to zlib stuff ( they are marked with a ZLIB CALL ) ..!

  unsigned long initial_compressed_buffer_filesize_estimation = compressBound( (uLongf) *cache[index].filesize); /*!ZLIB CALL!*/

  if (!WeCanCommitMoreMemoryForCaching(initial_compressed_buffer_filesize_estimation)) { return 0; }


  //Second job is to prepare the compressed memory block , we clean it up and allocate an unsigned long ..!
  AddNewMallocToCacheCounter(initial_compressed_buffer_filesize_estimation);
  cache[index].compressed_mem = (char * ) malloc(sizeof (char) * ( initial_compressed_buffer_filesize_estimation ));


  //First to prepare the memory length holder , we clean it up and allocate an unsigned long ..!
  cache[index].compressed_mem_filesize = (unsigned long * ) malloc(sizeof (unsigned long));
  *cache[index].compressed_mem_filesize = initial_compressed_buffer_filesize_estimation;




  int res=compress2( /*!ZLIB CALL!*/
                     (Bytef*)  cache[index].compressed_mem, //Destination *Compressed* file
                     (uLongf*) cache[index].compressed_mem_filesize, //Destination filesize (this will change so we pass a pointer)..
                     (Bytef*)  cache[index].mem,  //Source UNCompressed file
                     (uLongf)  *cache[index].filesize, //Source filesize ( this wont change so we pass it by value )
                     compression_level); //The compression level ( this needs some thought..! )
  if (Z_OK==res)
   {
     //If compression was a success we probably used less memory than what we thought we would before the procedure..
     //It could be the case that a realloc would help us use less system memory which will improve performance..
     //In case that the saved memory is less than a preset number of bytes it may not be worth it to make the realloc call..!
     if (REALLOC_TO_SAVE_MORE_THAN_THIS_NUMBER_BYTES<initial_compressed_buffer_filesize_estimation-*cache[index].compressed_mem_filesize)
     {
      char * better_fit = (char*) realloc ( cache[index].compressed_mem , *cache[index].compressed_mem_filesize );
      if (better_fit!=0) { cache[index].compressed_mem=better_fit;
                           AddNewMallocToCacheCounter(*cache[index].compressed_mem_filesize); // We subtract the freed bytes as a second operation to take care of race conditions..!
                           AddFreeOpToCacheCounter(initial_compressed_buffer_filesize_estimation);
                         }
     }
     //Finally , very important , never forget to mark the operation as successfull!
      return_value = 1;
     //-----------------------------
   } else
  if ( Z_BUF_ERROR==res )  { fprintf(stderr,"Compressed buffer was not created , The created buffer ( %u bytes ) was not large enough to hold the compressed data.\n",initial_compressed_buffer_filesize_estimation); } else
  if ( Z_MEM_ERROR == res) { fprintf(stderr,"Compressed buffer was not created , Insufficient memory..\n"); } else
  if ( Z_STREAM_ERROR == res ) { fprintf(stderr,"Compressed buffer was not created , The compression level was not Z_DEFAULT_LEVEL, or was not between 0 and 9...\n"); }


  if (!return_value)
  { //Compression failed so we will now free our buffers..!

     AddFreeOpToCacheCounter(initial_compressed_buffer_filesize_estimation); //A failed return_value ( compression ) means we still have our initial buffer , so we free the initial number of bytes..!
     free(cache[index].compressed_mem_filesize);
     free(cache[index].compressed_mem);

     cache[index].compressed_mem_filesize=0;
     cache[index].compressed_mem=0;
  }
  #endif

  return return_value;
}

/* ---------------------------------------------------------------------------------------------------------------------------------------
   The 3 functions that follow are aliases with different compression levels for the  CreateCompressedVersionofCachedResource(index,level);
   ---------------------------------------------------------------------------------------------------------------------------------------
*/
int CreateCompressedVersionofDynamicContent(struct AmmServer_Instance * instance,unsigned int index)
{
  if ( (!ENABLE_COMPRESSION) || (!ENABLE_DYNAMIC_CONTENT_COMPRESSION) ) { return 0; }
  //Dynamic Content should be compressed FAST! so 1 compression level
  return CreateCompressedVersionofCachedResource(instance,index,1);
}

int CreateCompressedVersionofStaticContent(struct AmmServer_Instance * instance,unsigned int index)
{
  if (!ENABLE_COMPRESSION) { return 0; }
  return CreateCompressedVersionofCachedResource(instance,index,3);
}

int CreateCompressedVersionofStaticContentPreloading(struct AmmServer_Instance * instance,unsigned int index)
{
  if (!ENABLE_COMPRESSION) { return 0; }
  return CreateCompressedVersionofCachedResource(instance,index,9);
}
/*
 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------



 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------
*/


int ChangeRequestIfInternalRequestIsAddressed(struct AmmServer_Instance * instance,char * request,char * templates_root)
{
  if (!ENABLE_INTERNAL_RESOURCES_RESOLVE)  { return 0; }
  //The role of request caching is to intercept incoming requests and if they are referring
  //to an internal resource using the TemplatesInternalURI URI we want to redirect the request
  //to our templates folder ..!
  //If the request was indeed a change request returns 1 else 0
  if ( strlen(request)>strlen(TemplatesInternalURI)+64 )
   {
       fprintf(stderr,"\nWARNING : Skipping ChangeRequestIfInternalRequestIsAddressed due to a very large request\n");
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


/*
 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------



 --------------------------------------------------------------------------------------
 --------------------------------------------------------------------------------------
*/

/*This is the Search Index Function , It is basically fully inefficient O(n) , it will be replaced by some binary search implementation*/
unsigned int Find_CacheItem(struct AmmServer_Instance * instance,char * resource,unsigned int * index)
{
  fprintf(stderr,"Serial slow searching for resource in cache %s ..",resource);

  struct cache_item * cache = (struct cache_item *) instance->cache;

  if (cache==0) { fprintf(stderr,"Cache hasn't been allocated yet\n"); return 0; }

  unsigned long file_we_are_looking_for = hash(resource);
  unsigned int i=0;

  for (i=0; i<instance->loaded_cache_items; i++)
   {
     if ( cache[i].filename_hash == file_we_are_looking_for )
      {
        ++cache[i].hits;
        if (cache[i].filesize!=0)
         { //Filesize gets pulled directly from the client.. For this reason we want to check if the client has allocated a value to prevent the possible segfault
          fprintf(stderr,"..found it here %u , %u hits , %0.2f Kbytes\n",i,cache[i].hits,(double) (*cache[i].filesize/1024) );
         }
        *index=i;
        return 1;
      }
   }

  return 0;
}

/*This is the Create Index Function , Nothing is sorted so we just append our cache item list with another one..
  Also for now only the hash is used to retrieve it ( talk about a sloppy implementation ) */
int Create_CacheItem(struct AmmServer_Instance * instance,char * resource,unsigned int * index)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;
  if (cache==0) { fprintf(stderr,"Cache hasn't been allocated yet\n"); return 0; }
  if (MAX_CACHE_SIZE_IN_MB<=instance->loaded_cache_items+1) { fprintf(stderr,"Cache is full , Could not Create_CacheItem(%s)",resource); return 0; }
  *index=instance->loaded_cache_items++;

  cache[*index].filename_hash = hash(resource);
  return 1;
}

/*This is the Destroy Index Function , it isn't implemented , as one can see.. */
int Destroy_CacheItem(unsigned int * index)
{
  fprintf(stderr,"Destroy_CacheItem(%u) hasn't been implemented yet\n",*index);
  return 0;
}


/*
   ------------------------------------------------------------------
   ------------------------------------------------------------------
   ------------------------------------------------------------------
*/


int LoadFileFromDisk_For_CacheItem(struct AmmServer_Instance * instance,char *filename,unsigned int * index)
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


int AddFile_As_CacheItem(struct AmmServer_Instance * instance,char * filename,unsigned int * index,struct stat * last_modification)
{
  if (!Create_CacheItem(instance,filename,index)) { /*We couldn't allocate a new index for this file */ return 0; }

  struct cache_item * cache = (struct cache_item *) instance->cache;

  fprintf(stderr,"Adding file %s to cache ( %0.2f / %u MB )\n",filename,(float) instance->loaded_cache_items_Kbytes/1048576 ,  MAX_CACHE_SIZE_IN_MB);

  if (!LoadFileFromDisk_For_CacheItem(instance,filename,index))
   {
       fprintf(stderr,"Could not read file %s into memory\n",filename);
       fprintf(stderr,"Erasing index from memory\n");
       Destroy_CacheItem(index);
       *index=0;
       return 0;
   }

  cache[*index].hits = 0;
  cache[*index].prepare_mem_callback=0; // No callback for this file..
  cache[*index].context=0;
  cache[*index].doNOTCache=0;

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



int AddDirectResource_As_CacheItem(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
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
  if (! Create_CacheItem(instance,full_filename,&index) ) { return 0; }

  cache[index].context = context;
  cache[index].mem = context->content;
  cache[index].filesize = &context->content_size;
  cache[index].compressed_mem_filesize=0;
  cache[index].compressed_mem=0;

  cache[index].hits = 0;
  cache[index].prepare_mem_callback = context->prepare_content_callback;

  return 1;
}


int AddDoNOTCache_CacheItem(struct AmmServer_Instance * instance,char * filename)
{
   struct cache_item * cache = (struct cache_item *) instance->cache;
   unsigned int index=0;
   if (Find_CacheItem(instance,filename,&index))  { cache[index].doNOTCache=1; }
    else
     {
       //File Doesn't exist, we have to create a cache index for it , and then mark it as uncachable..!
       if (!Create_CacheItem(instance,filename,&index) ) { return 0; }
       if (Find_CacheItem(instance,filename,&index)) { cache[index].doNOTCache=1; } else
                                             { return 0; } //Could not set doNOTCache..!
     }
   return 1;
}




int Remove_CacheItem(struct AmmServer_Instance * instance,unsigned int index)
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


int RemoveDirectResource_CacheItem(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem)
{
       context->MAX_content_size=0;
       if ((free_mem)&&(context->content!=0)) { free(context->content); context->content=0; }


       unsigned int index;
       if (!Find_CacheItem(instance,context->resource_name,&index) ) { fprintf(stderr,"Error..\n"); return 0; }
       return Remove_CacheItem(instance,index);
}

unsigned int GetHashForCacheItem(struct AmmServer_Instance * instance,unsigned int index)
{
    struct cache_item * cache = (struct cache_item *) instance->cache;
    return cache[index].filename_hash;
}



int InitializeCache(struct AmmServer_Instance * instance,unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB)
{
  struct cache_item * cache = (struct cache_item *) instance->cache;

   MAX_CACHE_SIZE_IN_MB=max_total_allocation_MB;
   MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB=max_allocation_per_entry_MB;
   MAX_SEPERATE_CACHE_ITEMS=max_seperate_items;
  if (cache==0)
   {
     cache = (struct cache_item *) malloc(sizeof(struct cache_item) * (MAX_SEPERATE_CACHE_ITEMS+1));
     if (cache == 0) { fprintf(stderr,"Unable to allocate initial cache memory\n"); return 0; }
   }


//  InitializeVariableCache(max_seperate_variables, max_total_var_allocation_MB , max_var_allocation_per_entry_MB);


   unsigned int i=0;
   for (i=0; i<MAX_SEPERATE_CACHE_ITEMS; i++) { cache[i].mem=0; cache[i].filesize=0; cache[i].prepare_mem_callback=0; }
   return 1;
}

int DestroyCache(struct AmmServer_Instance * instance)
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
      Remove_CacheItem(instance,i);
   }

   free(cache);
   cache = 0;
   instance->loaded_cache_items=0;
   instance->loaded_cache_items_Kbytes=0;

   return 1;
}


/*

  ----------------------------------------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------------------------------------

     This ( CheckForCachedVersionOfThePage ) is the most heavily used and complex call in this file , it is what the file server
     calls when a new file is requested.. and it both serves ready cache items but also creates them if we need them..
                 It also has to deal with calling the callback function for dynamic content

  ----------------------------------------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------------------------------------
*/

int CachedVersionExists(struct AmmServer_Instance * instance,char * verified_filename,unsigned int * index)
{
    if (Find_CacheItem(instance,verified_filename,index)) { return 1; }
    return 0;
}

char * CheckForCachedVersionOfThePage(struct AmmServer_Instance * instance,struct HTTPRequest * request,char * verified_filename,unsigned int * index,unsigned long *filesize,struct stat * last_modification,unsigned char * compression_supported)
{
      if (!CACHING_ENABLED)
      {
        fprintf(stderr,"Caching deactivated..!\n");
        *compression_supported=0;
        return 0;
      }

      struct cache_item * cache = (struct cache_item *) instance->cache;


       if (Find_CacheItem(instance,verified_filename,index)) //This can be avoided by adding an index as a parameter to this function call
        {
           //if doNOTCache is set and this is a real file..
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
                if ( (cache[*index].mem==0) || (cache[*index].filesize==0) )
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
                            return cache[*index].mem;
                          } else
                          {
                           fprintf(stderr,"Request deserves fresh page , %u last gen, %u now , %u cooldown\n",shared_context->last_callback,now,shared_context-> callback_every_x_msec);
                          }
                   }

                   /*Do callback here*/
                   shared_context->callback_cooldown=0;
                   shared_context->last_callback = now;
                   void ( *DoCallback) (unsigned int)=0 ;
                   DoCallback = cache[*index].prepare_mem_callback;


                   /*If we have GET or POST request variables , lets pass them through to our shared context.. */
                   shared_context->GET_request = request->GETquery;
                   if (shared_context->GET_request!=0) { shared_context->GET_request_length = strlen(shared_context->GET_request); } else
                                                        { shared_context->GET_request_length = 0; }

                   shared_context->POST_request = request->POSTquery;
                   if (shared_context->POST_request!=0) { shared_context->POST_request_length = strlen(shared_context->POST_request); } else
                                                         { shared_context->POST_request_length = 0; }

                   unsigned int UNUSED=666; // <- These variables are associated with this page ( POST / GET vars )
                   //They are an id ov the var_caching.c list so that the callback function can produce information based on them..!
                   DoCallback(UNUSED);
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
             fprintf(stderr,"Cache Serving back a compressed buffer sized %u bytes\n",*filesize);

             return cache[*index].compressed_mem;
           }
               else
           if (cache[*index].mem!=0)
           {
             *compression_supported=0; // The response is not compressed..!

             *filesize=*cache[*index].filesize;
             fprintf(stderr,"Cache Serving back a buffer sized %u bytes\n",*filesize);

             return cache[*index].mem;
           }

          /*We want to serve a cached version of the file END*/
         }
        } else
        {
           /* A cached copy doesn't seem to exist , lets make one and then claim it exists! */
           if ( AddFile_As_CacheItem(instance,verified_filename,index,last_modification) )
            {
              *compression_supported=0;
              *filesize=*cache[*index].filesize; //We return the filesize after the operation..
              return cache[*index].mem;
            }
        }
       //If we are here we are unlocky , our file wasn't in cache and to make things worse we also failed to load it so
       //regular file sending it as it is ..!

       *compression_supported=0;
       *filesize=0;
       fprintf(stderr,"Cache could not find file %s , filesize %u , compression support %u \n",verified_filename,(unsigned int) *filesize,*compression_supported);

       return 0;
}

