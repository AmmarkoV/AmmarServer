
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/hashmap/hashmap.h"

#include "state.h"


struct AmmServer_Instance  * default_server=0;
struct AmmServer_Instance  * admin_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct hashMap * boardHashMap =0;
struct hashMap * threadHashMap =0;


unsigned int threadIndexPageLength = 0;
char * threadIndexPage = 0;


int AmmServer_ExecuteCommandLineNum(char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize,unsigned int lineNumber)
{
 /* Open the command for reading. */
 FILE * fp = popen(command, "r");
 if (fp == 0 )
       {
         fprintf(stderr,"Failed to run command (%s) \n",command);
         return 0;
       }

 /* Read the output a line at a time - output it. */
  unsigned int i=0;
  while (fgets(what2GetBack, what2GetBackMaxSize , fp) != 0)
    {
        ++i;
        if (lineNumber==i) { break; }
    }
  /* close */
  pclose(fp);
  return 1;
}

int loadBoards()
{
  boardHashMap = hashMap_Create( 100 , 100 , 0 );


    unsigned int numberOfElements=0;
    char what2GetBack[1024]={0};
    AmmServer_ExecuteCommandLine("ls data/board -al | cut -d ' ' -f10 | wc -l ", what2GetBack , 1024 );
    numberOfElements = atoi(what2GetBack);

    unsigned int i=0;
    for (i=4; i<=numberOfElements; i++)
    {
     AmmServer_ExecuteCommandLineNum("ls data/board -al | cut -d ' ' -f10", what2GetBack , 1024 , i);
     if (strlen(what2GetBack)>1)
      { what2GetBack[strlen(what2GetBack)-1]=0; }
     hashMap_Add(boardHashMap,what2GetBack,0,0);
    }

   threadIndexPage = AmmServer_ReadFileToMemory("data/simple.html", &threadIndexPageLength );

}

int unloadBoards()
{

  hashMap_Destroy( boardHashMap );

  free(threadIndexPage);

}
