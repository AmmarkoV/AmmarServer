#include "hashmap.h"
#include "../tools/logs.h"

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



int hashMap_Grow(struct hashMap * hm,unsigned int growthSize)
{
  struct hashMapEntry * newentries = realloc(hm->entries , sizeof(struct hashMapEntry) * ( hm->maxNumberOfEntries + growthSize ) );
  if (newentries!=0)
     {
       hm->entries=newentries;
       memset(newentries+hm->maxNumberOfEntries,0,growthSize);
       hm->maxNumberOfEntries += growthSize;
       return 1;
     } else
     {
       warning("Could not grow hashMap (running out of memory?)");
     }
  return 0;
}

struct hashMap * hashMap_Create(unsigned int initialEntries , unsigned int entryAllocationStep)
{
  struct hashMap * hm = (struct hashMap *)  malloc(sizeof(struct hashMap));
  if (hm==0)  { error("Could not allocate a new hashmap"); return 0; }

  memset(hm,0,sizeof(struct hashMap));

  hm->entryAllocationStep=entryAllocationStep;

  if (!hashMap_Grow(hm,initialEntries) )
  {
    error("Could not grow new hashmap for the first time");
    free(hm);
    return 0;
  }

  return hm;
}

int hashMap_IsOK(struct hashMap * hm)
{
    if (hm == 0)  { return 0; }
    if (hm->entries == 0)  { return 0; }
    if (hm->maxNumberOfEntries == 0)  { return 0; }
    return 1;
}


struct hashMap * hashMap_Clone(struct hashMap * hm)
{
  return 0;
}

void hashMap_Clear(struct hashMap * hm)
{
  return;
}

void hashMap_Destroy(struct hashMap * hm)
{
  return ;
}

int hashMap_Add(struct hashMap * hm,char * key,void * val)
{
  if (hm->curNumberOfEntries >= hm->maxNumberOfEntries)
  {
    if  (!hashMap_Grow(hm,hm->entryAllocationStep))
    {
      error("Could not grow new hashmap for adding new values");
      return 0;
    }
  }

  //TODO ADD STUFF HERE :P


  return 0;
}

void * hashMap_Get(struct hashMap * hm,char * key,int * found)
{
  *found=0;
  if (!hashMap_IsOK(hm)) { return 0;}
  unsigned int i=0;
  unsigned long keyHash = hashFunction(key);
  while ( i < hm->curNumberOfEntries )
  {
    if ( hm->entries[i].keyHash == keyHash ) { *found=1; return i; }
    ++i;
  }
  return 0;
}


int hashMap_ContainsKey(struct hashMap * hm,char * key)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  unsigned int i=0;
  unsigned long keyHash = hashFunction(key);
  while ( i < hm->curNumberOfEntries )
  {
    if ( hm->entries[i].keyHash == keyHash ) { return 1; }
    ++i;
  }
  return 0;
}


int hashMap_ContainsValue(struct hashMap * hm,void * val)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  unsigned int i=0;
  while ( i < hm->curNumberOfEntries )
  {
    if ( hm->entries[i].payload == val ) { return 1; }
    ++i;
  }
  return 0;
}


int hashMap_GetSize(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  return 0;
}

