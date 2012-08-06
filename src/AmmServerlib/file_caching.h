#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

char * CheckForCachedVersionOfThePage(char * verified_filename,unsigned long *filesize,unsigned char gzip_supported);

int AddDirectResourceToCache(char * resource_name,char * content_memory,unsigned long * content_memory_size,void * prepare_content_callback);

int InitializeCache(unsigned int max_seperate_items , unsigned int max_total_allocation_MB);
int DestroyCache();

#endif // FILE_CACHING_H_INCLUDED
