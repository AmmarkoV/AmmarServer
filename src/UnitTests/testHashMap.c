#include <stdio.h>
#include "../AmmServerlib/hashmap/hashmap.h"


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */


int doHashMapTest()
{
  unsigned int errors=0;
  printf("Starting HashMap unit test \n");
  struct hashMap * hm = hashMap_Create(1000/*INITIAL*/,1000/*STEP*/,0/*ClearFN*/);
  if (hm == 0) { fprintf(stderr,RED "Could not create a new hashMap\n" NORMAL); return 0; }

  char keyStr[128]={0};
  unsigned long i=0;
  while (i<10000)
   {
      snprintf(keyStr,128,"%lu",i);
      if (! hashMap_Add(hm,keyStr,(void*) i,0) ) { ++errors; }
      ++i;
   }


  unsigned long index=0;
  i=0;
  while (i<10000)
   {
      snprintf(keyStr,128,"%lu",i);
      if (!hashMap_FindIndex(hm,keyStr,&index))
       {
         fprintf(stderr,RED "Could not find %lu\n" NORMAL,i);
         ++errors;
       }
      ++i;
   }
  hashMap_Destroy(hm);

 return (errors==0);
}


int doInjectTest()
{

}




int main(int argc, char *argv[])
{
  if (!doHashMapTest())   { fprintf(stderr,GREEN "Hash Map Test .. Sucess\n" NORMAL); }  else { fprintf(stderr,RED "Hash Map Test .. Failed\n" NORMAL); }
  if (!doInjectTest())   { fprintf(stderr,GREEN "Injection Test .. Sucess\n" NORMAL); }  else { fprintf(stderr,RED "Injection Test .. Failed\n" NORMAL); }

















  return 0;
}

