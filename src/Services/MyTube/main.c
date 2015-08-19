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
#include "thumbnailer.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define DO_DYNAMIC_THUMBNAILS 1
#define UPDATE_ALL_THUMBNAILS_ON_LAUNCH 0 //<-- this will make booting the program incredibly slow

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

#define VIDEO_FILES_PATH_1 "/media/db46941e-4297-41d0-aa7e-659452e16780/home/guarddog/Internet/"
#define VIDEO_FILES_PATH_2 "/home/ammar/Videos/Internet/"
#define VIDEO_FILES_PATH_3 "~/Videos/"

char video_root[MAX_FILE_PATH]="~/Videos/";
char database_root[MAX_FILE_PATH]="~/Videos/db/";


struct videoCollection * myTube=0;

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context videoPageContext={0};
struct AmmServer_RH_Context videoFileContext={0};
struct AmmServer_RH_Context randomVideoFileContext={0};
struct AmmServer_RH_Context thumbnailContext={0};
struct AmmServer_RH_Context interactContext={0};
struct AmmServer_RH_Context indexContext={0};
struct AmmServer_RH_Context faviconContext={0};


struct AmmServer_MemoryHandler * indexPage=0;
struct AmmServer_MemoryHandler * favicon=0;




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

                char data[512];
                snprintf(data,512,"<source src=\"video?v=%u\" type=\"video/mp4\">",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++++++++++++++++++++SOURCE+++++++++++++++++++++++++++",data);

                snprintf(data,512,"dthumb.jpg?v=%u",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++++++++++++++++++++THUMB+++++++++++++++++++++++++++",data);

                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++USER+++++++++","MyTube");


                ++myTube->video[videoID].views;
                snprintf(data,512,"%lu",myTube->video[videoID].views);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++VIEWS+++++++++",data);


                snprintf(data,512,"%lu",myTube->video[videoID].likes);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"++++++VOTESUP++++++",data);
                snprintf(data,512,"%lu",myTube->video[videoID].dislikes);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"++++++VOTESDOWN++++++",data);


                //snprintf(data,512,"/proc?upvote=%u",videoID);
                snprintf(data,512,"command('upvote=%u');",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++++UPVOTE+++++++++++",data);

                //snprintf(data,512,"/proc?downvote=%u",videoID);
                snprintf(data,512,"command('downvote=%u');",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++++DOWNVOTE+++++++++++",data);


                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++COMMENT+++++++++","Test Video Service");
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++USERCOMMENTS+++++++++","Comments are disabled..");
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++PLAYLIST+++++++++","Playlist");


                unsigned int randVideoID=0;
                char tag[512];
                unsigned int i=0;
                for (i=1; i<=10; i++)
                {
                 snprintf(tag,512,"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++PLAYLIST%u+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++",i);
                 randVideoID=rand()%myTube->numberOfLoadedVideos;
                 snprintf(data,512,"<table><tr><td><a href=\"/watch?v=%u\"><img src=\"dthumb.jpg?v=%u\" width=100></a></td><td><a href=\"/watch?v=%u\"><b>%s</b></a><br>by MyTube<br>%lu views</td></tr></table>",randVideoID,randVideoID,randVideoID,myTube->video[randVideoID].title,myTube->video[randVideoID].views );
                 AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,tag, data );

                }




               memcpy( rqst->content , videoMH->content , videoMH->contentSize );
               rqst->contentSize = videoMH->contentSize;
               fprintf(stderr,"Gave back %lu\n",rqst->contentSize);
               AmmServer_FreeMemoryHandler(&videoMH);
               }
              }
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_random_videopage(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int videoID=rand()%myTube->numberOfLoadedVideos;
  snprintf(rqst->content,rqst->MAXcontentSize,"<html> <head> <meta http-equiv=\"refresh\" content=\"0;URL='watch?v=%u'\" /> </head> <body> </body> </html>",videoID);
  fprintf(stderr,"Giving back random video %u/%u \n",videoID,myTube->numberOfLoadedVideos);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_index(struct AmmServer_DynamicRequest  * rqst)
{
  snprintf(rqst->content,rqst->MAXcontentSize,"<html> <head> <meta http-equiv=\"refresh\" content=\"0;URL='watch?v=%u'\" /> </head> <body> </body> </html>",videoDefaultTestTranmission);
  fprintf(stderr,"Giving back index video %u/%u \n",videoDefaultTestTranmission,myTube->numberOfLoadedVideos);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_favicon(struct AmmServer_DynamicRequest  * rqst)
{
  if (favicon==0) { return 0; }
  if (favicon->content==0) { return 0; }
  if (favicon->contentSize==0) { return 0; }

  memcpy(rqst->content,favicon->content,favicon->contentSize);
  rqst->contentSize = favicon->contentSize;
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_thumbnail(struct AmmServer_DynamicRequest  * rqst)
{
 #if DO_DYNAMIC_THUMBNAILS
  char videoRequested[128]={0};
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                fprintf(stderr,"Thumbnail Requested for Video  : %s \n",videoRequested);

                unsigned int videoID=atoi(videoRequested);

                if (videoID < myTube->numberOfLoadedVideos)
                {
                  char * thumbnailFile = generateThumbnailOfVideo(video_root,myTube->video[videoID].filename,database_root);
                  if (thumbnailFile!=0)
                   {
                    AmmServer_DynamicRequestReturnFile(rqst,thumbnailFile);
                    free(thumbnailFile);
                    return 0;
                   }

                }
              }
   #endif // DO_DYNAMIC_THUMBNAILS

  if (!AmmServer_DynamicRequestReturnFile(rqst,"public_html/thumb.jpg") )
   {
       AmmServer_Error("Could not return default thumbnail");
   }

  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_interact(struct AmmServer_DynamicRequest  * rqst)
{
  char videoRequested[128]={0};
  unsigned int videoID=0;
  if ( _GET(default_server,rqst,"upvote",videoRequested,128) )
  {
     videoID=atoi(videoRequested);
     if (videoID < myTube->numberOfLoadedVideos)
     {
         ++myTube->video[videoID].likes;
     }
  } else
  if ( _GET(default_server,rqst,"downvote",videoRequested,128) )
  {
     videoID=atoi(videoRequested);
     if (videoID < myTube->numberOfLoadedVideos)
     {
         ++myTube->video[videoID].dislikes;
     }
  } else
  if ( _GET(default_server,rqst,"comment",videoRequested,128) )
  {
  } else
  {

  }

  snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Ok</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


int thumbnailAllVideoDatabase(struct videoCollection * db)
{
   unsigned int i=0;
   for (i=0; i<db->numberOfLoadedVideos; i++)
   {
     char * thumbnailFile = generateThumbnailOfVideo(video_root,myTube->video[i].filename,database_root);
     if (thumbnailFile!=0)
     {
       free(thumbnailFile);
     }
   }
  return 1;
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{

  fprintf(stderr,"Reading master index file..  ");
  indexPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/player.html");
  fprintf(stderr,"current length %u , size is %u \n",indexPage->contentCurrentLength , indexPage->contentSize);

  favicon=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/favicon.ico");


  //Try to adapt to the server running this :P
  unsigned int i=0;
  for (i=0; i<3; i++)
  {
   if (i==0) { strncpy(video_root,VIDEO_FILES_PATH_1,MAX_FILE_PATH); } else
   if (i==1) { strncpy(video_root,VIDEO_FILES_PATH_2,MAX_FILE_PATH); } else
   if (i==2) { strncpy(video_root,VIDEO_FILES_PATH_3,MAX_FILE_PATH); }

   if ( AmmServer_DirectoryExists(video_root) )
     {
      fprintf(stderr,"Trying to load from %s \n ",video_root);
      snprintf(database_root,MAX_FILE_PATH,"%s/db/",video_root);
      myTube = loadVideoDatabase(video_root);
      break;
     }
  }

  if (myTube==0)
  {
    AmmServer_Error("Could not initialize video database from %s \n",video_root);
    exit(1);
  }

  //this will make boot incredibly slower
  #if UPDATE_ALL_THUMBNAILS_ON_LAUNCH
    thumbnailAllVideoDatabase(myTube);
  #endif // UPDATE_ALL_THUMBNAILS_ON_LAUNCH


  //---------------
  if (! AmmServer_AddResourceHandler(default_server,&videoFileContext,"/video",webserver_root,14096,0,&serve_videofile,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve video file\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&videoPageContext,"/watch",webserver_root,14096,0,&serve_videopage,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&randomVideoFileContext,"/random",webserver_root,4096,0,&serve_random_videopage,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve random video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&thumbnailContext,"/dthumb.jpg",webserver_root,4096,0,&serve_thumbnail,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve random video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&interactContext,"/proc",webserver_root,4096,0,&serve_interact,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve random video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&indexContext,"/index.html",webserver_root,4096,0,&serve_index,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve index page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&faviconContext,"/favicon.ico",webserver_root,4096,1000,&serve_favicon,SAME_PAGE_FOR_ALL_CLIENTS) ) { AmmServer_Warning("Failed adding serve favicon page\n"); }

  //---------------


  AmmServer_Success("default transmission video is %u ",videoDefaultTestTranmission);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&videoFileContext,1);
    AmmServer_RemoveResourceHandler(default_server,&videoPageContext,1);
    AmmServer_RemoveResourceHandler(default_server,&randomVideoFileContext,1);
    AmmServer_RemoveResourceHandler(default_server,&thumbnailContext,1);
    AmmServer_RemoveResourceHandler(default_server,&interactContext,1);
    AmmServer_RemoveResourceHandler(default_server,&indexContext,1);
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
