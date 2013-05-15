#include <stdio.h>
#include "../AmmServerlib/hashmap/hashmap.h"


int main(int argc, char *argv[])
{
  printf("Starting HashMap unit test \n");

  struct hashMap * hm = hashMap_Create(1000/*INITIAL*/,1000/*STEP*/,0/*ClearFN*/);
  if (hm == 0) { fprintf(stderr,"Could not create a new hashMap\n"); return 1; }

  char keyStr[128]={0};
  unsigned int i=0;
  while (i<10000)
   {
      sprintf(keyStr,"%u",i);
      hashMap_Add(hm,keyStr,i,0);
      ++i;
   }


  int found=0;
  i=0;
  while (i<10000)
   {
      sprintf(keyStr,"%u",i);
      hashMap_GetIndex(hm,keyStr,&found);
      if (!found)
       {
         fprintf(stderr,"Could not find %u\n",i);
       }
      ++i;
   }



  hashMap_Destroy(hm);
  return 0;
}

