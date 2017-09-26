
#define _GNU_SOURCE
//for strcasestr

#include "renderVideoList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned int getAVideoForQuery(struct videoCollection * db , const char * query , int * foundVideo)
{
  AmmServer_Warning("Searching for `%s` among %u videos  \n\n",query,db->numberOfLoadedVideos);
  unsigned int i=0;
  *foundVideo=0;
  for (i=0; i<db->numberOfLoadedVideos; i++)
  {
      if (strstr(db->video[i].filename , query)!=0)
      {
        AmmServer_Success("Found it @ %u \n\n",i);
        *foundVideo+=1;
        return i;
      }
  }

  if (*foundVideo==0) {   AmmServer_Error("Could not Find it %s \n\n",query); }
  if (*foundVideo>1)  {   AmmServer_Error("Found more than one videos %s \n\n",query); *foundVideo=0; }
  return (*foundVideo);
}

int renderVideoList(struct videoCollection *  db ,
                    struct AmmServer_MemoryHandler * headerHTML ,
                    struct AmmServer_DynamicRequest  *  rqst,
                    const char * query ,
                    unsigned int userID ,
                    unsigned int * doImmediateVideoID,
                    unsigned int doPickFromList,
                    unsigned int pickNumber)
{

  AmmServer_Warning("renderVideoList(%s)",query);

  unsigned int limitOfVideosPerPage=25;
  unsigned int foundVideos = 0 , i =0;
  unsigned int outOfMemory = 0;
  unsigned int remainingSize = rqst->MAXcontentSize;

  rqst->content[0]=0;
  char data[512];
  unsigned int dataSize;


  snprintf(data,512,"<!DOCTYPE html>\
           <html>\
           <head>\
            <meta charset=\"UTF-8\">\
            <title>MyTube Listing %s</title>\
            <link rel=\"stylesheet\" type=\"text/css\" href=\"mytube.css\">\
         </head>\
         <body>",query);
  dataSize=strlen(data);
  if (dataSize>remainingSize) { outOfMemory = 1; } else { strncat(rqst->content,data,dataSize); remainingSize-=dataSize; }



  if (headerHTML->contentSize>remainingSize) { outOfMemory = 1; } else { strncat(rqst->content,headerHTML->content,headerHTML->contentSize); remainingSize-=headerHTML->contentSize; }



  snprintf(data,512,"<table>"); dataSize=strlen(data);
  if (dataSize>remainingSize) { outOfMemory = 1; } else { strncat(rqst->content,data,dataSize); remainingSize-=dataSize; }


  for (i=0; i<db->numberOfLoadedVideos; i++)
  {

      if (strcasestr(db->video[i].filename , query)!=0)
      // if (strstr(db->video[i].filename , query)!=0) faster case sensitive search but needs all filenames to be lowercase or smth..
        {
         snprintf(data,512,
                  "<tr>\
                    <td>\
                     <a href=\"watch?v=%u\">\
                      <img src=\"dthumb.jpg?v=%u\" height=100>\
                     </a>\
                    </td>\
                    <td>\
                     <a href=\"watch?v=%u\"  >%s</a>\
                    </td>\
                    </tr> ",i,i,i,db->video[i].title); dataSize=strlen(data);
         if (dataSize>remainingSize) { outOfMemory = 1; } else { strncat(rqst->content,data,dataSize);  remainingSize-=dataSize; }

          ++foundVideos;
          *doImmediateVideoID=i;

          if ( (doPickFromList) && (foundVideos==pickNumber) )
          {
             //We were looking to pick a specific video from the list so we are ready now
             *doImmediateVideoID=i; //In case the code above changes in the future..!
             break;
          }
        }

    if (foundVideos>limitOfVideosPerPage) {  AmmServer_Warning("Stopping after %u results",limitOfVideosPerPage); break; }

  }
  AmmServer_Warning("Found %u results",foundVideos);

  snprintf(data,512,"</table></body></html>"); dataSize=strlen(data);
  if (dataSize>remainingSize) { outOfMemory = 1; } else { strncat(rqst->content,data,dataSize);  remainingSize-=dataSize;}

  if (doPickFromList)
    {
      AmmServer_Success("Immediate response of picked from list %u",*doImmediateVideoID);
      //We dont handle one videos , only lists here , let the parent know we failed and show doImmediateVideoID
      return 0;
    } else
  if (outOfMemory==1)
    {
      AmmServer_Error("Could not fit list to memory buffer `%s`\n\n",query);
      snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><head><meta http-equiv=\"refresh\" content=\"5;URL='index.html'\" /></head><body><h2>Could not return results/h2></body> </html> ");
      rqst->contentSize=strlen(rqst->content);
      return 1; //We handled it
    } else
  if (foundVideos==0)
    {
      AmmServer_Error("Could not Find Video Query  `%s`\n\n",query);
      snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><head><meta http-equiv=\"refresh\" content=\"5;URL='index.html'\" /></head><body><h2>We got your query but could not find it..</h2></body> </html> ");
      rqst->contentSize=strlen(rqst->content);
      return 1; //We handled it
    } else
  if (foundVideos==1)
    {
      AmmServer_Success("Immediate response of single found Video %u",*doImmediateVideoID);
      //We dont handle one videos , only lists here , let the parent know we failed and show doImmediateVideoID
      return 0;
    }


  //We have multiple videos ready and we already handled it
  AmmServer_Warning("Client Should have our list \n");
  //We signal when our content stops
  rqst->contentSize=strlen(rqst->content);

  //We have handled it
  return 1;
}
