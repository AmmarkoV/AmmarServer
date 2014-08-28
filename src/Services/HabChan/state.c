
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/hashmap/hashmap.h"
#include "../../AmmServerlib/InputParser/InputParser_C.h"


#include "state.h"
#include "board.h"


struct AmmServer_Instance  * default_server=0;
struct AmmServer_Instance  * admin_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct hashMap * boardHashMap =0;
struct hashMap * threadHashMap =0;

struct site ourSite={0};

unsigned int threadIndexPageLength = 0;
char * threadIndexPage = 0;




//This function prepares the content of  form context , ( content )
void * debug_get_callback(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><body><br><br>",rqst->MAXcontentSize);
  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         strcat(rqst->content,"<hr>GET REQUEST dynamically added here : <br><i>"); strcat(rqst->content, rqst->GET_request ); strcat(rqst->content,"</i><hr>");
       }
    }
  strcat(rqst->content,"</body></html>");

  rqst->contentSize=strlen(rqst->content);
  return 0;
}




int loadSite( char * filename )
{
  boardHashMap = hashMap_Create( 100 , 100 , 0 );

  unsigned int numberOfElements=0;
    char what2GetBack[1024]={0};
    AmmServer_ExecuteCommandLine("ls data/board -al | cut -d ' ' -f10 | wc -l ", what2GetBack , 1024 );
    numberOfElements = atoi(what2GetBack);

    ourSite.boards = (struct board * ) malloc(sizeof(struct board) * MAX_BOARDS);
    if (ourSite.boards == 0 ) { fprintf(stderr,"Cannot allocate memory to hold boards , failed to load "); return 0; }

    ourSite.maxNumberOfBoards = MAX_BOARDS;
    ourSite.numberOfBoards = 0;
    strncpy(ourSite.siteName ,filename  ,MAX_STRING_SIZE  );


   threadIndexPage = AmmServer_ReadFileToMemory("data/simple.html", &threadIndexPageLength );

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





    unsigned int i=0;
    for (i=4; i<=numberOfElements; i++)
    {
     AmmServer_ExecuteCommandLineNum("ls data/board -al | cut -d ' ' -f10", what2GetBack , 1024 , i);
     if (strlen(what2GetBack)>1)
         { what2GetBack[strlen(what2GetBack)-1]=0; }

     addBoardToSite( &ourSite , what2GetBack );
    }





  return 1;
}

int unloadSite()
{

  hashMap_Destroy( boardHashMap );

  free(threadIndexPage);
  return 1;
}




int addPostToThread( char * boardName ,  struct thread * newThread ,  struct post * newPost )
{
 return 0;
}












