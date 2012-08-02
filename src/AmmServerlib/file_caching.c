#include "file_caching.h"


/* djb2
this algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
*/
    unsigned long hash(unsigned char *str)
    {
        unsigned long hash = 5381;
        int c;

        while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return hash;
    }



char * CheckForCachedVersionOfThePage(char * verified_filename,unsigned long *filesize)
{
       //TODO : Implement page caching..
       filesize=0;
       return 0;
}
