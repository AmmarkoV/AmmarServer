
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include "board.h"
#include "thread.h"

#include "../../AmmServerlib/AmmServerlib.h"

void * processPostReceiver(struct AmmServer_DynamicRequest  * rqst)
{
   //board=b&replythread=new&name=tettee&s=ttete&message=etetete&imagefile=&postpassword=tetetet
   unsigned int succesfulAddition=0;

   char boardName[MAX_STRING_SIZE]={0};
   char replyThread[MAX_STRING_SIZE]={0};
   char resultThreadName[MAX_STRING_SIZE]={0};

   struct post newPost={0};
   newPost.message =  (char *) malloc ( MAX_STRING_SIZE * sizeof(char) );
   if (newPost.message!=0) { newPost.message[0]=0; }

   _POSTcpy(rqst,"board",boardName,MAX_STRING_SIZE);
   _POSTcpy(rqst,"replythread",replyThread,MAX_STRING_SIZE);

   _POSTcpy(rqst,"name",newPost.op,MAX_STRING_SIZE);
   if ( strlen(newPost.op)==0 ) { snprintf(newPost.op,MAX_STRING_SIZE,"Anonymous"); }
   _POSTcpy(rqst,"postpassword",newPost.password,MAX_STRING_SIZE);

   char subject[MAX_STRING_SIZE]={0};
   _POSTcpy(rqst,"s",subject,MAX_STRING_SIZE);

   if (newPost.message!=0) { _POSTcpy(rqst,"message",newPost.message,MAX_STRING_SIZE); }

   const char * fileBytes = 0;
   unsigned int fileBytesSize = 0;
   unsigned int fileNameSize = 0;
   const char * uploadedFileName = _FILES(rqst,"imagefile",FILENAME,&fileNameSize);
   if ( (uploadedFileName!=0) && (strlen(uploadedFileName)>0) )
   {
     unsigned int candidateSize=0;
     const char * candidateBytes = _FILES(rqst,"imagefile",VALUE,&candidateSize);
     if ( (candidateBytes!=0) && (candidateSize>0) )
     {
       fileBytes = candidateBytes;
       fileBytesSize = candidateSize;
       newPost.hasFile = 1;
       snprintf(newPost.fileOriginalName,MAX_STRING_SIZE,"%s",uploadedFileName);
     }
   }

   if ( (newPost.message!=0) && (strlen(newPost.message)>0) && hashMap_ContainsKey(boardHashMap,boardName) )
   {
     if ( (strlen(replyThread)==0) || (strcmp(replyThread,"new")==0) )
     {
       //A brand new thread , newPost becomes reply #0 ( the OP post ) of it
       if ( createThread(boardName,newPost.op,subject,newPost.password,&newPost,fileBytes,fileBytesSize,resultThreadName,MAX_STRING_SIZE) )
       {
         succesfulAddition=1;
       }
     } else
     {
       //A reply to an existing thread , make sure the thread we were pointed at really belongs to this board
       unsigned long boardIndex=0;
       unsigned long encoded=0;
       if ( hashMap_GetULongPayload(boardHashMap,boardName,&boardIndex) &&
            hashMap_GetULongPayload(threadHashMap,replyThread,&encoded) &&
            ( (encoded / MAX_THREADS_PER_BOARD) == boardIndex ) )
       {
         unsigned long threadSlot = encoded % MAX_THREADS_PER_BOARD;
         if ( addPostToThread(boardName,&ourSite.boards[boardIndex].threads[threadSlot],&newPost,fileBytes,fileBytesSize) )
         {
           succesfulAddition=1;
           snprintf(resultThreadName,MAX_STRING_SIZE,"%s",replyThread);
         }
       }
     }
   }


  if (succesfulAddition)
  {
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html>\
             <head>\
              <meta http-equiv=\"refresh\" content=\"0; url=threadView.html?board=%s&thread=%s\">\
             </head>\
             <body>Post received , redirecting..</body></html>" , boardName , resultThreadName );
   rqst->contentSize=strlen(rqst->content);
  } else
  {
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html>\
             <head>\
              <meta http-equiv=\"refresh\" content=\"5; url=index.html\">\
             </head>\
             <body><br><br><br><br><br><br><center><h1>Incorrect post , please try again</h1></center></body></html>");
   rqst->contentSize=strlen(rqst->content);
  }



 //Deallocate everything ..
 if (newPost.message  !=0) { free(newPost.message ); }

 return 0;
}
