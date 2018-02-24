
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
   //board=b&replythread=0&MAX_FILE_SIZE=3072000&email=&name=tettee&s=ttete&em=etetet&message=etetete&imagefile=&embed=tete&embedtype=youtube&postpassword=tetetet
   struct post newPost={0};
   struct thread newThread={0};
   unsigned int succesfulAddition=0;

   char * boardName = (char *) malloc ( MAX_STRING_SIZE * sizeof(char) );

   newPost.message =  (char *) malloc ( MAX_STRING_SIZE * sizeof(char) );

   if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         if ( _GETcpy(rqst,"board",boardName,MAX_STRING_SIZE) ) { }
         if ( _GETcpy(rqst,"name",newThread.op,MAX_STRING_SIZE) ) { }
         if ( _GETcpy(rqst,"s",newThread.title,MAX_STRING_SIZE) ) { }
         if ( _GETcpy(rqst,"postpassword",newThread.password,MAX_STRING_SIZE) ) { }
         //TODO Process options if ( _GET(rqst,"em",newThread->op,MAX_STRING_SIZE) ) { }
         if ( _GETcpy(rqst,"message",newPost.message ,MAX_STRING_SIZE) ) { }
         //if ( _GET(rqst,"name",newThread.op,MAX_STRING_SIZE) ) { }
       }
    }


   //TODO : CREATE FILE HERE



/*
   if ( addThreadToBoard( boardName , &newThread ) )
   {
      if ( addPostToThread( boardName , &newThread , &newPost ) )
      {
        succesfulAddition = 1;
      }
   }*/


  if (succesfulAddition)
  {
   snprintf(rqst->content,rqst->MAXcontentSize,
           "<html>\
             <head>\
              <meta http-equiv=\"refresh\" content=\"5; url=threadView.html?board=%s&&id=%u\">\
             </head>\
             <body>\
           \
            <br> Got a message going to board %s <br>\
            From : %s  <br>\
            Subject : %s <br>\
            Password : %s <br>\
            Message : %s <br> \
           \
           \
           Got back something</body></html>" , boardName , 666 , boardName , newThread.op , newThread.title , newThread.password , newPost.message );
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
 if (boardName!=0) { free(boardName); }
 if (newPost.message  !=0) { free(newPost.message ); }

 return 0;
}

