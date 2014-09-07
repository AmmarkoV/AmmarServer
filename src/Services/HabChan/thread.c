
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include "thread.h"


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/InputParser/InputParser_C.h"

void * prepareThreadView(struct AmmServer_DynamicRequest  * rqst)
{
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><body>Welcome to Hab Chan</body></html>" );
   rqst->contentSize=strlen(rqst->content);
   return 0;
}

void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst)
{
  strcpy(rqst->content,"<html><body>Welcome to Hab Chan");

  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         char * boardID = (char *) malloc ( 256 * sizeof(char) );
         if (boardID!=0)
          {
            if ( _GET(default_server,rqst,"board",boardID,256) )
             {
                if ( hashMap_ContainsKey(boardHashMap,boardID) )
                {
                  strcat(rqst->content,"GOT A BOARD !!!  : "); strcat(rqst->content,boardID); strcat(rqst->content," ! ! <br>");

                  // int serveThreadsOfBoard(struct AmmServer_DynamicRequest  * rqst)
                } else
                {
                  strcat(rqst->content,"No BOARD  , denied!!!  <BR> ");
                }
             }
            free(boardID);
          }
       }
    }
   strcat(rqst->content,"</body></html>" );
   rqst->contentSize=strlen(rqst->content);

   rqst->contentSize = threadIndexPageLength;
   strncpy(rqst->content,threadIndexPage,rqst->contentSize);
  return 0;
}



int loadThread(char * threadName , struct board * ourBoard , struct thread * ourThread)
{
   if (ourBoard==0) { fprintf(stderr,"Cannot load thread without an allocated board\n"); return 0; }
   if (ourThread==0) { fprintf(stderr,"Cannot load thread without an allocated thread\n"); return 0; }
   fprintf(stderr,"Loading Thread `%s` to board `%s` \n",threadName,ourBoard->name);

   char filename[LINE_MAX_LENGTH]={0};
   snprintf(filename,LINE_MAX_LENGTH,"data/board/%s/%s/status.ini",ourBoard->name,threadName);
   char line [LINE_MAX_LENGTH]={0};
   //Try and open filename
   FILE * fp = fopen(filename,"r");
   if (fp == 0 ) { fprintf(stderr,"Cannot open loadBoardSettings file %s \n",filename); return 0; }

    //Allocate a token parser
    struct InputParserC * ipc=0;
    ipc = InputParser_Create(LINE_MAX_LENGTH,5);
    if (ipc==0) { fprintf(stderr,"Cannot allocate memory for new loadBoardSettings parser\n"); return 0; }

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
                if (InputParser_WordCompareNoCase2(ipc,0,(char*)"OP")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->op,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCase2(ipc,0,(char*)"TITLE")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->title,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCase2(ipc,0,(char*)"LASTREPLY")==1)
                {
                   ourThread->lastReply.year   =  InputParser_GetWordInt(ipc,1);
                   ourThread->lastReply.month  =  InputParser_GetWordInt(ipc,2);
                   ourThread->lastReply.day    =  InputParser_GetWordInt(ipc,3);
                   ourThread->lastReply.hour   =  InputParser_GetWordInt(ipc,4);
                   ourThread->lastReply.minute =  InputParser_GetWordInt(ipc,5);
                   ourThread->lastReply.second =  InputParser_GetWordInt(ipc,6);
                } else
                if (InputParser_WordCompareNoCase2(ipc,0,(char*)"NUMBEROFREPLIES")==1)
                {
                   ourThread->numberOfReplies =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCase2(ipc,0,(char*)"NUMBEROFIMAGES")==1)
                {
                   ourThread->numberOfImages =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCase2(ipc,0,(char*)"STICKY")==1)
                {
                   ourThread->sticky =  InputParser_GetWordInt(ipc,1);
                }
              }
          }
       }

    InputParser_Destroy(ipc);
    fclose(fp);
    return 1;
}


int addThreadToBoard( const char * boardName , const char * threadName )
{
  fprintf(stderr,"Adding Thread `%s` to board `%s` \n",threadName,boardName);

  if ( ! hashMap_ContainsKey(boardHashMap,boardName) )
  {
    fprintf(stderr,"Could not find board name `%s` , Cannot create a thread in non existing board\n", boardName);
    return 0;
  }

  unsigned long threadID=0;
  unsigned long boardID=0;
  if ( hashMap_FindIndex(boardHashMap,boardName,&boardID) )
  {
   loadThread(threadName , &ourSite.boards[boardID] , &ourSite.boards[boardID].threads[threadID]);
   hashMap_Add(threadHashMap,threadName,0,0);
   return 1;
  }

 return 0;
}



