/*
AmmarServer , simple template executable

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2015

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
#include "indexer.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";
char video_root[MAX_FILE_PATH]="/home/ammar/Videos/Internet/";
char database_root[MAX_FILE_PATH]="/home/ammar/Videos/Internet/db/";

struct videoCollection * myTube=0;

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context videoPageContext={0};
struct AmmServer_RH_Context videoFileContext={0};
struct AmmServer_RH_Context randomVideoFileContext={0};
struct AmmServer_RH_Context thumbnailContext={0};


struct AmmServer_MemoryHandler * indexPage=0;



//This function prepares the content of  stats context , ( stats.content )
void * serve_videofile(struct AmmServer_DynamicRequest  * rqst)
{
  char videoRequested[128]={0};
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                fprintf(stderr,"Video Requested is : %s \n",videoRequested);

                unsigned int videoID=atoi(videoRequested);
                if (videoID >= myTube->numberOfLoadedVideos)
                {
                  rqst->headerResponse=404;
                } else
                {
                char * fullpath = path_cat2(video_root,myTube->video[videoID].filename);
                if (fullpath!=0 )
                {
                 AmmServer_Warning("Trying to redirect request to file (%s) ..!\n",fullpath);
                 if (!AmmServer_DynamicRequestReturnFile(rqst,fullpath) )
                 {
                  AmmServer_Error("Could not redirect request to file (%s) ..!\n",fullpath);
                 }
                 free(fullpath);
                }

                }

              }
  return 0;
}

//This function prepares the content of  stats context , ( stats.content )
void * serve_videopage(struct AmmServer_DynamicRequest  * rqst)
{
  char videoRequested[128]={0};
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                fprintf(stderr,"Video Requested is : %s \n",videoRequested);

                unsigned int videoID=atoi(videoRequested);

                if (videoID >= myTube->numberOfLoadedVideos)
                {
                  rqst->headerResponse=404;
                } else
                {
                struct AmmServer_MemoryHandler * videoMH = AmmServer_CopyMemoryHandler(indexPage);
                AmmServer_Warning("Replacing Variables for (%s) ..!\n",myTube->video[videoID].filename );
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,3,"++++++++++++++++++++++++++++++++++++++++++++++++++++++TITLE++++++++++++++++++++++++++++++++++++++++++++++++++++++",myTube->video[videoID].title);

                char data[256];
                snprintf(data,256,"<source src=\"video?v=%u\" type=\"video/mp4\">",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++++++++++++++++++++SOURCE+++++++++++++++++++++++++++",data);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++USER+++++++++","USERNAME");
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++VIEWS+++++++++","1");
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++COMMENT+++++++++","Comment of video etc");
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++USERCOMMENTS+++++++++","Comment of user video etc");
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++PLAYLIST+++++++++","Playlist");

               memcpy( rqst->content , videoMH->content , videoMH->contentSize );
               rqst->contentSize = videoMH->contentSize;
               fprintf(stderr,"Gave back %u\n",rqst->contentSize);
               AmmServer_FreeMemoryHandler(&videoMH);
               }
              }
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_random_videopage(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int videoID=rand()%myTube->numberOfLoadedVideos;
  snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='watch?v=%u'\" /></head></html>",videoID);
  fprintf(stderr,"Giving back random video %u/%u \n",videoID,myTube->numberOfLoadedVideos);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_thumbnail(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int videoID=rand()%myTube->numberOfLoadedVideos;
  snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='watch?v=%u'\" /></head></html>",videoID);
  fprintf(stderr,"Giving back random video %u/%u \n",videoID,myTube->numberOfLoadedVideos);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{

  fprintf(stderr,"Reading master index file..  ");
  indexPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/player.html");
  fprintf(stderr,"current length %u , size is %u \n",indexPage->contentCurrentLength , indexPage->contentSize);


  myTube = loadVideoDatabase(video_root);


  //---------------
  if (! AmmServer_AddResourceHandler(default_server,&videoFileContext,"/video",webserver_root,14096,0,&serve_videofile,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve video file\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&videoPageContext,"/watch",webserver_root,14096,0,&serve_videopage,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&randomVideoFileContext,"/random",webserver_root,14096,0,&serve_random_videopage,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve random video page\n"); }
  //---------------

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&videoPageContext,1);
    AmmServer_RemoveResourceHandler(default_server,&videoFileContext,1);
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

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "MyTube",
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
