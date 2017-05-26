#include "chat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



struct AmmServer_MemoryHandler * chatPage=0;

int isLinkToImage(char * message)
{

 return 0;
}

int stripMessage(char * message)
{
  unsigned int messageLength = strlen(message);
  unsigned int i=0;

  for (i=0; i<messageLength; i++)
  {
    switch (message[i])
    {
      case '+' : message[i]=' '; break;
      case '<' : message[i]=' '; break;
      case '>' : message[i]=' '; break;
    };
  }
 return 1;
}

int appendMessage(const char * chatroom, const char * from , const char * message )
{
  FILE * fp;
  fp=fopen(chatroom,"a");
  if (fp!=0)
  {
    fprintf(fp,"%s:%s<br>\n",from,message);
    fclose(fp);
    return 1;
  }
 return 0;
}



//This function prepares the content of  random_chars context , ( random_chars.content )
void * chat_callback(struct AmmServer_DynamicRequest  * rqst)
{
 int haveMessage=0;
 int haveName=0;

 char name[128]={0};
  if ( _GET(rqst->instance,rqst,"name",name,128) )    { haveName=1; }

  char message[512]={0};
  if ( _GET(rqst->instance,rqst,"text",message,512) )
    {
     stripMessage(message);
     //isLinkToImage
     haveMessage=1;
    }


  if ( (haveMessage) && (haveName) )
  {
    appendMessage("db/default.chat",name,message);
  }

  struct AmmServer_MemoryHandler * chatRoomWithContents = AmmServer_CopyMemoryHandler(chatPage);
  AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,2,"$CHATROOM_NAME$","AmmarServer");


  struct AmmServer_MemoryHandler * chatContents=AmmServer_ReadFileToMemoryHandler("db/default.chat");
    if (chatContents!=0)
      {
         AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,1,"$MESSAGES_GO_HERE$",chatContents->content);
         AmmServer_FreeMemoryHandler(&chatContents);
      } else
      {
         AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,1,"$MESSAGES_GO_HERE$","Error Retrieving content..");

      }
  if (haveName) { AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,1,"$USER$",name); } else
                { AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,1,"$USER$","Unknown"); }

  if ( (haveMessage) && (haveName) )
  {
   AmmServer_Success("Received From : %s => Message: %s",name,message);
   AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,1,"$STATUS$","Message sent");
  } else
  {
   AmmServer_Warning("Received Something From : %s => Message: %s",name,message);
   AmmServer_ReplaceAllVarsInMemoryHandler(chatRoomWithContents,1,"$STATUS$","Could not deliver message");
  }


  memcpy (rqst->content , chatRoomWithContents->content , chatRoomWithContents->contentCurrentLength );
  rqst->contentSize=chatRoomWithContents->contentCurrentLength ;
  AmmServer_FreeMemoryHandler(&chatRoomWithContents);
  return 0;
}




void * chatMessages_callback(struct AmmServer_DynamicRequest  * rqst)
{
 AmmServer_Success("Serving back chat contents..");

  struct AmmServer_MemoryHandler * chatContents=AmmServer_ReadFileToMemoryHandler("db/default.chat");
    if (chatContents!=0)
      {
         memcpy (rqst->content , chatContents->content , chatContents->contentCurrentLength );
         rqst->contentSize=chatContents->contentCurrentLength;
         AmmServer_FreeMemoryHandler(&chatContents);
      } else
      {
         snprintf(rqst->content,rqst->MAXcontentSize,"Could not get messages");
         rqst->contentSize=strlen(rqst->content);
      }
 return 0;
}
