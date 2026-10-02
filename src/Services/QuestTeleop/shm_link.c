/*
AmmarServer , QuestTeleop relay : same-host link to the robot side through SharedMemoryVideoBuffers ( see shm_link.h )
*/
#include "shm_link.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "sharedMemoryVideoBuffers.h"
#include "../../BasicImaging/basicImaging.h"

static struct SharedMemoryContext * context=0;
static struct VideoFrame * poseStream=0;
static char * poseBuffer=0;
static unsigned int poseBufferSize=0;
static uint64_t poseSeq=0;
static pthread_mutex_t poseLock=PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t viewLock=PTHREAD_MUTEX_INITIALIZER;
static uint64_t lastViewTimestamp=0;
static unsigned char * sideBySide=0; //the eyes interleaved row by row for the encoder
static unsigned long sideBySideSize=0;


int shmLink_start(unsigned int maxPoseSize)
{
  if (createSharedMemoryContextDescriptor(QUEST_SHM_CONTEXT)!=0) { return 0; }
  context = connectToSharedMemoryContextDescriptor(QUEST_SHM_CONTEXT);
  if (context==0) { return 0; }
  poseBufferSize = sizeof(struct QuestPoseHeader) + maxPoseSize;
  if (createGenericMetaData(context,"quest_pose",poseBufferSize)!=0) { return 0; }
  poseStream = getVideoBufferPointer(context,"quest_pose");
  poseBuffer = (char*) malloc(poseBufferSize);
  return (poseStream!=0) && (poseBuffer!=0);
}

void shmLink_stop()
{
  if (context!=0) { destroyVideoFrame(context,"quest_pose"); }
  free(poseBuffer);  poseBuffer=0;
  free(sideBySide);  sideBySide=0;
  poseStream=0;
  context=0;
}

void shmLink_publishPose(const char * json,unsigned int length)
{
  if ( (poseStream==0) || (length+sizeof(struct QuestPoseHeader)>poseBufferSize) ) { return; }
  pthread_mutex_lock(&poseLock);
   struct QuestPoseHeader * h = (struct QuestPoseHeader *) poseBuffer;
   h->magic  = QUEST_POSE_MAGIC;
   h->length = length;
   h->seq    = ++poseSeq;
   memcpy(poseBuffer+sizeof(struct QuestPoseHeader),json,length);
   if (startWritingToVideoBufferPointer(poseStream))
   {
     copy_to_shared_memory(poseStream,poseBuffer,sizeof(struct QuestPoseHeader)+length,0);
     stopWritingToVideoBufferPointer(poseStream);
   }
  pthread_mutex_unlock(&poseLock);
}

//JPEG with a COM segment right after SOI : FFD8 FFFE <len> <text> <the rest of the encoder's output>
static unsigned long encodeWithComment(struct Image * img,const char * comment,char * jpeg,unsigned long maxSize,int quality)
{
  unsigned long textLength = strlen(comment);
  unsigned long headerLength = 2 + 4 + textLength;
  if (maxSize<=headerLength) { return 0; }
  unsigned long encoded=0;
  //Encode after the room the comment needs , then move the SOI in front of it
  if (!BasicImaging_SaveJPEGToMemory(img,(unsigned char*)jpeg+headerLength-2,maxSize-headerLength+2,&encoded,quality)) { return 0; }
  jpeg[0]=(char)0xFF; jpeg[1]=(char)0xD8;
  jpeg[2]=(char)0xFF; jpeg[3]=(char)0xFE;
  jpeg[4]=(char)((textLength+2)>>8); jpeg[5]=(char)((textLength+2)&0xFF);
  memcpy(jpeg+6,comment,textLength);
  return headerLength-2+encoded;
}

unsigned long shmLink_takeViewJPEG(char * jpeg,unsigned long maxSize,int quality)
{
  if (context==0) { return 0; }
  //Looked up on every call : the robot side may start after us , restart , or change the view size
  struct VideoFrame * view = getVideoBufferPointer(context,"quest_view");
  if (view==0) { return 0; }

  unsigned long result=0;
  pthread_mutex_lock(&viewLock);
  if (startReadingFromVideoBufferPointer(view))
  {
    uint64_t timestamp = getVideoFrameTimestamp(view);
    const unsigned char * data = getVideoFrameDataPointer(view);
    unsigned long size = getVideoFrameDataSize(view);
    const struct QuestViewHeader * h = (const struct QuestViewHeader *) data;
    if ( (timestamp!=lastViewTimestamp) && (data!=0) && (size>=sizeof(struct QuestViewHeader)) && (h->magic==QUEST_VIEW_MAGIC) &&
         (h->eyes>=1) && (h->eyes<=2) &&
         (sizeof(struct QuestViewHeader) + (unsigned long) h->eyes*h->height*h->width*3 <= size) )
    {
      const unsigned char * pixels = data + sizeof(struct QuestViewHeader);
      unsigned long eyeRow = (unsigned long) h->width*3 , eyeSize = eyeRow*h->height;
      unsigned long needed = eyeSize*h->eyes;
      if (needed>sideBySideSize)
      {
        unsigned char * grown = (unsigned char*) realloc(sideBySide,needed);
        if (grown!=0) { sideBySide=grown; sideBySideSize=needed; }
      }
      if (needed<=sideBySideSize)
      {
        for (unsigned int y=0; y<h->height; y++)
          for (unsigned int e=0; e<h->eyes; e++)
            memcpy(sideBySide + (y*h->eyes+e)*eyeRow , pixels + e*eyeSize + y*eyeRow , eyeRow);

        struct Image img={0};
        img.pixels=sideBySide; img.width=h->width*h->eyes; img.height=h->height;
        img.channels=3; img.bitsperpixel=24; img.image_size=needed;
        char comment[160];
        snprintf(comment,sizeof(comment),"fovy=%.2f q=%.5f,%.5f,%.5f,%.5f%s",
                 h->fovy,h->quat[0],h->quat[1],h->quat[2],h->quat[3],(h->eyes==2) ? " stereo=1" : "");
        result = encodeWithComment(&img,comment,jpeg,maxSize,quality);
        lastViewTimestamp=timestamp;
      }
    }
    stopReadingFromVideoBufferPointer(view);
  }
  pthread_mutex_unlock(&viewLock);
  return result;
}
