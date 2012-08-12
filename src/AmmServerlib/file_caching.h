#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

int CachedVersionExists(char * verified_filename);
char * CheckForCachedVersionOfThePage(char * verified_filename,unsigned long *filesize,unsigned char gzip_supported);

int AddDirectResourceToCache(char * web_root_path,char * resource_name,char * content_memory,unsigned long * content_memory_size,void * prepare_content_callback);

unsigned int FindCacheIndexForFile(char * filename,unsigned int * index);

int InitializeCache(unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB);
int DestroyCache();

int ChangeRequestIfInternalRequestIsAddressed(char * request,char * templates_root);

#endif // FILE_CACHING_H_INCLUDED
