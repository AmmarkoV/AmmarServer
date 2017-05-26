/*
AmmarServer , simple template executable

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"


char webserver_root[MAX_FILE_PATH]="src/Services/Social/res/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context login={0};
struct AmmServer_RH_Context chat={0};
struct AmmServer_RH_Context chatMessages={0};

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
  if ( _GET(default_server,rqst,"name",name,128) )    { haveName=1; }

  char message[512]={0};
  if ( _GET(default_server,rqst,"text",message,512) )
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


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  chatPage=AmmServer_ReadFileToMemoryHandler("src/Services/Social/res/chatroom.html");

  AmmServer_AddResourceHandler(default_server,&chat,"/chat.html",webserver_root,4096,0,&chat_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  chat.allowCrossRequests=1;
  AmmServer_AddResourceHandler(default_server,&chatMessages,"/chatmessages.html",webserver_root,4096,0,&chatMessages_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  chatMessages.allowCrossRequests=1;


}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&chat,1);
    AmmServer_RemoveResourceHandler(default_server,&chatMessages,1);
    AmmServer_FreeMemoryHandler(&chatPage);
}
/*! Dynamic content code ..! END ------------------------*/




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=8087;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "Social",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );


    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    //Create dynamic content allocations and associate context to the correct files
    init_dynamic_content();
    //stats.html and formtest.html should be availiable from now on..!

         while ( (AmmServer_Running(default_server))  )
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             //In other applications the programmer could use the main thread to do anything he likes..
             //The only caveat is that he would takeup more CPU time from the server and that he would have to poll
             //the AmmServer_Running() call once in a while to make sure everything is in order
             //usleep(60000);
             sleep(1);
           }

    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}
