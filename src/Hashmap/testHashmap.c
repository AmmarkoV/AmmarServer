//
//gcc testHashmap.c hashmap.c -o testHashmap -lpthread
//written by Ammar Qammaz
//


#include <stdio.h>     /* files */
#include <stdlib.h>     /* qsort */
#include <string.h>     /* memset */
#include <pthread.h>


#include "hashmap.h"

static unsigned int failures=0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"FAIL %s:%d : %s\n",__FILE__,__LINE__,#cond); ++failures; } } while(0)

static int findULong(struct hashMap * hm,const char * key,unsigned long hint,unsigned long * out)
{
  *out=hint;
  return hashMap_GetULongPayload(hm,key,out);
}

//"Ez" and "FY" have the same djb2 hash , a hash-only match used to return the wrong entry
static void test_hashCollision(int useSorting)
{
  CHECK(hashFunction("Ez")==hashFunction("FY"));
  struct hashMap * hm = hashMap_Create(4,4,0,useSorting);
  unsigned long pl=0, idx=0;
  CHECK(hashMap_AddULong(hm,"Ez",111));
  CHECK(!hashMap_ContainsKey(hm,"FY"));
  CHECK(!findULong(hm,"FY",0,&pl));
  CHECK(!hashMap_FindIndex(hm,"FY",&idx));
  CHECK(hashMap_AddULong(hm,"FY",222));
  CHECK(findULong(hm,"Ez",0,&pl) && pl==111);
  CHECK(findULong(hm,"Ez",1,&pl) && pl==111);
  CHECK(findULong(hm,"FY",0,&pl) && pl==222);
  CHECK(findULong(hm,"FY",1,&pl) && pl==222);
  hashMap_Destroy(hm);
}

//Big sorted maps go through bsearch , which used to return a stale pre-sort index ( and miss keys after RemoveKey )
static void test_sortedLookupAfterRemove()
{
  struct hashMap * hm = hashMap_Create(8,8,0,1);
  char key[32]; unsigned long i,idx,pl;
  for (i=0; i<100; i++) { snprintf(key,sizeof(key),"key%lu",i); CHECK(hashMap_AddULong(hm,key,i)); }
  CHECK(hashMap_RemoveKey(hm,"key3"));
  CHECK(hashMap_RemoveKey(hm,"key0"));
  CHECK(hashMap_RemoveKey(hm,"key99"));
  CHECK(!hashMap_RemoveKey(hm,"key3"));
  CHECK(hashMap_GetCurrentNumberOfEntries(hm)==97);
  for (i=0; i<100; i++)
  {
    snprintf(key,sizeof(key),"key%lu",i);
    int removed = (i==0)||(i==3)||(i==99);
    idx=1000000; //Out of range hint , forces the real search
    int found = hashMap_FindIndex(hm,key,&idx);
    CHECK(found == !removed);
    if (found) { CHECK(strcmp(hashMap_GetKeyAtIndex(hm,idx),key)==0); }
    CHECK(findULong(hm,key,1000000,&pl) == !removed);
    if (!removed) { CHECK(pl==i); }
  }
  CHECK(hm->isSorted); //The lookups above re-sorted after the removals , so they went through bsearch
  hashMap_Destroy(hm);
}

//entryAllocationStep 0 used to grow by nothing and write past the end of entries
static void test_zeroAllocationStep()
{
  struct hashMap * hm = hashMap_Create(1,0,0,0);
  CHECK(hm!=0);
  char key[32]; unsigned long i,pl;
  for (i=0; i<10; i++) { snprintf(key,sizeof(key),"k%lu",i); CHECK(hashMap_AddULong(hm,key,i)); }
  for (i=0; i<10; i++) { snprintf(key,sizeof(key),"k%lu",i); CHECK(findULong(hm,key,0,&pl) && pl==i); }
  hashMap_Destroy(hm);
  CHECK(hashMap_Create(0,1,0,0)==0); //No initial space is a clean failure
}

static void test_nullAndEmptyKeys()
{
  struct hashMap * hm = hashMap_Create(4,4,0,0);
  unsigned long idx=0,pl=0; void * p=0;
  CHECK(!hashMap_Add(hm,0,0,0));
  CHECK(!hashMap_Add(hm,"x",0,5)); //Copying 5 bytes from a null payload
  CHECK(hashMap_GetCurrentNumberOfEntries(hm)==0);
  CHECK(hashMap_AddULong(hm,"",7));
  CHECK(!hashMap_ContainsKey(hm,0));
  CHECK(!hashMap_FindIndex(hm,0,&idx));
  CHECK(!hashMap_FindIndexSerial(hm,0,&idx));
  CHECK(!hashMap_RemoveKey(hm,0));
  CHECK(!hashMap_GetPayload(hm,0,&p));
  CHECK(!hashMap_GetULongPayload(hm,0,&pl));
  CHECK(hashMap_ContainsKey(hm,""));
  CHECK(findULong(hm,"",0,&pl) && pl==7);
  hashMap_Destroy(hm);

  //Null map
  CHECK(!hashMap_Sort(0));
  CHECK(!hashMap_PrepareForQueries(0));
  CHECK(!hashMap_Print(0,"null"));
  CHECK(!hashMap_FindIndex(0,"a",&idx));
  CHECK(!hashMap_FindIndexSerial(0,"a",&idx));
  CHECK(!hashMap_Add(0,"a",0,0));
  CHECK(!hashMap_RemoveKey(0,"a"));
  CHECK(!hashmap_SwapRecords(0,0,0));
  hashMap_Clear(0);
  hashMap_Destroy(0);
}

//hashMap_GetPayload used to take its output by value and never return anything
static void test_getPayload()
{
  struct hashMap * hm = hashMap_Create(4,4,0,0);
  int marker=0;
  CHECK(hashMap_Add(hm,"copy","hello",6));
  CHECK(hashMap_Add(hm,"ptr",&marker,0));
  void * p=0;
  CHECK(hashMap_GetPayload(hm,"copy",&p) && p!=0 && strcmp((char*)p,"hello")==0);
  CHECK(hashMap_GetPayload(hm,"ptr",&p) && p==&marker);
  p=(void*)&p;
  CHECK(!hashMap_GetPayload(hm,"missing",&p) && p==(void*)&p);
  hashMap_Destroy(hm);
}

//Swapping must invalidate the sorted flag or the next bsearch runs on unsorted data
static void test_swapRecords()
{
  struct hashMap * hm = hashMap_Create(64,64,0,1);
  char key[32]; unsigned long i,pl;
  for (i=0; i<40; i++) { snprintf(key,sizeof(key),"s%lu",i); CHECK(hashMap_AddULong(hm,key,i)); }
  CHECK(hashMap_PrepareForQueries(hm));
  CHECK(hashmap_SwapRecords(hm,0,39));
  CHECK(!hashmap_SwapRecords(hm,0,40));
  for (i=0; i<40; i++) { snprintf(key,sizeof(key),"s%lu",i); CHECK(findULong(hm,key,1000000,&pl) && pl==i); }
  hashMap_Destroy(hm);
}

static void test_clearAndReuse()
{
  struct hashMap * hm = hashMap_Create(2,2,0,0);
  unsigned long pl=0;
  CHECK(hashMap_Add(hm,"a","AAA",4));
  CHECK(hashMap_Add(hm,"b","BBB",4));
  CHECK(findULong(hm,"a",0,&pl)); CHECK(findULong(hm,"a",0,&pl));
  hashMap_Clear(hm);
  CHECK(hashMap_GetCurrentNumberOfEntries(hm)==0);
  CHECK(!hashMap_ContainsKey(hm,"a"));
  CHECK(hashMap_AddULong(hm,"c",3));
  CHECK(hm->entries[0].hits==0);
  CHECK(findULong(hm,"c",0,&pl) && pl==3);
  hashMap_Destroy(hm);
}

//Lookups used to qsort/read the entries array without the lock while other threads realloc'ed it
#define THREADS 4
#define KEYS_PER_THREAD 300
struct threadArg { struct hashMap * hm; unsigned long id; unsigned int errors; };

static void * concurrentWorker(void * ptr)
{
  struct threadArg * a = (struct threadArg *) ptr;
  char key[32]; unsigned long i,j,pl;
  for (i=0; i<KEYS_PER_THREAD; i++)
  {
    snprintf(key,sizeof(key),"t%lu_%lu",a->id,i);
    if (!hashMap_AddULong(a->hm,key,a->id*100000+i)) { ++a->errors; }
    for (j=0; j<=i; j+=7) //Look back at our own keys while the other threads keep growing the map
    {
      snprintf(key,sizeof(key),"t%lu_%lu",a->id,j);
      pl=j;
      if ( (!hashMap_GetULongPayload(a->hm,key,&pl)) || (pl!=a->id*100000+j) ) { ++a->errors; }
    }
  }
  return 0;
}

static void test_concurrentAddAndFind(int useSorting)
{
  struct hashMap * hm = hashMap_Create(4,4,0,useSorting);
  pthread_t threads[THREADS];
  struct threadArg args[THREADS];
  unsigned long t;
  for (t=0; t<THREADS; t++) { args[t].hm=hm; args[t].id=t; args[t].errors=0; pthread_create(&threads[t],0,concurrentWorker,&args[t]); }
  for (t=0; t<THREADS; t++) { pthread_join(threads[t],0); CHECK(args[t].errors==0); }
  CHECK(hashMap_GetCurrentNumberOfEntries(hm)==THREADS*KEYS_PER_THREAD);
  hashMap_Destroy(hm);
}

int main()
{
  test_hashCollision(0);
  test_hashCollision(1);
  test_sortedLookupAfterRemove();
  test_zeroAllocationStep();
  test_nullAndEmptyKeys();
  test_getPayload();
  test_swapRecords();
  test_clearAndReuse();
  test_concurrentAddAndFind(0);
  test_concurrentAddAndFind(1);
  CHECK(hashMap_Test(0));
  CHECK(hashMap_Test(1));

  if (failures) { fprintf(stderr,"HashMap tests : %u failures\n",failures); return 1; }
  fprintf(stderr,"HashMap tests : all passed\n");
  return 0;
}
