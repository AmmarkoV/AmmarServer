/*
AmmarServer , QuestTeleop relay

Sits between an Oculus Quest 2 teleoperation APK and a robot ( or simulator ) front-end ,
both of which are plain HTTP/1.1 keep-alive clients of this server :

   Quest   --POST /setpose.html  ( multipart field "pose"  , JSON )-->  server  --GET /pose.json-->  robot
   Quest   <--GET  /frame.jpg  -------------------------------------  server  <--POST /setframe.html ( multipart file "frame" , JPEG )--  robot

Only the latest pose and the latest frame are kept , there is no queueing : a client that polls
slower than the other side produces simply skips the in-between ones.

A robot side on the same host can skip HTTP ( built with SharedMemoryVideoBuffers , see shm_link.h ) :
every pose also goes to the quest_pose shared memory stream , and raw view pixels the robot puts in
quest_view are JPEG-encoded here when the Quest asks for /frame.jpg.

Written by Ammar Qammaz a.k.a. AmmarkoV

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../../AmmServerlib/AmmServerlib.h"
#if USE_SHMVB
 #include "shm_link.h"
#endif

#define DEFAULT_BINDING_PORT 8080

#define MAX_POSE_SIZE  (64*1024)    //A pose packet ( head + 2 x 24 hand joints + bind poses ) is ~5KB of JSON
#define MAX_FRAME_SIZE (2*1024*1024) //A 1080p JPEG is ~300KB

char webserver_root[MAX_FILE_PATH]="public_html/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";

struct AmmServer_Instance  * default_server=0;

struct AmmServer_RH_Context setPoseContext={0};
struct AmmServer_RH_Context getPoseContext={0};
struct AmmServer_RH_Context setFrameContext={0};
struct AmmServer_RH_Context getFrameContext={0};

//The latest pose / frame , each one guarded by its own lock since they are written and read by different connection threads
struct LatestBlob
{
  pthread_mutex_t lock;
  char * data;
  unsigned long size;
  unsigned long maxSize;
  unsigned long updates;
};

struct LatestBlob pose ={PTHREAD_MUTEX_INITIALIZER,0,0,MAX_POSE_SIZE,0};
struct LatestBlob frame={PTHREAD_MUTEX_INITIALIZER,0,0,MAX_FRAME_SIZE,0};


static int storeBlob(struct LatestBlob * blob,const char * data,unsigned long size)
{
  if ( (data==0) || (size==0) || (size>blob->maxSize) ) { return 0; }
  pthread_mutex_lock(&blob->lock);
   memcpy(blob->data,data,size);
   blob->size=size;
   ++blob->updates;
  pthread_mutex_unlock(&blob->lock);
  return 1;
}

static unsigned long serveBlob(struct LatestBlob * blob,struct AmmServer_DynamicRequest * rqst)
{
  pthread_mutex_lock(&blob->lock);
   unsigned long size = blob->size;
   if (size>rqst->MAXcontentSize) { size=0; }
   memcpy(rqst->content,blob->data,size);
  pthread_mutex_unlock(&blob->lock);
  rqst->contentSize=size;
  return size;
}

static void reply(struct AmmServer_DynamicRequest * rqst,int ok,unsigned long updates)
{
  snprintf(rqst->content,rqst->MAXcontentSize,"{\"ok\":%d,\"updates\":%lu}",ok,updates);
  rqst->contentSize=strlen(rqst->content);
}


//POST /setpose.html , multipart field "pose" holds the JSON the Quest produced
void * setPose_callback(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int size=0;
  const char * value = _POST(rqst,"pose",&size);
  int ok = storeBlob(&pose,value,size);
  #if USE_SHMVB
   if (ok) { shmLink_publishPose(value,size); }
  #endif
  reply(rqst,ok,pose.updates);
  return 0;
}

//GET /pose.json , the latest pose ( {} until the Quest has sent one )
void * getPose_callback(struct AmmServer_DynamicRequest  * rqst)
{
  if (!serveBlob(&pose,rqst))
     {
       snprintf(rqst->content,rqst->MAXcontentSize,"{}");
       rqst->contentSize=2;
     }
  return 0;
}

//POST /setframe.html , multipart file field "frame" holds a JPEG of the robot's view
void * setFrame_callback(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int size=0;
  const char * value = _FILES(rqst,"frame",VALUE,&size);
  int ok = storeBlob(&frame,value,size);
  reply(rqst,ok,frame.updates);
  return 0;
}

//GET /frame.jpg , the latest frame
void * getFrame_callback(struct AmmServer_DynamicRequest  * rqst)
{
  #if USE_SHMVB
   //A newer view in shared memory replaces the latest frame ( encoded only when someone asks for it )
   pthread_mutex_lock(&frame.lock);
    unsigned long size = shmLink_takeViewJPEG(frame.data,frame.maxSize,80);
    if (size) { frame.size=size; ++frame.updates; }
   pthread_mutex_unlock(&frame.lock);
  #endif
  if (!serveBlob(&frame,rqst))
     {
       //No frame yet : an empty body would make the library fall back to serving a file and close the keep-alive
       //connection , so answer a single byte that the Quest fails to decode and ignores
       rqst->content[0]=0;
       rqst->contentSize=1;
     }
  return 0;
}


void init_dynamic_content()
{
  pose.data  = (char*) malloc(pose.maxSize);
  frame.data = (char*) malloc(frame.maxSize);
  if ( (pose.data==0) || (frame.data==0) ) { AmmServer_Error("Could not allocate pose/frame buffers\n"); exit(1); }
  #if USE_SHMVB
   if (shmLink_start(MAX_POSE_SIZE)) { fprintf(stderr,"QuestTeleop relay : shared memory link on %s\n",QUEST_SHM_CONTEXT); }
                                else { AmmServer_Warning("QuestTeleop relay : no shared memory link , HTTP only\n"); }
  #endif

  //Every handler gets DIFFERENT_PAGE_FOR_EACH_CLIENT so each response is copied into its own buffer under our lock ,
  //a shared page would be sent after the framework unlocks it and could be torn by the next upload
  AmmServer_AddResourceHandler(default_server,&setPoseContext,"/setpose.html",256,0,&setPose_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&getPoseContext,"/pose.json",MAX_POSE_SIZE,0,&getPose_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&setFrameContext,"/setframe.html",256,0,&setFrame_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&getFrameContext,"/frame.jpg",MAX_FRAME_SIZE,0,&getFrame_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);

  AmmServer_DoNOTCacheResourceHandler(default_server,&getPoseContext);
  AmmServer_DoNOTCacheResourceHandler(default_server,&getFrameContext);
}

void close_dynamic_content()
{
  AmmServer_RemoveResourceHandler(default_server,&setPoseContext,1);
  AmmServer_RemoveResourceHandler(default_server,&getPoseContext,1);
  AmmServer_RemoveResourceHandler(default_server,&setFrameContext,1);
  AmmServer_RemoveResourceHandler(default_server,&getFrameContext,1);
  #if USE_SHMVB
   shmLink_stop();
  #endif
  free(pose.data);  pose.data=0;
  free(frame.data); frame.data=0;
}


int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s QuestTeleop relay starting up..\n",AmmServer_Version());
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    snprintf(bindIP,MAX_IP_STRING_SIZE,"0.0.0.0");

    default_server = AmmServer_StartWithArgs(
                                             "questteleop",
                                              argc,argv , //e.g. -p 8080
                                              bindIP,
                                              DEFAULT_BINDING_PORT,
                                              0,
                                              webserver_root,
                                              templates_root
                                              );
    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    init_dynamic_content();
    fprintf(stderr,"QuestTeleop relay : Quest POSTs /setpose.html and GETs /frame.jpg , robot GETs /pose.json and POSTs /setframe.html\n");

    unsigned long lastPose=0,lastFrame=0;
    while (AmmServer_Running(default_server))
           {
             sleep(5);
             fprintf(stdout,"QuestTeleop : %.1f poses/s in , %.1f frames/s in\n",(pose.updates-lastPose)/5.0,(frame.updates-lastFrame)/5.0);
             fflush(stdout);
             lastPose=pose.updates; lastFrame=frame.updates;
           }

    close_dynamic_content();
    AmmServer_Stop(default_server);
    AmmServer_Warning("QuestTeleop relay stopped\n");
    return 0;
}
