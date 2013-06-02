#ifndef HASHMAP_H_INCLUDED
#define HASHMAP_H_INCLUDED



#include <pthread.h>

unsigned long hashFunction(char *str);


struct hashMapEntry
{
  unsigned long keyHash;
  unsigned int keyLength;
  char * key;
  unsigned int payloadLength;
  void * payload;
  unsigned int hits;
};


struct hashMap
{
  unsigned int maxNumberOfEntries;
  unsigned int curNumberOfEntries;
  unsigned int entryAllocationStep;
  struct hashMapEntry * entries;

  void * clearItemCallbackFunction;
  pthread_mutex_t hm_addLock;
  pthread_mutex_t hm_fileLock;
};



struct hashMap * hashMap_Create(unsigned int initialEntries , unsigned int entryAllocationStep,void * clearItemFunction);
void hashMap_Destroy(struct hashMap * hm);
int hashMap_Sort(struct hashMap * hm);
int hashMap_Add(struct hashMap * hm,char * key,void * val,unsigned int valLength);
int hashMap_AddULong(struct hashMap * hm,char * key,unsigned long val);
int hashMap_FindIndex(struct hashMap * hm,char * key,unsigned long * index);
int hashmap_SwapRecords(struct hashMap * hm , unsigned int index1,unsigned int index2);
char * hashMap_GetKeyAtIndex(struct hashMap * hm,unsigned int index);
unsigned long hashMap_GetHashAtIndex(struct hashMap * hm,unsigned int index);
int hashMap_GetPayload(struct hashMap * hm,char * key,void * payload);
int hashMap_GetULongPayload(struct hashMap * hm,char * key,unsigned long * payload);
void hashMap_Clear(struct hashMap * hm);
int hashMap_ContainsKey(struct hashMap * hm,char * key);
int hashMap_ContainsValue(struct hashMap * hm,void * val);
int hashMap_GetMaxNumberOfEntries(struct hashMap * hm);
int hashMap_GetCurrentNumberOfEntries(struct hashMap * hm);

int hashMap_LoadToFile(struct hashMap * hm,char * filename);
int hashMap_SaveToFile(struct hashMap * hm,char * filename);

#endif // HASHMAP_H_INCLUDED
