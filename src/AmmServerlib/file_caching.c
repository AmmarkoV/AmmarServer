#include <stdio.h>
#include "configuration.h"
#include "file_caching.h"

enum cace_item_states
{
   UNUSED =0 ,
   LOADING ,
   READY
};

struct cache_item
{
   unsigned char state;
   char filename[MAX_FILE_PATH];
   unsigned int hits;
   unsigned long filesize;
};



/*! djb2
This algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
Needless to say , this is our hash function..!
*/
    unsigned long hash(unsigned char *str)
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



char * CheckForCachedVersionOfThePage(char * verified_filename,unsigned long *filesize,unsigned char gzip_supported)
{
       //TODO : Implement page caching..
       *filesize=0;

       fprintf(stderr,"Cache skipping check on %s , filesize %u , gzip support %u \n",verified_filename,(unsigned int) *filesize,gzip_supported);

       return 0;
}
