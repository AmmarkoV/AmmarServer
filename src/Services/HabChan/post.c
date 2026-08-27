#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include "post.h"

#include "../../InputParser/InputParser_C.h"

int loadPostHeader(char * postHeaderFilename , struct post * ourPost , unsigned int postIndex)
{
   FILE * fp = fopen(postHeaderFilename,"r");
   if (fp == 0 ) { fprintf(stderr,"Cannot open loadPostHeader file %s \n",postHeaderFilename); return 0; }


    char line [LINE_MAX_LENGTH]={0};
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
                   InputParser_GetWord(ipc,1,ourPost->op,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"IMAGENAME")==1)
                {
                   ourPost->hasFile=1;
                   InputParser_GetWord(ipc,1,ourPost->fileOriginalName,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"TIMESTAMP")==1)
                {
                   ourPost->creation.year   =  InputParser_GetWordInt(ipc,1);
                   ourPost->creation.month  =  InputParser_GetWordInt(ipc,2);
                   ourPost->creation.day    =  InputParser_GetWordInt(ipc,3);
                   ourPost->creation.hour   =  InputParser_GetWordInt(ipc,4);
                   ourPost->creation.minute =  InputParser_GetWordInt(ipc,5);
                   ourPost->creation.second =  InputParser_GetWordInt(ipc,6);
                }
              }
          }
       }

    InputParser_Destroy(ipc);
    fclose(fp);

    //header_N only ever stores the name a file was originally uploaded as ( imagename(...) ) , never the
    //actual on-disk filename , so derive it the same deterministic way addPostToThread does when saving..!
    if (ourPost->hasFile) { deriveCachedImageName(ourPost->fileOriginalName,postIndex,ourPost->fileCachedName,MAX_STRING_SIZE); }

    return 1;
}


int loadPostContent(char * postHeaderFilename,struct post * ourPost)
{
 ourPost->message = AmmServer_ReadFileToMemory(postHeaderFilename,&ourPost->messageSize);
 return 1;
}





int savePostHeader(const char * postHeaderFilename , struct post * ourPost)
{
   FILE * fp = fopen(postHeaderFilename,"w");
   if (fp == 0 ) { fprintf(stderr,"Cannot open %s for writing\n",postHeaderFilename); return 0; }

   fprintf(fp,"op(%s)\n",ourPost->op);
   if (ourPost->hasFile) { fprintf(fp,"imagename(%s)\n",ourPost->fileOriginalName); }
   fprintf(fp,"timestamp(%u,%u,%u,%u,%u,%u)\n",
           ourPost->creation.year , ourPost->creation.month , ourPost->creation.day ,
           ourPost->creation.hour , ourPost->creation.minute , ourPost->creation.second);

   fclose(fp);
   return 1;
}


int savePostContent(const char * postFilename , struct post * ourPost)
{
   if (ourPost->message == 0 ) { return 0; }
   return AmmServer_WriteFileFromMemory(postFilename,ourPost->message,strlen(ourPost->message));
}


int loadPosts(struct board * ourBoard , struct thread * ourThread)
{
  int i=0;
  char postHeaderFilename[MAX_STRING_SIZE+1]={0};

  ourThread->maxNumberOfReplies = MAX_POSTS_PER_THREAD;
  ourThread->numberOfReplies = 0;
  ourThread->numberOfImages = 0;
  ourThread->replies = (struct post * ) malloc(sizeof(struct post) * MAX_POSTS_PER_THREAD);

  if (ourThread->replies!=0)
  {
   while (i<ourThread->maxNumberOfReplies)
    {
     snprintf(postHeaderFilename,MAX_STRING_SIZE,"data/board/%s/%s/header_%u" , ourBoard->name ,  ourThread->name , i);
     if (loadPostHeader(postHeaderFilename,&ourThread->replies[i],i) )
     {
       snprintf(postHeaderFilename,MAX_STRING_SIZE,"data/board/%s/%s/post_%u" , ourBoard->name ,  ourThread->name , i);
       loadPostContent(postHeaderFilename,&ourThread->replies[i]);
       ++ourThread->numberOfReplies ;
       if ( strlen(ourThread->replies[i].fileOriginalName) > 0 )
       {
         ++ourThread->numberOfImages;
       }

      fprintf(stderr,"Post %u = %s\n",i,ourThread->replies[i].message);
      fprintf(stderr,"Image %u = %s\n",i,ourThread->replies[i].fileOriginalName);
     }  else
     {
        return 1;
     }

     ++i;
   }
   return 1;
  }

  return 0;
}
