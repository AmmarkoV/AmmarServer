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

#include "session.h"
#include "login.h"
#include "chat.h"
#include "home.h"
#include "posts.h"


char webserver_root[MAX_FILE_PATH]="src/Services/Social/res/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context login={0};
struct AmmServer_RH_Context signup={0};
struct AmmServer_RH_Context logout={0};
struct AmmServer_RH_Context home={0};
struct AmmServer_RH_Context profile={0};
struct AmmServer_RH_Context newPost={0};
struct AmmServer_RH_Context newComment={0};
struct AmmServer_RH_Context newLike={0};
struct AmmServer_RH_Context chat={0};
struct AmmServer_RH_Context chatSpeak={0};
struct AmmServer_RH_Context chatPicture={0};
struct AmmServer_RH_Context chatMessages={0};
struct AmmServer_RH_Context createRoom={0};


//Every resource here is session enabled : that is what makes rqst->sessionToken ( and with it the whole
//_SESSION* / AmmServer_CurrentUsername() / CSRF token family ) mean anything inside the callbacks..
static void addSessionResourceHandler(
                                       struct AmmServer_RH_Context * context,
                                       const char * resourceName,
                                       unsigned int allocateMemoryBytes,
                                       void * callback,
                                       unsigned int scenario
                                     )
{
  AmmServer_AddResourceHandler(default_server,context,resourceName,allocateMemoryBytes,0,callback,scenario);
  AmmServer_DoNOTCacheResourceHandler(default_server,context);
  context->requestContext.useSessionLifecycle=1;
}


//This function adds a Resource Handler for each page of the social network and associates it with its callback
void init_dynamic_content()
{
  if (!initializeLoginSystem())    { AmmServer_Error("Could not initialize user accounts"); }
  if (!loadPosts("db/social.db"))  { AmmServer_Error("Could not initialize the post database"); }
  if (!initializeChat())           { AmmServer_Error("Could not initialize the chat rooms"); }

  addSessionResourceHandler(&login,"/doLogin.html",4096,&login_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  addSessionResourceHandler(&signup,"/doSignup.html",4096,&signup_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  addSessionResourceHandler(&logout,"/logout.html",4096,&logout_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);

  addSessionResourceHandler(&home,"/home.html",1024*1024,&home_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  addSessionResourceHandler(&profile,"/profile.html",1024*1024,&profile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);

  addSessionResourceHandler(&newPost,"/post.html",4096,&post_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  addSessionResourceHandler(&newComment,"/comment.html",4096,&comment_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  addSessionResourceHandler(&newLike,"/like.html",4096,&like_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);

  addSessionResourceHandler(&chat,"/chat.html",65536,&chatPage_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  addSessionResourceHandler(&chatMessages,"/chatmessages.html",65536,&chatMessages_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  addSessionResourceHandler(&chatSpeak,"/chatSpeak.html",4096,&chatSpeak_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  addSessionResourceHandler(&chatPicture,"/chatPicture.html",4096,&chatPicture_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  addSessionResourceHandler(&createRoom,"/createRoom.html",4096,&createRoom_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&login,1);
    AmmServer_RemoveResourceHandler(default_server,&signup,1);
    AmmServer_RemoveResourceHandler(default_server,&logout,1);
    AmmServer_RemoveResourceHandler(default_server,&home,1);
    AmmServer_RemoveResourceHandler(default_server,&profile,1);
    AmmServer_RemoveResourceHandler(default_server,&newPost,1);
    AmmServer_RemoveResourceHandler(default_server,&newComment,1);
    AmmServer_RemoveResourceHandler(default_server,&newLike,1);
    AmmServer_RemoveResourceHandler(default_server,&chat,1);
    AmmServer_RemoveResourceHandler(default_server,&chatSpeak,1);
    AmmServer_RemoveResourceHandler(default_server,&chatPicture,1);
    AmmServer_RemoveResourceHandler(default_server,&chatMessages,1);
    AmmServer_RemoveResourceHandler(default_server,&createRoom,1);

    unloadPosts();
    stopLoginSystem();
}




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
