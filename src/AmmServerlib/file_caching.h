#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

char * CheckForCachedVersionOfThePage(char * verified_filename,unsigned long *filesize,unsigned char gzip_supported);

int InitializeCache(unsigned int max_seperate_items , unsigned int max_total_allocation_MB);
int DestroyCache();

#endif // FILE_CACHING_H_INCLUDED
