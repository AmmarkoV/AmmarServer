#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <dirent.h>

#include "state.h"
#include "thread.h"

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../InputParser/InputParser_C.h"


//This function prepares the content of  stats context , ( stats.content )
void * prepareBoardIndexView(struct AmmServer_DynamicRequest  * rqst)
{
    strcpy(rqst->content,"<html><title>HabChan Board Index</title><body>Welcome to Hab Chan , powered by <a href=\"https://github.com/AmmarkoV/AmmarServer/\">AmmarServer</a> , final destination <br>");

    if (boardHashMap->curNumberOfEntries==0)
    {
      strcat(rqst->content," <br><br><br> <center> <h2>No boards exist </h2> </center> ");
    } else
    {
     unsigned int i=0;
     for (i=0; i<=boardHashMap->curNumberOfEntries; i++)
     {
      const char * key = hashMap_GetKeyAtIndex(boardHashMap,i);
      if (key!=0)
      {
       strcat(rqst->content," <a href=\"threadIndexView.html?board=");
       strcat(rqst->content,key);
       strcat(rqst->content,"\">");
       strcat(rqst->content," Board ");
       strcat(rqst->content,key);
       strcat(rqst->content," </a> <br>");
      }
     }
    }

    strcat(rqst->content,"</body></html>");

   rqst->contentSize=strlen(rqst->content);
   return 0;
}



int loadBoardSettings(char * boardName , struct board * ourBoard)
{
   if (ourBoard==0) { fprintf(stderr,"Cannot load board without an allocated board\n"); return 0; }

   char filename[LINE_MAX_LENGTH]={0};
   snprintf(filename,LINE_MAX_LENGTH,"data/board/%s/boardStatus.ini",boardName);
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
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"MAXTHREADS",10)==1)
                {
                   ourBoard->maxThreads =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"CURRENTTHREADS",14)==1)
                {
                   ourBoard->currentThreads =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"ACTIVE",6)==1)
                {
                   ourBoard->active =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"ACTIVEUSERS",11)==1)
                {
                   ourBoard->currentUsers =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"THREADQUEUE",11)==1)
                {
                    fprintf(stderr,"TODO : fix thread queue\n");
                }
              }
          }
       }

    InputParser_Destroy(ipc);
    fclose(fp);
    return 1;
}






int addBoardToSite( struct site * targetSite , char * boardName )
{
  fprintf(stderr,"Adding Board `%s` to site `%s` \n",boardName,targetSite->siteName);

  if ((targetSite==0)||(boardName==0)) { fprintf(stderr,"Cannot addBoardToSite with incorrect parameters\n"); return 0; }

  if ( targetSite->numberOfBoards+1 >= targetSite->maxNumberOfBoards)
  {
   fprintf(stderr,"Cannot add another board , site is full , please adjust MAX_BOARDS definition in state.h \n ");
   return 0;
  }

  //Update hashmap used to check for sites
  hashMap_Add(boardHashMap,boardName,0,0);

  unsigned long boardID=0;
  if ( hashMap_FindIndex(boardHashMap,boardName,&boardID) )
  {
    targetSite->numberOfBoards++;
    strncpy( targetSite->boards[boardID].name  , boardName , MAX_STRING_SIZE );

    loadBoardSettings(boardName , &targetSite->boards[boardID] );

    if (ourSite.boards[boardID].threads == 0 )
    {
        ourSite.boards[boardID].threads = (struct thread * ) malloc(sizeof(struct thread) * MAX_THREADS_PER_BOARD );
    }
   ourSite.boards[boardID].currentThreads=0;
   ourSite.boards[boardID].maxThreads=MAX_THREADS_PER_BOARD;
  }


   unsigned int numberOfThreads=0;
   char command[MAX_STRING_SIZE]={0};

   snprintf(command,MAX_STRING_SIZE,"data/board/%s/",boardName);

   DIR *dp;
   struct dirent *ep;
   dp = opendir (command);
   if (dp != NULL)
    {
      while (ep = readdir (dp))
       {
         if (strcmp(ep->d_name,"boardStatus.ini")==0)  { } else
         if (strcmp(ep->d_name,".")==0)                { } else
         if (strcmp(ep->d_name,"..")==0)               { } else
            {
              //fprintf(stderr,"Adding thread %s \n",ep->d_name);
              addThreadToBoard( boardName , ep->d_name );
              ++numberOfThreads;
            }
       }
      closedir (dp);
    } else
    {
     fprintf(stderr,"Cannot open directory to list channels \n");
    }



 return 0;
}
