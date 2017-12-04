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

#include "renderVideoPage.h"
#include "renderVideoList.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define DO_DYNAMIC_THUMBNAILS 1
#define UPDATE_ALL_THUMBNAILS_ON_LAUNCH 0 //<-- this will make booting the program incredibly slow

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

#define MASTER_INDEX_FILE "db/mytube.index"

#define VIDEO_FILES_PATH_1 "/media/db46941e-4297-41d0-aa7e-659452e16780/home/guarddog/Internet/"
#define VIDEO_FILES_PATH_2 "/home/ammar/Videos/Internet/"
#define VIDEO_FILES_PATH_3 "~/Videos/"

char video_root[MAX_FILE_PATH]="~/Videos/";
char database_root[MAX_FILE_PATH]="~/Videos/db/";

unsigned int stop=0;

struct videoCollection * myTube=0;

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context errorPageContext={0};
struct AmmServer_RH_Context videoPageContext={0};
struct AmmServer_RH_Context videoFileContext={0};
struct AmmServer_RH_Context uploadContext={0};
struct AmmServer_RH_Context randomVideoFileContext={0};
struct AmmServer_RH_Context thumbnailContext={0};
struct AmmServer_RH_Context interactContext={0};
struct AmmServer_RH_Context indexContext={0};
struct AmmServer_RH_Context faviconContext={0};
struct AmmServer_RH_Context cssContext={0};
struct AmmServer_RH_Context jsContext={0};
struct AmmServer_RH_Context stopContext={0};


struct AmmServer_MemoryHandler * indexPage=0;
struct AmmServer_MemoryHandler * headerPage=0;
struct AmmServer_MemoryHandler * favicon=0;
struct AmmServer_MemoryHandler * uploadIcon=0;
struct AmmServer_MemoryHandler * cssFile=0;
struct AmmServer_MemoryHandler * jsFile=0;


//public_html/robots.txt
//public_html/sitemap.xml

int enableMonitor=0;



//This function prepares the content of  stats context , ( stats.content )
void * serve_index(struct AmmServer_DynamicRequest  * rqst)
{
  snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><head><meta http-equiv=\"refresh\" content=\"0;URL='watch?v=%u'\" /></head><body> </body> </html> ",videoDefaultTestTranmission);
  fprintf(stderr,"Giving back index video %u/%u \n",videoDefaultTestTranmission,myTube->numberOfLoadedVideos);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * serve_upload(struct AmmServer_DynamicRequest  * rqst)
{
   return serve_index(rqst);
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_random_videopage(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int videoID=rand()%myTube->numberOfLoadedVideos;
  snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n\
     <html>\n\
       <head>\n\
        <script type=\"text/javascript\">\n\
         <!--\n\
            function Redirect() {\n\
                                  window.location=\"watch?v=%u\";\n\
                                }\n\
         //-->\n\
      </script><meta http-equiv=\"refresh\" content=\"0;URL='watch?v=%u'\"/></head><body onload=\"Redirect();\"> </body></html> ",videoID,videoID);
  fprintf(stderr,"Giving back random video %u/%u \n",videoID,myTube->numberOfLoadedVideos);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}





//This function prepares the content of  stats context , ( stats.content )
void * serve_videofile(struct AmmServer_DynamicRequest  * rqst)
{
  char videoRequested[128]={0};
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                fprintf(stderr,"Video Requested is : %s \n",videoRequested);

                unsigned int videoID=getDBIndexFromPermanentLink(videoRequested);
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
  int sessionFoundVideo =0 ;
  int queryFoundVideo =0 ;
  unsigned int userID=0;
  unsigned int videoID=0;
  char sessionRequested[128]={0};
  char sessionToken[128]={0};
  char videoRequested[128]={0};

  char pickRequested[128]={0};
  char timeRequested[128]={0};
  unsigned int doPickFromList=0;
  unsigned int pickNumber=0;
  unsigned int startTime=0;

  if ( _GET(default_server,rqst,"t",timeRequested,128) )
              {
                startTime=atoi(timeRequested);
              }


  if ( _GET(default_server,rqst,"s",sessionRequested,128) )
              {
                userID = getAUserIDForSession(myTube,sessionRequested,sessionToken,&sessionFoundVideo );
              }

  if ( _GET(default_server,rqst,"p",pickRequested,128) )
              {
                doPickFromList=1;
                pickNumber=atoi(pickRequested);
              }

  if ( _GET(default_server,rqst,"q",videoRequested,128) )
              {
                if (renderVideoList(myTube,headerPage,rqst,videoRequested,userID,&videoID,doPickFromList,pickNumber))
                {
                  //renderVideoList handled the query on its own ( no results or many results )
                  return 0;
                }  else
                {
                  //Only one result renderVideoList failed to server we will with the videoID provided
                  queryFoundVideo=1;
                }
              }
         else
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                fprintf(stderr,"Video Requested is : %s \n",videoRequested);
                videoID=getDBIndexFromPermanentLink(videoRequested);
                queryFoundVideo=1;
              }




  if ( queryFoundVideo )
                if (videoID >= myTube->numberOfLoadedVideos)
                {
                  rqst->headerResponse=404;
                } else
                {
                   struct AmmServer_MemoryHandler * videoMH = AmmServer_CopyMemoryHandler(indexPage);

                   if (renderVideoPage(myTube , videoMH , videoID , userID , startTime))
                   {
                    memcpy( rqst->content , videoMH->content , videoMH->contentCurrentLength );
                    rqst->contentSize = videoMH->contentCurrentLength;
                    rqst->content[rqst->contentSize]=0; //Make sure null termination is there..!
                    fprintf(stderr,"Gave back %lu\n",rqst->contentSize);

                    if (myTube->video[videoID].stateChanges)
                     { saveVideoStats(myTube,database_root,videoID); }
                   }

                   AmmServer_FreeMemoryHandler(&videoMH);
               }
    else
 {
  snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\" /></head><body><h2>Redirecting..</h2></body> </html> ");
  rqst->contentSize=strlen(rqst->content);
 }

  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_stop(struct AmmServer_DynamicRequest  * rqst)
{
  AmmServer_Error("Stopping Server after request\n");
  snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><body>Stopping</body></html>");
  stop=1;
  rqst->contentSize=strlen(rqst->content);

  return 0;
}

void * serve_js(struct AmmServer_DynamicRequest  * rqst)
{
  return AmmServer_DynamicRequestReturnMemoryHandler(rqst,jsFile);
}

void * serve_css(struct AmmServer_DynamicRequest  * rqst)
{
  return AmmServer_DynamicRequestReturnMemoryHandler(rqst,cssFile);
}

//This function prepares the content of  stats context , ( stats.content )
void * serve_favicon(struct AmmServer_DynamicRequest  * rqst)
{
  return AmmServer_DynamicRequestReturnMemoryHandler(rqst,favicon);
}

//This function prepares the content of  stats context , ( stats.content )
void * serve_thumbnail(struct AmmServer_DynamicRequest  * rqst)
{
 #if DO_DYNAMIC_THUMBNAILS
  char videoRequested[128]={0};
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                fprintf(stderr,"Thumbnail Requested for Video  : %s \n",videoRequested);

                if (strcmp(videoRequested,"upload")==0)
                {
                  if (uploadIcon==0) { return 0; }
                  if (uploadIcon->content==0) { return 0; }
                  if (uploadIcon->contentSize==0) { return 0; }

                   memcpy(rqst->content,uploadIcon->content,uploadIcon->contentSize);
                  return 0;
                } else
                if (strcmp(videoRequested,"linux")==0)
                {
                  if (!AmmServer_DynamicRequestReturnFile(rqst,"public_html/thumb.jpg") )
                      {
                        AmmServer_Error("Could not return default thumbnail");
                      }
                } else
                {
                 unsigned int videoID=getDBIndexFromPermanentLink(videoRequested);

                 if (videoID < myTube->numberOfLoadedVideos)
                 {
                  char * thumbnailFile = generateThumbnailOfVideo(1,video_root,myTube->video[videoID].filename,database_root);
                  if (thumbnailFile!=0)
                   {
                    AmmServer_DynamicRequestReturnFile(rqst,thumbnailFile);
                    free(thumbnailFile);
                    return 0;
                   }
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
void * serve_playbackerror(struct AmmServer_DynamicRequest  * rqst)
{
  char videoRequested[128]={0};
  if ( _GET(default_server,rqst,"v",videoRequested,128) )
              {
                AmmServer_Error("Playback Client Error for Video  : %s \n",videoRequested);
                unsigned int videoID=getDBIndexFromPermanentLink(videoRequested);
                if (videoID < myTube->numberOfLoadedVideos)
                {
                  AmmServer_AppendToFile("log/mytube_playbackerror.log",videoRequested);
                  snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><body>Error Report ACK</body></html>");
                  rqst->contentSize=strlen(rqst->content);
                }
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
     videoID=getDBIndexFromPermanentLink(videoRequested);
     if (videoID < myTube->numberOfLoadedVideos)
     {
         ++myTube->video[videoID].likes;
         ++myTube->video[videoID].stateChanges;
     }
  } else
  if ( _GET(default_server,rqst,"downvote",videoRequested,128) )
  {
     videoID=getDBIndexFromPermanentLink(videoRequested);
     if (videoID < myTube->numberOfLoadedVideos)
     {
         ++myTube->video[videoID].dislikes;
         ++myTube->video[videoID].stateChanges;
     }
  } else
  if ( _GET(default_server,rqst,"comment",videoRequested,128) )
  {
  } else
  {

  }

  if (myTube->video[videoID].stateChanges)
   { saveVideoStats(myTube,database_root,videoID); }

  snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Ok</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


int thumbnailAllVideoDatabase(struct videoCollection * db)
{
   unsigned int i=0;

   createThumbnailDir(database_root);

   for (i=0; i<db->numberOfLoadedVideos; i++)
   {
     char * thumbnailFile = generateThumbnailOfVideo(0,video_root,myTube->video[i].filename,database_root);
     if (thumbnailFile!=0)
     {
       free(thumbnailFile);
     }
   }
  return 1;
}

int indexAllVideoDatabase(struct videoCollection * db)
{
  return indexerSaveVideoDatabaseToIndexFile(MASTER_INDEX_FILE,db);
}




//This function prepares the content of  stats context , ( stats.content )
void * schedulerSaveStatistics()
{
return 0;
}


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{

  fprintf(stderr,"Reading master index file..  ");
  indexPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/player.html");
  fprintf(stderr,"current length %u , size is %u \n",indexPage->contentCurrentLength , indexPage->contentSize);
  headerPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/header.html");
  fprintf(stderr,"current length %u , size is %u \n",headerPage->contentCurrentLength , headerPage->contentSize);


  favicon=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/favicon.ico");
  cssFile=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/mytube.css");
  jsFile=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/mytube.js");
  uploadIcon=AmmServer_ReadFileToMemoryHandler("src/Services/MyTube/res/upload.png");



  AmmServer_AddScheduler( default_server,"Stats",(void*) schedulerSaveStatistics, 30 /*mins*/* 60 /*seconds*/  , 0 );


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
      myTube = indexerLoadVideoDatabaseFromIndexFile(MASTER_INDEX_FILE,video_root,database_root); //loadVideoDatabase(video_root,database_root);
      break;
     }
  }
  fprintf(stderr,"Done loading.. \n ");

  if (myTube==0)
  {
    AmmServer_Error("Could not initialize video database from %s \n",video_root);
    exit(1);
  }

  //this will make boot incredibly slower
  #if UPDATE_ALL_THUMBNAILS_ON_LAUNCH
    thumbnailAllVideoDatabase(myTube);
  #endif // UPDATE_ALL_THUMBNAILS_ON_LAUNCH

  if (enableMonitor)
  {
    AmmServer_Warning("Enabling monitor\n");
    AmmServer_EnableMonitor(default_server);
    if (! AmmServer_AddResourceHandler(default_server,&stopContext,"/stop.html",webserver_root,4096,1000,&serve_stop,SAME_PAGE_FOR_ALL_CLIENTS) )
         { AmmServer_Warning("Failed adding serve stop page\n"); }
  }


  //---------------


  if (! AmmServer_AddResourceHandler(default_server,&uploadContext,"/upload.html",webserver_root,14096,0,&serve_upload,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve error file\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&errorPageContext,"/error",webserver_root,14096,0,&serve_playbackerror,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve error file\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&videoFileContext,"/video",webserver_root,14096,0,&serve_videofile,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve video file\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&videoPageContext,"/watch",webserver_root,25000,0,&serve_videopage,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&randomVideoFileContext,"/random",webserver_root,4096,0,&serve_random_videopage,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve random video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&thumbnailContext,"/dthumb.jpg",webserver_root,4096,0,&serve_thumbnail,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding serve random video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&interactContext,"/proc",webserver_root,4096,0,&serve_interact,DIFFERENT_PAGE_FOR_EACH_CLIENT) )         { AmmServer_Warning("Failed adding serve random video page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&indexContext,"/index.html",webserver_root,4096,0,&serve_index,DIFFERENT_PAGE_FOR_EACH_CLIENT) )    { AmmServer_Warning("Failed adding serve index page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&faviconContext,"/favicon.ico",webserver_root,16400,1000,&serve_favicon,SAME_PAGE_FOR_ALL_CLIENTS) ) { AmmServer_Warning("Failed adding serve favicon page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&cssContext,"/mytube.css",webserver_root,4096,1000,&serve_css,SAME_PAGE_FOR_ALL_CLIENTS) ) { AmmServer_Warning("Failed adding serve favicon page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&jsContext,"/mytube.js",webserver_root,4096,1000,&serve_js,SAME_PAGE_FOR_ALL_CLIENTS) ) { AmmServer_Warning("Failed adding serve favicon page\n"); }


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
    AmmServer_RemoveResourceHandler(default_server,&faviconContext,1);

    AmmServer_FreeMemoryHandler(&indexPage);
    AmmServer_FreeMemoryHandler(&favicon);


  if (enableMonitor)
  {
    AmmServer_RemoveResourceHandler(default_server,&stopContext,1);
  }



    unloadVideoDatabase(myTube);
    //AmmServer_Stop(default_server);
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
                                             "mytube",
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

    int i=0;
    for (i=0; i<argc; i++)
    {
     if (strcmp(argv[i],"--thumbnail")==0) {
                                            fprintf(stderr,"Thumbnailing .. \n");
                                            thumbnailAllVideoDatabase(myTube);
                                            close_dynamic_content();
                                            AmmServer_Stop(default_server);
                                            exit(0);
                                          } else
     if (strcmp(argv[i],"--index")==0)    {
                                            fprintf(stderr,"Recreating Index File .. \n");
                                            indexAllVideoDatabase(myTube);
                                            close_dynamic_content();
                                            AmmServer_Stop(default_server);
                                            exit(0);
                                          }
    }


         while ( (AmmServer_Running(default_server)) && (!stop)  )
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             //In other applications the programmer could use the main thread to do anything he likes..
             //The only caveat is that he would takeup more CPU time from the server and that he would have to poll
             //the AmmServer_Running() call once in a while to make sure everything is in order
             //usleep(60000);
             sleep(1);
           }

     fprintf(stderr,"Waiting for clients to finish\n");
     sleep(3);

    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
     sleep(3);
    return 0;
}
