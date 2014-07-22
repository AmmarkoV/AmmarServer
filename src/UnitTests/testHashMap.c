#include <stdio.h>
#include "../AmmServerlib/hashmap/hashmap.h"


int main(int argc, char *argv[])
{
  printf("Starting HashMap unit test \n");

  struct hashMap * hm = hashMap_Create(1000/*INITIAL*/,1000/*STEP*/,0/*ClearFN*/);
  if (hm == 0) { fprintf(stderr,"Could not create a new hashMap\n"); return 1; }

  char keyStr[128]={0};
  unsigned long i=0;
  while (i<10000)
   {
      snprintf(keyStr,128,"%lu",i);
      hashMap_Add(hm,keyStr,(void*) i,0);
      ++i;
   }


  unsigned long index=0;
  i=0;
  while (i<10000)
   {
      snprintf(keyStr,128,"%lu",i);
      if (!hashMap_FindIndex(hm,keyStr,&index))
       {
         fprintf(stderr,"Could not find %lu\n",i);
       }
      ++i;
   }



  hashMap_Destroy(hm);
  return 0;
}

