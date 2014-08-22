
#include <stdio.h>
#include <stdlib.h>
#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/hashmap/hashmap.h"

#include "state.h"


struct AmmServer_Instance  * default_server=0;
struct AmmServer_Instance  * admin_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct hashMap * boardHashMap =0;


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
     hashMap_Add(boardHashMap,what2GetBack,0,0);
    }


}

int unloadBoards()
{

  hashMap_Destroy( boardHashMap );

}
