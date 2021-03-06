
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../Hashmap/hashmap.h"
#include "../../InputParser/InputParser_C.h"


#include "state.h"
#include "board.h"


struct AmmServer_Instance  * default_server=0;
struct AmmServer_Instance  * admin_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct hashMap * boardHashMap =0;
struct hashMap * threadHashMap =0;

struct site ourSite={0};

struct AmmServer_MemoryHandler * threadIndexPage = 0;




int loadSite( char * filename )
{
  boardHashMap = hashMap_Create( 100 , 100 , 0 , 1 /*We should have sorting enabled..!*/ );
  threadHashMap = hashMap_Create( 10000 , 1000 , 0 , 1 /*We should have sorting enabled..!*/ );

    unsigned int numberOfElements=0;
    char what2GetBack[1024]={0};
    AmmServer_ExecuteCommandLine("ls data/board -al | cut -d ' ' -f10 | wc -l ", what2GetBack , 1024 );
    numberOfElements = atoi(what2GetBack);

    ourSite.boards = (struct board * ) malloc(sizeof(struct board) * MAX_BOARDS);
    if (ourSite.boards == 0 ) { fprintf(stderr,"Cannot allocate memory to hold boards , failed to load "); return 0; }

    ourSite.maxNumberOfBoards = MAX_BOARDS;
    ourSite.numberOfBoards = 0;
    strncpy(ourSite.siteName ,filename  ,MAX_STRING_SIZE  );


    threadIndexPage      = AmmServer_ReadFileToMemoryHandler("data/simple.html");

   //------------------------------------------------------

   char line [LINE_MAX_LENGTH]={0};
   //Try and open filename
   FILE * fp = fopen(filename,"r");
   if (fp == 0 ) { fprintf(stderr,"Cannot open trajectory stream %s \n",filename); return 0; }

    //Allocate a token parser
    struct InputParserC * ipc=0;
    ipc = InputParser_Create(LINE_MAX_LENGTH,5);
    if (ipc==0) { fprintf(stderr,"Cannot allocate memory for new stream\n"); return 0; }

    while (!feof(fp))
       {
         //We get a new line out of the file
         unsigned int readOpResult = (fgets(line,LINE_MAX_LENGTH,fp)!=0);
         if ( readOpResult != 0 )
           {
             //We tokenize it
             unsigned int words_count = InputParser_SeperateWords(ipc,line,0);
             if ( words_count > 0 )
              {
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"SITENAME",8)==1)
                {
                     InputParser_GetWord(ipc,1, ourSite.siteName , MAX_STRING_SIZE );
                } else
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"SITEDESCRIPTION",8)==1)
                {
                     InputParser_GetWord(ipc,1, ourSite.siteDescription , MAX_STRING_SIZE );
                }
              }
          }
       }

    InputParser_Destroy(ipc);
    fclose(fp);




   DIR *dp;
   struct dirent *ep;
   dp = opendir ("data/board");
   if (dp != NULL)
    {
      while (ep = readdir (dp))
       {
         if (strcmp(ep->d_name,".")==0)  { } else
         if (strcmp(ep->d_name,"..")==0) { } else
            {
              //fprintf(stderr,"Adding board %s \n",ep->d_name);
              addBoardToSite( &ourSite , ep->d_name );
            }
       }
      closedir (dp);
    } else
    {
     fprintf(stderr,"Cannot open directory to list channels \n");
    }



  return 1;
}

int unloadSite()
{

  hashMap_Destroy( boardHashMap );
  hashMap_Destroy( threadHashMap );

  free(threadIndexPage);
  return 1;
}




int addPostToThread( char * boardName ,  struct thread * newThread ,  struct post * newPost )
{
 return 0;
}












