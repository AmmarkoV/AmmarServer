#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../AmmServerlib.h"
#include "../server_configuration.h"
#include "../tools/http_tools.h"

#include "file_caching.h"
#include "file_compression.h"

#if ENABLE_COMPRESSION
#include <zlib.h>
#endif


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

  if (!WeCanCommitMoreMemoryForCaching(instance,initial_compressed_buffer_filesize_estimation)) { return 0; }


  //Second job is to prepare the compressed memory block , we clean it up and allocate an unsigned long ..!
  AddNewMallocOpToCacheCounter(instance,initial_compressed_buffer_filesize_estimation);
  cache[index].compressed_mem = (char * ) malloc(sizeof (char) * ( initial_compressed_buffer_filesize_estimation ));
  //We dont need to clear this buffer , it is a waste of time .. It will get filled in one step , so lets conserve CPU time..

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
                           AddNewMallocOpToCacheCounter(instance,*cache[index].compressed_mem_filesize); // We subtract the freed bytes as a second operation to take care of race conditions..!
                           AddFreeOpToCacheCounter(instance,initial_compressed_buffer_filesize_estimation);
                         }
     }
     //Finally , very important , never forget to mark the operation as successfull!
      return_value = 1;
     //-----------------------------
   } else
  if ( Z_BUF_ERROR==res )  { fprintf(stderr,"Compressed buffer was not created , The created buffer ( %lu bytes ) was not large enough to hold the compressed data.\n",initial_compressed_buffer_filesize_estimation); } else
  if ( Z_MEM_ERROR == res) { fprintf(stderr,"Compressed buffer was not created , Insufficient memory..\n"); } else
  if ( Z_STREAM_ERROR == res ) { fprintf(stderr,"Compressed buffer was not created , The compression level was not Z_DEFAULT_LEVEL, or was not between 0 and 9...\n"); }


  if (!return_value)
  { //Compression failed so we will now free our buffers..!

     AddFreeOpToCacheCounter(instance,initial_compressed_buffer_filesize_estimation); //A failed return_value ( compression ) means we still have our initial buffer , so we free the initial number of bytes..!
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
