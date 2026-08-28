
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "state.h"
#include "board.h"
#include "thread.h"
#include "csrf.h"

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmCaptcha/AmmCaptcha.h"

void * processPostReceiver(struct AmmServer_DynamicRequest  * rqst)
{
   //board=b&replythread=new&name=tettee&s=ttete&message=etetete&imagefile=&postpassword=tetetet
   unsigned int succesfulAddition=0;
   const char * failureReason = 0;
   unsigned int imageWasDropped = 0;

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

   //Every check below runs in order and the first one that fails wins , so the visitor is told exactly
   //what went wrong instead of a single generic "Incorrect post" for every possible reason.
   char csrfToken[64]={0};
   _POSTcpy(rqst,"csrftoken",csrfToken,sizeof(csrfToken));

   char captchaIDStr[32]={0};
   _POSTcpy(rqst,"captchaID",captchaIDStr,sizeof(captchaIDStr));
   char captchaReply[64]={0};
   _POSTcpy(rqst,"captcha",captchaReply,sizeof(captchaReply));

   unsigned long boardIndex=0;
   unsigned long encodedThread=0;
   struct thread * targetThread = 0;
   unsigned int isNewThread = ( (strlen(replyThread)==0) || (strcmp(replyThread,"new")==0) );

   if ( !isCSRFTokenValid(csrfToken) )
   {
     failureReason = "This form has expired , please reload the page and try again";
   } else
   if ( !AmmCaptcha_isReplyCorrect((unsigned int)atoi(captchaIDStr),captchaReply) )
   {
     failureReason = "Captcha answer was incorrect";
   } else
   if ( !hashMap_ContainsKey(boardHashMap,boardName) )
   {
     failureReason = "Unknown board";
   } else
   if ( (newPost.message==0) || (strlen(newPost.message)==0) )
   {
     failureReason = "Message cannot be empty";
   } else
   if ( isNewThread )
   {
     hashMap_GetULongPayload(boardHashMap,boardName,&boardIndex);
     if ( ourSite.boards[boardIndex].currentThreads >= ourSite.boards[boardIndex].maxThreads )
     {
       failureReason = "This board is full and cannot accept new threads";
     }
   } else
   {
     if ( ! ( hashMap_GetULongPayload(boardHashMap,boardName,&boardIndex) &&
              hashMap_GetULongPayload(threadHashMap,replyThread,&encodedThread) &&
              ( (encodedThread / MAX_THREADS_PER_BOARD) == boardIndex ) ) )
     {
       failureReason = "Thread not found";
     } else
     {
       targetThread = &ourSite.boards[boardIndex].threads[encodedThread % MAX_THREADS_PER_BOARD];
       if (targetThread->deleted)
       {
         failureReason = "Thread not found";
       } else
       if (targetThread->numberOfReplies >= targetThread->maxNumberOfReplies)
       {
         failureReason = "This thread is full and cannot accept more replies";
       }
     }
   }

   if (failureReason==0)
   {
     if (isNewThread)
     {
       if ( createThread(boardName,newPost.op,subject,newPost.password,&newPost,fileBytes,fileBytesSize,resultThreadName,MAX_STRING_SIZE) )
       {
         succesfulAddition=1;
       } else
       {
         failureReason = "Internal error while creating the thread , please try again";
       }
     } else
     {
       if ( addPostToThread(boardName,targetThread,&newPost,fileBytes,fileBytesSize) )
       {
         succesfulAddition=1;
         snprintf(resultThreadName,MAX_STRING_SIZE,"%s",replyThread);
       } else
       {
         failureReason = "Internal error while storing the reply , please try again";
       }
     }

     //addPostToThread() clears newPost.hasFile if an attachment was provided but rejected ( not a real jpg/png/gif )
     if ( (fileBytes!=0) && (fileBytesSize>0) && (!newPost.hasFile) ) { imageWasDropped=1; }
   }


  if (succesfulAddition)
  {
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html>\
             <head>\
              <meta http-equiv=\"refresh\" content=\"0; url=threadView.html?board=%s&thread=%s\">\
             </head>\
             <body>Post received%s , redirecting..</body></html>" ,
             boardName , resultThreadName ,
             imageWasDropped ? " ( attachment was rejected : not a valid jpg/png/gif )" : "" );
   rqst->contentSize=strlen(rqst->content);
  } else
  {
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html>\
             <head>\
              <meta http-equiv=\"refresh\" content=\"5; url=index.html\">\
             </head>\
             <body><br><br><br><br><br><br><center><h1>%s</h1></center></body></html>",
             (failureReason!=0) ? failureReason : "Incorrect post , please try again");
   rqst->contentSize=strlen(rqst->content);
  }



 //Deallocate everything ..
 if (newPost.message  !=0) { free(newPost.message ); }

 return 0;
}
