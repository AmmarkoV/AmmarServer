#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

#include "AmmServerlib.h"

#include "http_header_analysis.h"

#include <sys/stat.h>
#include <time.h>

struct cache_item
{
   //TODO: add this to the checks to avoid hash collisions -> char filename[MAX_FILE_PATH];
   //it will have negative performance effect though :P

   struct AmmServer_RH_Context * context;

   unsigned long filename_hash;
   unsigned int hits;

   unsigned long * filesize;
   char * mem;

   int content_type; /*AS Declared in http_tools int GetContentType(char * filename,char * content_type) */
   unsigned long * compressed_mem_filesize;
   char * compressed_mem;

   void * prepare_mem_callback;

   unsigned char doNOTCache;

   /*Modification time..!*/
   unsigned char hour;
   unsigned char minute;
   unsigned char second;
   unsigned char wday;
   unsigned char day;
   unsigned char month;
   unsigned int  year;
};


int CachedVersionExists(struct AmmServer_Instance * instance,char * verified_filename,unsigned int * index);
char * CheckForCachedVersionOfThePage(struct AmmServer_Instance * instance,struct HTTPRequest * request,char * verified_filename,unsigned int * index,unsigned long *filesize,struct stat * last_modification,unsigned char * compression_supported);

int AddFile_As_CacheItem(struct AmmServer_Instance * instance,char * filename,unsigned int * index,struct stat * last_modification);
int AddDirectResource_As_CacheItem(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context);
int AddDoNOTCache_CacheItem(struct AmmServer_Instance * instance,char * filename);
int RemoveDirectResource_CacheItem(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem);


unsigned int GetHashForCacheItem(struct AmmServer_Instance * instance,unsigned int index);
unsigned int Find_CacheItem(struct AmmServer_Instance * instance,char * resource,unsigned int * index);

int InitializeCache(struct AmmServer_Instance * instance,unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB);
int DestroyCache(struct AmmServer_Instance * instance);

int ChangeRequestIfInternalRequestIsAddressed(struct AmmServer_Instance * instance,char * request,char * templates_root);

#endif // FILE_CACHING_H_INCLUDED
