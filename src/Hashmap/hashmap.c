//
//hashmap implementation for C , original (master) repository -> https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/hashmap/hashmap.c
//written by Ammar Qammaz
//


#include <stdio.h>     /* files */
#include <stdlib.h>     /* qsort */
#include <string.h>     /* memset */


#include "hashmap.h"


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */

#if HASHMAP_BE_THREAD_SAFE
 #define HM_LOCK(hm)   pthread_mutex_lock(&(hm)->hm_addLock)
 #define HM_UNLOCK(hm) pthread_mutex_unlock(&(hm)->hm_addLock)
#else
 #define HM_LOCK(hm)
 #define HM_UNLOCK(hm)
#endif // HASHMAP_BE_THREAD_SAFE




/*! djb2
This algorithm (k=33) was first reported by dan bernstein many years ago in comp.lang.c. another version of this algorithm (now favored by bernstein) uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it works better than many other constants, prime or not) has never been adequately explained.
Needless to say , this is our hash function..!
*/
unsigned long hashFunction(const char *str)
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
  if (growthSize==0) { fprintf(stderr,RED "HashMap: Refusing to grow hashMap by 0 entries\n" NORMAL); return 0; }
  struct hashMapEntry * newentries = realloc(hm->entries , sizeof(struct hashMapEntry) * ( hm->maxNumberOfEntries + growthSize ) );
  if (newentries!=0)
     {
       hm->entries=newentries;
       memset(newentries+hm->maxNumberOfEntries,0,growthSize * sizeof(struct hashMapEntry));
       hm->maxNumberOfEntries += growthSize;
       return 1;
     } else
     {
       fprintf(stderr,RED"HashMap: Could not grow hashMap (running out of memory?)" NORMAL);
     }
  return 0;
}

struct hashMap * hashMap_Create(unsigned int initialEntries,unsigned int entryAllocationStep,void * clearItemFunction , unsigned int useSorting)
{
  struct hashMap * hm = (struct hashMap *)  malloc(sizeof(struct hashMap));
  if (hm==0)  { fprintf(stderr,RED "HashMap: Could not allocate a new hashmap" NORMAL); return 0; }

  memset(hm,0,sizeof(struct hashMap));
  if (entryAllocationStep==0) { entryAllocationStep=1; } //Growing by 0 would let hashMap_Add() write past the end

  hm->entries=0;
  hm->entryAllocationStep=entryAllocationStep;
  hm->maxNumberOfEntries=0;
  hm->curNumberOfEntries=0;
  hm->useSorting=useSorting;

  if (!hashMap_Grow(hm,initialEntries) )
  {
    fprintf(stderr,YELLOW "HashMap: Could not grow new hashmap for the first time" NORMAL);
    free(hm);
    return 0;
  }

  hm->clearItemCallbackFunction = clearItemFunction;

    #if HASHMAP_BE_THREAD_SAFE
      pthread_mutex_init(&hm->hm_addLock,0);
      pthread_mutex_init(&hm->hm_fileLock,0);
    #endif // HASHMAP_BE_THREAD_SAFE

  return hm;
}

int hashMap_IsOK(struct hashMap * hm)
{
    if (hm == 0)  { return 0; }
    HM_LOCK(hm); //entries/maxNumberOfEntries change when hashMap_Add() grows the map , must not be called with the lock held
    int ok = ( (hm->entries != 0) && (hm->maxNumberOfEntries != 0) );
    HM_UNLOCK(hm);
    return ok;
}

int hashMap_GetCurrentNumberOfEntries(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  return hm->curNumberOfEntries;
}

int hashMap_GetMaxNumberOfEntries(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  return hm->maxNumberOfEntries;
}


int hashMap_IsSorted(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  unsigned int i=1;
    while ( i < hm->curNumberOfEntries )
    {
      if (hm->entries[i-1].keyHash > hm->entries[i].keyHash)
         { return 0; /*We got ourself a non sorted entry!*/ }
      ++i;
    }
  return 1;
}


void hashMap_Clear(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return; }
  HM_LOCK(hm);
  void ( *hashMapClearCallback) ( void * )=0 ;
  hashMapClearCallback = hm->clearItemCallbackFunction;
  unsigned int i=0;
  unsigned int entryNumber = hm->curNumberOfEntries; //cur

  hm->isSorted=0;
  hm->curNumberOfEntries = 0;

  while (i < entryNumber)
  {
    //Clear the payload , if we have a callback function for that use it
    if (hm->clearItemCallbackFunction != 0 )
         { hashMapClearCallback(hm->entries[i].payload); } else
    //If payload is not just a pointer ( length != 0 )  , free it
    if (hm->entries[i].payloadLength!=0)
         {
           //fprintf(stderr,"Clearing %u/%u entry payload\n", i , entryNumber );
           free(hm->entries[i].payload);
           hm->entries[i].payload=0;
         }


    if (hm->entries[i].key!=0)
    {
      //fprintf(stderr,"Clearing %u/%u entry key\n", i , entryNumber );
      free(hm->entries[i].key);
      hm->entries[i].key=0;
    }

    memset(&hm->entries[i],0,sizeof(struct hashMapEntry));

    ++i;
  }

  HM_UNLOCK(hm);
  return;
}

void hashMap_Destroy(struct hashMap * hm)
{
  if (hm==0) { return ; }

  if ( hm->entries != 0)
  {
   hashMap_Clear(hm); //Takes the lock , so it must run before the mutexes are destroyed
   hm->maxNumberOfEntries=0;
   free(hm->entries);
   hm->entries=0;
  }

    #if HASHMAP_BE_THREAD_SAFE
      pthread_mutex_destroy(&hm->hm_addLock);
      pthread_mutex_destroy(&hm->hm_fileLock);
    #endif // HASHMAP_BE_THREAD_SAFE

  hm->clearItemCallbackFunction=0;
  free(hm);


  return ;
}


/* qsort struct comparision function (price float field) ( See qsort call in main ) */
int cmpHashTableItems (const void * a, const void * b)
{
  struct hashMapEntry *ia = (struct hashMapEntry *)a;
  struct hashMapEntry *ib = (struct hashMapEntry *)b;
  if ( ia->keyHash <  ib->keyHash ) return -1;
  if ( ia->keyHash == ib->keyHash ) return 0;
 return 1;
}

//Caller must hold hm_addLock
static void hashMap_SortUnlocked(struct hashMap * hm)
{
  qsort(
         hm->entries ,
         hm->curNumberOfEntries ,
         sizeof(struct hashMapEntry),
         cmpHashTableItems
        );
  hm->isSorted=1;
}

int hashMap_Sort(struct hashMap * hm)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  if (!hm->useSorting) { fprintf(stderr,YELLOW "HashMap: Sorting is disabled for this hashmap\n" NORMAL); return 0; }
  HM_LOCK(hm);
  hashMap_SortUnlocked(hm);
  HM_UNLOCK(hm);
  return 1;
}


//Caller must hold hm_addLock
int hashMap_FindIndexSerialWithPrecomputedHash(struct hashMap * hm,const char * key , unsigned long keyHash,unsigned long * index)
{
  unsigned long i=0;
   //If the hashmap is not sorted then we have to do a stupid and really slow serial search
  while ( i < hm->curNumberOfEntries )
  {
    //fprintf(stderr,"Comparing %s to %s .. ",key,hm->entries[i].key);
    if ( hm->entries[i].keyHash == keyHash )
         {
          if ( (hm->entries[i].key!=0) && (strcmp(key,hm->entries[i].key)==0) )
          {
           ++hm->entries[i].hits;
           *index = i;
           //fprintf(stderr,"hit\n");
           return 1;
          }
         }
   //fprintf(stderr,"nope\n");
   ++i;
  }
 return 0;
}

int hashMap_FindIndexSerial(struct hashMap * hm,const char * key,unsigned long * index)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  if (key==0) { return 0; }
  unsigned long keyHash = hashFunction(key);
  HM_LOCK(hm);
  int result = hashMap_FindIndexSerialWithPrecomputedHash(hm,key,keyHash,index);
  HM_UNLOCK(hm);
  return result;
}

//Caller must hold hm_addLock
int hashMap_BSearch(struct hashMap* hm,const char *  key , unsigned long keyHash,unsigned long * index)
{
  if (!hm->useSorting) { fprintf(stderr,YELLOW "HashMap: Sorting is disabled for this hashmap\n" NORMAL); return 0; }
  struct hashMapEntry needle = {0};
  needle.keyHash = keyHash;
  struct hashMapEntry * haystack = hm->entries;

  struct hashMapEntry * pItem = (struct hashMapEntry *) bsearch (
                                                                  (void*) &needle,
                                                                  (void*) haystack ,
                                                                  hm->curNumberOfEntries ,
                                                                  sizeof(struct hashMapEntry),
                                                                  cmpHashTableItems
                                                                );
  if (pItem!=0) {
                  //The position comes from the pointer itself , qsort() moves entries around so any stored index goes stale
                  *index = (unsigned long) (pItem - haystack);
                  //printf ("%ul is in the array[%u] (%s vs %s) .\n",keyHash,*index , pItem->key , key);

                  if ( (pItem->key==0) || (strcmp(key, pItem->key)!=0) )
                  {
                    fprintf(stderr,YELLOW "HashMap: Hash collision on binary search.. reverting to slow serial search..\n" NORMAL);
                    return hashMap_FindIndexSerialWithPrecomputedHash(hm,key,keyHash,index);
                  }

                 return 1;
                }
                //else { printf ("%ul is not in the array of %u elements .\n",keyHash , hm->curNumberOfEntries ); }
  return 0;
}


int hashMap_Add(struct hashMap * hm,const char * key,void * val,unsigned int valLength)
{
  if (!hashMap_IsOK(hm)) { fprintf(stderr,YELLOW "HashMap: Cannot add something to an uninitialized hashmap..\n" NORMAL);  return 0; }
  if (key==0) { fprintf(stderr,YELLOW "HashMap: Cannot add an entry without a key..\n" NORMAL);  return 0; }
  if ( (valLength!=0) && (val==0) ) { fprintf(stderr,YELLOW "HashMap: Cannot copy %u bytes from a null payload..\n" NORMAL,valLength);  return 0; }

  //Allocate everything before taking the lock , so failures need no unwinding of the hashmap
  unsigned int keyLength = strlen(key);
  char * newKey = (char *) malloc(sizeof(char) * (keyLength+1) );
  if (newKey==0) { fprintf(stderr,"HashMap: While Adding a new key to hashmap , could not allocate key\n"); return 0; }
  memcpy(newKey,key,keyLength+1);

  void * newPayload = val; //valLength==0 : We store and serve direct pointers! :)
  if (valLength!=0)
  {
    newPayload = malloc(sizeof(char) * valLength);
    if (newPayload==0)
    {
      fprintf(stderr,"HashMap: While Adding a new key to hashmap , couldn't allocate payload\n");
      free(newKey);
      return 0;
    }
    memcpy(newPayload,val,valLength);
  }

  HM_LOCK(hm); // LOCK PROTECTED OPERATION -------------------------------------------

  if (hm->curNumberOfEntries >= hm->maxNumberOfEntries)
  {
    if  (!hashMap_Grow(hm,hm->entryAllocationStep))
    {
      HM_UNLOCK(hm);
      fprintf(stderr,YELLOW "HashMap: Could not grow new hashmap for adding new values\n" NORMAL);
      free(newKey);
      if (valLength!=0) { free(newPayload); }
      return 0;
    }
  }

  struct hashMapEntry * entry = &hm->entries[hm->curNumberOfEntries++];
  entry->keyHash       = hashFunction(key);
  entry->keyLength     = keyLength;
  entry->key           = newKey;
  entry->payloadLength = valLength;
  entry->payload       = newPayload;
  entry->hits          = 0;

  hm->isSorted=0;

  HM_UNLOCK(hm); // LOCK PROTECTED OPERATION -------------------------------------------

  return 1;
}


int hashMap_AddULong(struct hashMap * hm,const char * key,unsigned long val)
{
  void * valPTRForm=0;
  valPTRForm = (void *) val;
  return hashMap_Add(hm,key,valPTRForm,0);
}

int hashMap_PrepareForQueries(struct hashMap *hm)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if (!hm->useSorting)   { return 1;} //Nothing to prepare

  HM_LOCK(hm);
  if (!hm->isSorted)
  {
     hashMap_SortUnlocked(hm);
  }
  HM_UNLOCK(hm);

return 1;
}



//Caller must hold hm_addLock , *index is used as a hint and receives the result
static int hashMap_FindIndexUnlocked(struct hashMap * hm,const char * key,unsigned long * index)
{
  if ( (!hm->isSorted) && (hm->useSorting) ) { hashMap_SortUnlocked(hm); }

  unsigned long keyHash = hashFunction(key);

  //Maybe the index was the answer all along ( the hash alone is not enough , different keys can share a hash )
  if (*index< hm->curNumberOfEntries)
  {
    if ( (hm->entries[*index].keyHash == keyHash) && (hm->entries[*index].key!=0) && (strcmp(key,hm->entries[*index].key)==0) )
    {
     ++hm->entries[*index].hits;
     return 1;
    }
  }

  //If the hashmap is sorted and "big" we do a fast binary search
 if ( (hm->isSorted) && (hm->useSorting) && ( hm->curNumberOfEntries>NUMBER_OF_ENTRIES_BELOW_IT_IS_NOT_WORTH_SORTING ) )
 {
   return hashMap_BSearch(hm,key,keyHash,index);
 } else
 {
  //If the hashmap is not sorted then we have to do a stupid and really slow serial search
  return hashMap_FindIndexSerialWithPrecomputedHash(hm,key,keyHash,index);
 }
  return 0;
}

int hashMap_FindIndex(struct hashMap * hm,const char * key,unsigned long * index)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if ( (key==0) || (index==0) ) { return 0;}
  HM_LOCK(hm);
  int result = hashMap_FindIndexUnlocked(hm,key,index);
  HM_UNLOCK(hm);
  return result;
}






int hashmap_SwapRecords(struct hashMap * hm , unsigned int index1,unsigned int index2)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  HM_LOCK(hm);
  if ( (index1 >= hm->curNumberOfEntries ) || (index2 >= hm->curNumberOfEntries ) ) { HM_UNLOCK(hm); return 0; }

  struct hashMapEntry buf  = hm->entries[index1];
  hm->entries[index1]      = hm->entries[index2];
  hm->entries[index2]      = buf;
  if (index1!=index2) { hm->isSorted=0; }
  HM_UNLOCK(hm);
  return 1;
}


char * hashMap_GetKeyAtIndex(struct hashMap * hm,unsigned int index)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if (index >= hm->curNumberOfEntries ) { return 0; }
  return hm->entries[index].key;
}


unsigned long hashMap_GetHashAtIndex(struct hashMap * hm,unsigned int index)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if (index >= hm->curNumberOfEntries ) { return 0; }
  return hm->entries[index].keyHash;
}

int hashMap_GetPayload(struct hashMap * hm,const char * key,void ** payload)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if ( (key==0) || (payload==0) ) { return 0;}
  int result=0;
  unsigned long i=0;
  HM_LOCK(hm);
  if (hashMap_FindIndexUnlocked(hm,key,&i))
    {
       *payload = hm->entries[i].payload;
       result=1;
    }
  HM_UNLOCK(hm);
  return result;
}

int hashMap_GetULongPayload(struct hashMap * hm,const char * key,unsigned long * payload)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  if ( (key==0) || (payload==0) ) { return 0;}
  int result=0;
  unsigned long i=*payload;
  HM_LOCK(hm);
  if (hashMap_FindIndexUnlocked(hm,key,&i))
     {
       *payload = (unsigned long) hm->entries[i].payload;
       result=1;
     }
  HM_UNLOCK(hm);
  return result;
}

void * hashMap_GetPayloadAtIndex(struct hashMap * hm,unsigned int index,unsigned int * payloadLength)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  if (index >= hm->curNumberOfEntries) { return 0; }
  if (payloadLength!=0) { *payloadLength = hm->entries[index].payloadLength; }
  return hm->entries[index].payload;
}

int hashMap_RemoveKey(struct hashMap * hm,const char * key)
{
  if (!hashMap_IsOK(hm)) { return 0; }
  if (key==0) { return 0; }

  unsigned long keyHash = hashFunction(key);
  unsigned long index=0;

  HM_LOCK(hm); // LOCK PROTECTED OPERATION - the lookup must be inside the lock or the index can go stale before we use it
  if (!hashMap_FindIndexSerialWithPrecomputedHash(hm,key,keyHash,&index)) { HM_UNLOCK(hm); return 0; } //Serial on purpose - correct even if isSorted is stale

  void ( *hashMapClearCallback) ( void * )=0 ;
  hashMapClearCallback = hm->clearItemCallbackFunction;

  if (hashMapClearCallback != 0)                     { hashMapClearCallback(hm->entries[index].payload); }
  else if (hm->entries[index].payloadLength != 0)     { free(hm->entries[index].payload); }
  if (hm->entries[index].key != 0)                    { free(hm->entries[index].key); }

  unsigned int lastIndex = hm->curNumberOfEntries - 1;
  if ( (unsigned int) index != lastIndex ) { memcpy(&hm->entries[index],&hm->entries[lastIndex],sizeof(struct hashMapEntry)); }
  memset(&hm->entries[lastIndex],0,sizeof(struct hashMapEntry));

  --hm->curNumberOfEntries;
  hm->isSorted=0; //Swap-with-last compaction breaks sort order, same as hashMap_Add() already does on insert

  HM_UNLOCK(hm); // LOCK PROTECTED OPERATION -------------------------------------------

  return 1;
}

int hashMap_ContainsKey(struct hashMap * hm,const char * key)
{
  unsigned long index=0;
  return hashMap_FindIndex(hm,key,&index);
}

int hashMap_ContainsValue(struct hashMap * hm,void * val)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  int result=0;
  unsigned int i=0;
  HM_LOCK(hm);
  while ( i < hm->curNumberOfEntries )
  {
    if ( hm->entries[i].payload == val ) { result=1; break; }
    ++i;
  }
  HM_UNLOCK(hm);
  return result;
}

int hashMap_Print(struct hashMap * hm,const char * title)
{
  if (!hashMap_IsOK(hm)) { return 0;}
  HM_LOCK(hm);
  fprintf(stderr,"Hash map %s Printout -----------------------------------\n",title);
  unsigned int i=0;
  for (i=0; i<hm->curNumberOfEntries; i++)
  {
   fprintf(stderr,"#%u - %s => %lu \n",i,hm->entries[i].key , (unsigned long) hm->entries[i].payload);
  }
  fprintf(stderr,"---------------------------------------------------------\n");
  HM_UNLOCK(hm);
  return 1;
}

int hashMap_SaveToFile(struct hashMap * hm,const char * filename)
{
  if (!hashMap_IsOK(hm)) { return 0;}

  #if HASHMAP_BE_THREAD_SAFE
     pthread_mutex_lock (&hm->hm_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
  #endif // HASHMAP_BE_THREAD_SAFE

  int result = 0;
  FILE * pFile=0;
  pFile = fopen (filename,"wb");
  unsigned int zero=0;
  if (pFile!=0)
   {
    HM_LOCK(hm); //Entries must not be reallocated/removed while we are writing them out
    unsigned int i=0;

    char uintsize=sizeof(hm->entries[0].keyLength);
    fwrite(&uintsize,1,1,pFile);
    fwrite(&hm->curNumberOfEntries,sizeof(unsigned int),1, pFile);
    fwrite(&hm->entryAllocationStep,sizeof(unsigned int),1, pFile);
     while (i<hm->curNumberOfEntries)
       {
         if  ( (hm->entries[i].keyLength!=0)&&(hm->entries[i].key!=0) )
           {
            fwrite(&hm->entries[i].keyLength,sizeof(unsigned int),1, pFile);
            fwrite(hm->entries[i].key,sizeof(char),hm->entries[i].keyLength, pFile);
           } else
           { fwrite(&zero,sizeof(unsigned int),1, pFile); }

         if  ( (hm->entries[i].payloadLength!=0)&&(hm->entries[i].payload!=0) )
           {
            fwrite(&hm->entries[i].payloadLength,sizeof(unsigned int),1, pFile);
            fwrite(hm->entries[i].payload,sizeof(char),hm->entries[i].payloadLength, pFile);
           } else
           { fwrite(&zero,sizeof(unsigned int),1, pFile); }

         ++i;
       }
     HM_UNLOCK(hm);

     if (fclose(pFile)==0) { result=1; }
    }

  #if HASHMAP_BE_THREAD_SAFE
    pthread_mutex_unlock (&hm->hm_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
  #endif // HASHMAP_BE_THREAD_SAFE

  return result;
}

int hashMap_LoadToFile(struct hashMap * hm,const char * filename)
{
    if (hm==0) { fprintf(stderr,"HashMap: hashMap_LoadToFile cannot load file `%s` without an allocated hashmap structure \n",filename); return 0; }
    fprintf(stderr,RED "hashMap_LoadToFile not implemented ( max entries when called %u ) \n" NORMAL,hm->maxNumberOfEntries);
    return 0;
    /*
    FILE * pFile;
    pFile = fopen (filename,"rb");
    if (pFile!=0)
    {
      //TODO IMPLEMENT!
     fclose (pFile);
     return 0;
    }
    */
    return 0;
}

int hashMap_GetUniqueStringForItem( char * strOut , unsigned int strOutLength , unsigned int itemID, unsigned int totalItems )
{
  fprintf(stderr,RED "hashMap_GetUniqueStringForItem not implemented \n" NORMAL);
  unsigned int charsToUse='z'-'a';
  unsigned int charactersThatAreNeeded = strOutLength / charsToUse;
  unsigned int i=0;

  for (i=0; i<charactersThatAreNeeded; i++)
   {
      //TODO :
   }
  return 0;
}

int hashMap_Test(int useSorting)
{
 struct hashMap * hm = hashMap_Create(5,1,0,useSorting);
 if (hm==0) { return 0; }

 char testName[4]={0};

 unsigned int i=0;
 for (i=0; i<20; i++)
 {
     testName[0]='a'+i;
     testName[1]='a'+i;
     testName[2]='a'+i;
     testName[3]=0;
     hashMap_AddULong(hm,testName,20-i);
 }

 hashMap_PrepareForQueries(hm);

 unsigned int errors=0;
 unsigned long payload=0;
 for (i=0; i<20; i++)
 {
     payload=0;
     testName[0]='a'+i;
     testName[1]='a'+i;
     testName[2]='a'+i;
     testName[3]=0;
     if ( (!hashMap_GetULongPayload(hm,testName,&payload)) || (payload!=20-i) )
     { fprintf(stderr,"Error with %s value %lu instead of %u\n",testName,payload,20-i); ++errors; }
 }

 hashMap_Destroy(hm);
 return (errors==0);
}

