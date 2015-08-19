
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thumbnailer.h"
#include "../../AmmServerlib/AmmServerlib.h"


char * generateThumbnailOfVideo(const char * videoDirectory,const char * videofile,const char * thumbDirectory)
{
   unsigned int thumbDirectoryLength=strlen(thumbDirectory);
   unsigned int videoFileLength=strlen(videofile);

   unsigned int thumbnailFileLength=thumbDirectoryLength+videoFileLength+25;
   char * thumbnailFile = (char*) malloc(sizeof(char) * thumbnailFileLength);

   if (thumbnailFile!=0)
   {
    snprintf(thumbnailFile,thumbnailFileLength,"%s/%s_thumb.jpg",thumbDirectory,videofile);
    if (AmmServer_FileExists(thumbnailFile))
    {
      fprintf(stderr,"Thumbnail already exists\n");
      return thumbnailFile;
    } else
    {
     #if GENERATE_NEW_THUMBNAILS
     char what2Execute[1024];
     snprintf(what2Execute,1024,"ffmpeg -y -i \"%s/%s\" -ss 00:00:14.435 -vframes 1 -vf scale=320:240 \"%s\" ",videoDirectory,videofile,thumbnailFile);

     fprintf(stderr,"Spawning %s .. ",thumbnailFile);
     int i=system(what2Execute);

     if (i==0)
     {
       fprintf(stderr,"success \n");
       return thumbnailFile;
     }
     fprintf(stderr,"failed\n");
     #else
      fprintf(stderr,"Generating new thumbnails for %s is disabled in this build\n",videofile);
     #endif // GENERATE_NEW_THUMBNAILS
     free(thumbnailFile);
    }

   }
  return 0;
}
