#ifndef HASHMAP_H_INCLUDED
#define HASHMAP_H_INCLUDED

unsigned long hashFunction(char *str);


struct hashMapEntry
{
  unsigned int keyLength;
  char * key;
  unsigned long keyHash;
  void * payload;
};


struct hashMap
{
  unsigned int maxNumberOfEntries;
  unsigned int curNumberOfEntries;
  unsigned int entryAllocationStep;
  struct hashMapEntry * entries;
};



struct hashMap * hashMap_Create(unsigned int initialEntries , unsigned int entryAllocationStep);
struct hashMap * hashMap_Clone(struct hashMap * hm);
void hashMap_Destroy(struct hashMap * hm);
int hashMap_Add(struct hashMap * hm,char * key,void * val);
void * hashMap_Get(struct hashMap * hm,char * key,int * found);
void hashMap_Clear(struct hashMap * hm);
int hashMap_ContainsKey(struct hashMap * hm,char * key);
int hashMap_ContainsValue(struct hashMap * hm,void * val);
int hashMap_GetSize(struct hashMap * hm);

#endif // HASHMAP_H_INCLUDED
