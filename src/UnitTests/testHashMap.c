#include <stdio.h>
#include "../AmmServerlib/hashmap/hashmap.h"


int main(int argc, char *argv[])
{
  printf("Hello World\n");

  struct hashMap * hm = hashMap_Create(1000/*INITIAL*/,1000/*STEP*/,0/*ClearFN*/);

  char keyStr[128];
  unsigned int i=0; 
  while (i<10000)
   {
      sprintf(keyStr,"%u",i);
      hashMap_Add(hm,keyStr,i,0);
   }
  hashMap_Destroy(hm);
  return 0;
} 

