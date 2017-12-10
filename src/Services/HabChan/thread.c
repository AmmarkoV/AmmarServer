
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include "thread.h"


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../InputParser/InputParser_C.h"

void * prepareThreadView(struct AmmServer_DynamicRequest  * rqst)
{
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><body>Welcome to Hab Chan</body></html>" );
   rqst->contentSize=strlen(rqst->content);
   return 0;
}


char * mallocHTMLListOfThreadsOfBoard(const char * boardName,unsigned int * htmlLength)
{
    char * buffer=(char*) malloc(sizeof(char) * 100000);
    if (buffer==0) { return 0; }
    buffer[0]=0;

    unsigned long boardIndex = 0;
    if ( hashMap_FindIndex(boardHashMap,boardName,&boardIndex) )
    {
        unsigned int threadID=0;
        for (threadID=0; threadID<ourSite.boards[boardIndex].currentThreads; threadID++)
           {

               strcat(buffer,"\
               <div style=\"background-color:#ffffee;\">\
                <br>\
                  <div>\
                    <table width=\"400\" style=\"background-color:#f0e0d6;\">\
                       <tr>\
                        <td colspan=2> Header Here </td>\
                       </tr>\
                       <tr>\
                        <td> <img src=\"board/b/1/image_1.jpg\" height=\"100\"> </td> <td> payload here </td>\
                       </tr>\
                    </table>\
                 </div><br><hr><br>");

              unsigned int postID=0;
              for (postID=0; postID<4; postID++)
              {
               strcat(buffer,"\
                 <div style=\"background-color:#f0e0d6;\">\
                    <table>\
                       <tr>\
                         <td colspan=2> Header Here </td>\
                         <td> image here </td> <td> payload here </td>\
                       </tr>\
                    </table>\
                 </div><br>");
              }
             strcat(buffer,"</div>");
           }
    }
  return buffer;
}

void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst)
{
 fprintf(stderr,"prepareThreadIndexView  \n");
  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         char boardID[257]={0};
         if ( _GET(default_server,rqst,"board",boardID,256) )
             {
                fprintf(stderr,"board: %s \n",boardID);
                if ( hashMap_ContainsKey(boardHashMap,boardID) )
                {
                  struct AmmServer_MemoryHandler * threadIndexPageWithContents = AmmServer_CopyMemoryHandler(threadIndexPage);

                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!BOARDNAME!",boardID);
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!THREADID!","new");

                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,2,"!list channels here!","todo: add channel list..");
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!Say Which Board we are in here!",boardID);
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!HabChan Dynamic Title here!","HabChan @ AmmarServer");



                  unsigned int threadsHTMLLength=0;
                  char * threadsHTML = mallocHTMLListOfThreadsOfBoard(boardID,&threadsHTMLLength);
                  if (threadsHTML!=0)
                   {
                    AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"<!--THREAD_CONTENT-->",threadsHTML);
                    fprintf(stderr,"content: %s \n",threadsHTML);
                    free(threadsHTML);
                   }



                   memcpy (rqst->content , threadIndexPageWithContents ->content , threadIndexPageWithContents ->contentCurrentLength );
                   rqst->contentSize=threadIndexPageWithContents ->contentCurrentLength ;
                   AmmServer_FreeMemoryHandler(&threadIndexPageWithContents);
                   return 0;
                } else
                {
                  strcat(rqst->content,"No BOARD  , denied!!!  <BR> ");
                }
             }
       }
    }

   strcpy(rqst->content,"<html>ERROR</html>");
   rqst->contentSize=strlen(rqst->content);
  return 0;
}



int loadThread(const char * threadName , struct board * ourBoard , struct thread * ourThread)
{
   if (ourBoard==0)  { fprintf(stderr,"Cannot load thread without an allocated board\n");  return 0; }
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
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"OP")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->op,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"TITLE")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->title,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"LASTREPLY")==1)
                {
                   ourThread->lastReply.year   =  InputParser_GetWordInt(ipc,1);
                   ourThread->lastReply.month  =  InputParser_GetWordInt(ipc,2);
                   ourThread->lastReply.day    =  InputParser_GetWordInt(ipc,3);
                   ourThread->lastReply.hour   =  InputParser_GetWordInt(ipc,4);
                   ourThread->lastReply.minute =  InputParser_GetWordInt(ipc,5);
                   ourThread->lastReply.second =  InputParser_GetWordInt(ipc,6);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"NUMBEROFREPLIES")==1)
                {
                   ourThread->numberOfReplies =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"NUMBEROFIMAGES")==1)
                {
                   ourThread->numberOfImages =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"STICKY")==1)
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
   if (hashMap_Add(threadHashMap,threadName,0,0))
   {
    if ( hashMap_FindIndex(threadHashMap,threadName,&threadID) )
    {
     loadThread(threadName , &ourSite.boards[boardID] , &ourSite.boards[boardID].threads[threadID]);
     return 1;
    }else
    { fprintf(stderr,"hashMap_FindIndex(thread) failure\n"); }
   } else
   { fprintf(stderr,"hashMap_Add failure\n"); }
  }else
  { fprintf(stderr,"hashMap_FindIndex(board) failure\n"); }

 fprintf(stderr,"Failed to add thread %s to board %s\n", threadName,boardName);
 return 0;
}



