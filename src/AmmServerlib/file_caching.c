#include <stdio.h>
#include <stdlib.h>
#include "configuration.h"
#include "file_caching.h"


struct cache_item
{
   char filename[MAX_FILE_PATH];
   unsigned long filename_hash;
   unsigned int hits;
   unsigned long filesize;
   char * mem;
};


unsigned char CACHING_ENABLED=1;
unsigned int MAX_TOTAL_ALLOCATION_IN_MB=64;
unsigned int MAX_CACHE_SIZE=1000;
unsigned long loaded_cache_items_Kbytes;
unsigned int loaded_cache_items;
struct cache_item * cache=0;

/*! djb2
This algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
Needless to say , this is our hash function..!
*/
    unsigned long hash(char *str)
    {
        if (str==0) return 0;
        if (str[0]==0) return 0;

        unsigned long hash = 5381;
        int c=1;

        while (c != 0)
        {
            c = *str++;
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

        return hash;
    }


unsigned int FindCacheIndexForFile(char * filename,unsigned int * index)
{
  fprintf(stderr,"Searching for file in cache%s ..",filename);
  unsigned long file_we_are_looking_for = hash(filename);
  unsigned int i=0;

  for (i=0; i<loaded_cache_items; i++)
   {
     if ( cache[i].filename_hash == file_we_are_looking_for )
      {
        ++cache[i].hits;
        fprintf(stderr," .. found it here %u , %u hits\n",i,cache[i].hits);
        *index=i;
        return 1;
      }
   }

  return 0;
}

int AddFileToCache(char * filename,unsigned int * index)
{
  fprintf(stderr,"Adding file %s to cache ( %0.2f / %u MB )\n",filename,(float) loaded_cache_items_Kbytes/1048576 , MAX_TOTAL_ALLOCATION_IN_MB);
  FILE * pFile = fopen (filename, "rb" );
  if (pFile==0) { fprintf(stderr,"Could not open file to cache it.. %s\n",filename); return 0;}
  if ( fseek (pFile , 0 , SEEK_END)!=0 ) { fprintf(stderr,"Could not find file size to cache client..!\nUnable to serve client\n"); fclose(pFile); return 0; }
  unsigned long lSize = ftell (pFile);
  if (MAX_TOTAL_ALLOCATION_IN_MB*1024<loaded_cache_items_Kbytes+lSize/1024)  { fprintf(stderr,"We have reached the soft cache limit of %u MB\n",MAX_TOTAL_ALLOCATION_IN_MB); fclose(pFile); return 0; }
  rewind (pFile);
  char * buffer = (char*) malloc ( sizeof(char) * (lSize));
  if (buffer == 0 ) { fprintf(stderr,"Could not allocate enough memory to cache this file..!\n"); fclose(pFile); return 0;  }
  *index=loaded_cache_items++;
  loaded_cache_items_Kbytes+=(unsigned int) lSize/1024;

  // copy the file into the buffer:
  size_t result;
  result = fread (buffer,1,lSize,pFile);
  if (result != lSize) { fprintf(stderr,"Reading error , while filling in newly allocated cache item %s \n",filename); free (buffer); fclose (pFile); return 0; }
  // file has been cached , so time to make it show up on the cache array
  cache[*index].filename_hash = hash(filename);
  cache[*index].mem = buffer;
  cache[*index].filesize = lSize;
  cache[*index].hits = 0;
  fprintf(stderr,"File %s has %u bytes cached with index %u \n",filename,(unsigned int ) lSize,*index);
  fclose(pFile);

  return 1;
}


char * CheckForCachedVersionOfThePage(char * verified_filename,unsigned long *filesize,unsigned char gzip_supported)
{
      if (!CACHING_ENABLED)
      {
        fprintf(stderr,"Caching deactivated..!\n");
        return 0;
      }

       unsigned int index=0;
       if (FindCacheIndexForFile(verified_filename,&index))
        {
           if (cache[index].mem!=0)
           {
             *filesize=cache[index].filesize;
             return cache[index].mem;
           }
        } else
        {
           if ( AddFileToCache(verified_filename,&index) )
            {
              *filesize=cache[index].filesize;
              return cache[index].mem;
            }
        }
       //If we are here we are unlocky , our file wasn't in cache and to make things worse we also failed to load it so
       //regular file sending it is ..!

       //TODO : Implement page caching..
       *filesize=0;
       fprintf(stderr,"Cache could not find file %s , filesize %u , gzip support %u \n",verified_filename,(unsigned int) *filesize,gzip_supported);

       return 0;
}

int InitializeCache(unsigned int max_seperate_items , unsigned int max_total_allocation_MB)
{
  MAX_TOTAL_ALLOCATION_IN_MB=max_total_allocation_MB;
  if (cache==0)
   {
     cache = (struct cache_item *) malloc(sizeof(struct cache_item) * (max_seperate_items+1));
     if (cache == 0) { fprintf(stderr,"Unable to allocate initial cache memory\n"); return 0; }
   }

   unsigned int i=0;
   for (i=0; i<max_seperate_items; i++) { cache[i].mem=0; }
   return 1;
}

int DestroyCache()
{
   unsigned int i=0;
   for (i=0; i<loaded_cache_items; i++)
   {
     if ( cache[i].mem != 0 )
      {
          free(cache[i].mem);
          cache[i].mem=0;
      }
   }

   free(cache);
   cache = 0;
   loaded_cache_items=0;

   return 1;
}
