#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

#include "AmmServerlib.h"

#include "http_header_analysis.h"

#include <sys/stat.h>
#include <time.h>

int CachedVersionExists(char * verified_filename,unsigned int * index);
char * CheckForCachedVersionOfThePage(struct HTTPRequest * request,char * verified_filename,unsigned long *filesize,struct stat * last_modification,unsigned char gzip_supported);

int AddDirectResourceToCache(struct AmmServer_RH_Context * context);
int RemoveDirectResourceToCache(struct AmmServer_RH_Context * context,unsigned char free_mem);
int DoNotCacheResource(char * filename);

unsigned int FindCacheIndexForFile(char * filename,unsigned int * index);

int InitializeCache(unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB,unsigned int max_seperate_variables , unsigned int max_total_var_allocation_MB,unsigned int max_var_allocation_per_entry_MB);
int DestroyCache();

int ChangeRequestIfInternalRequestIsAddressed(char * request,char * templates_root);

#endif // FILE_CACHING_H_INCLUDED
