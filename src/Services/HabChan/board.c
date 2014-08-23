

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "state.h"

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/InputParser/InputParser_C.h"


//This function prepares the content of  stats context , ( stats.content )
void * prepareBoardIndexView(struct AmmServer_DynamicRequest  * rqst)
{
    strcpy(rqst->content,"<html><body>Welcome to Hab Chan<br>");

    unsigned int i=0;
    for (i=0; i<=boardHashMap->curNumberOfEntries; i++)
    {
      char * key = hashMap_GetKeyAtIndex(boardHashMap,i);

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

    strcat(rqst->content,"</body></html>");

   rqst->contentSize=strlen(rqst->content);
   return 0;
}



int loadBoardSettings(char * boardName , struct board * ourBoard)
{
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
                    fprintf(stderr,"TODO : fix thread queue");
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
  if ((targetSite==0)||(boardName==0)) { fprintf(stderr,"Cannot addBoardToSite with incorrect parameters\n"); return 0; }

  if ( targetSite->numberOfBoards+1 >= targetSite->maxNumberOfBoards)
  {
   fprintf(stderr,"Cannot add another board , site is full , please adjust MAX_BOARDS definition in state.h \n ");
   return 0;
  }

  unsigned int thisBoardID = targetSite->numberOfBoards++;

  strncpy( targetSite->boards[thisBoardID].name  , boardName , MAX_STRING_SIZE );

  loadBoardSettings(boardName , &targetSite->boards[thisBoardID] );

  //Update hashmap used to check for sites
  hashMap_Add(boardHashMap,boardName,0,0);



   unsigned int numberOfThreads=0;
   char command[MAX_STRING_SIZE]={0};
   char what2GetBack[1024]={0};

   //ls data/board/b -a | sed 's/ /\n/g' | egrep '^[0-9].*'
   snprintf(command,MAX_STRING_SIZE,"ls data/board/%s/ -al | cut -d ' ' -f10 | wc -l",boardName);
   AmmServer_ExecuteCommandLine(command, what2GetBack , 1024 );
   numberOfThreads = atoi(what2GetBack);

   snprintf(command,MAX_STRING_SIZE,"ls data/board/%s/ -al | cut -d ' ' -f10",boardName);
   unsigned int i=0;
    for (i=4; i<=numberOfThreads; i++)
    {
     AmmServer_ExecuteCommandLineNum(command, what2GetBack , 1024 , i);
     if (strlen(what2GetBack)>1)
         { what2GetBack[strlen(what2GetBack)-1]=0; }

         if (strcmp(what2GetBack,"boardStatus.ini")!=0 )
          {
            addThreadToBoard( boardName , what2GetBack );
          }
    }



 return 0;
}
