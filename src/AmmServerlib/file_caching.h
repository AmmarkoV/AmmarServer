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

extern struct cache_item * cache;

int CachedVersionExists(char * verified_filename,unsigned int * index);
char * CheckForCachedVersionOfThePage(struct HTTPRequest * request,char * verified_filename,unsigned int * index,unsigned long *filesize,struct stat * last_modification,unsigned char gzip_supported);

int AddDirectResourceToCache(struct AmmServer_RH_Context * context);
int RemoveDirectResourceToCache(struct AmmServer_RH_Context * context,unsigned char free_mem);
int DoNotCacheResource(char * filename);


unsigned int GetHashForCacheItem(unsigned int index);
unsigned int FindCacheIndexForResource(char * resource,unsigned int * index);

int InitializeCache(unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB);
int DestroyCache();

int ChangeRequestIfInternalRequestIsAddressed(char * request,char * templates_root);

#endif // FILE_CACHING_H_INCLUDED
