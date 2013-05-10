#include "hashmap.h"

/*! djb2
This algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
Needless to say , this is our hash function..!
*/
unsigned long hashFunction(char *str)
    {
        if (str==0) return 0;
        if (str[0]==0) return 0;

        unsigned long hash = 5381; //<- magic
        int c=1;

        while (c != 0)
        {
            c = *str++;
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

        return hash;
    }



struct hashMap * hashMap_Create(unsigned int initialEntries , unsigned int entryAllocationStep)
{
  return 0;
}

struct hashMap * hashMap_Clone(struct hashMap * hm)
{
  return 0;
}

void hashMap_Destroy(struct hashMap * hm)
{
  return ;
}

void * hashMap_Add(struct hashMap * hm,char * key,void * val)
{
  return 0;
}

void * hashMap_Get(struct hashMap * hm,char * key)
{
  return 0;
}

void hashMap_Clear(struct hashMap * hm)
{
  return;
}

int hashMap_ContainsKey(struct hashMap * hm,char * key)
{
  return 0;
}


int hashMap_ContainsValue(struct hashMap * hm,void * val)
{
  return 0;
}


int hashMap_GetSize(struct hashMap * hm)
{
  return 0;
}

