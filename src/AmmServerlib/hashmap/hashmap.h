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
};


struct hashMap
{
  unsigned int maxNumberOfEntries;
  unsigned int curNumberOfEntries;
  unsigned int entryAllocationStep;
  struct hashMapEntry * entries;

  void * clearItemCallbackFunction;
  pthread_mutex_t hm_addLock;
};



struct hashMap * hashMap_Create(unsigned int initialEntries , unsigned int entryAllocationStep,void * clearItemFunction);
void hashMap_Destroy(struct hashMap * hm);
int hashMap_Sort(struct hashMap * hm);
int hashMap_Add(struct hashMap * hm,char * key,void * val,unsigned int valLength);
void * hashMap_Get(struct hashMap * hm,char * key,int * found);
void hashMap_Clear(struct hashMap * hm);
int hashMap_ContainsKey(struct hashMap * hm,char * key);
int hashMap_ContainsValue(struct hashMap * hm,void * val);
int hashMap_GetSize(struct hashMap * hm);

#endif // HASHMAP_H_INCLUDED
