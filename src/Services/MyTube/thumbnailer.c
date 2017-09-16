
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thumbnailer.h"
#include "../../AmmServerlib/AmmServerlib.h"



int createThumbnailDir(const char * outputDir)
{
  char savePath[512]={0};
  snprintf(savePath,512,"mkdir -p %s/db",outputDir);

  int i=system(savePath);
  if (i==0) { fprintf(stderr,"Successfully created thumbnail dataset \n"); } else
            { fprintf(stderr,"Failed to create thumbnail dataset \n"); }

  return 1;
}



char * generateThumbnailOfVideo(int live,const char * videoDirectory,const char * videofile,const char * thumbDirectory)
{
   fprintf(stderr,"generateThumbnailOfVideo(%s,%s,%s)\n",videoDirectory,videofile,thumbDirectory);
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
     if (live)
     {
      #if GENERATE_NEW_THUMBNAILS_LIVE
       #warning "This build generates new thumbnails live ,this is not very good.."
      #else
       free(thumbnailFile);
       return 0;
      #endif // GENERATE_NEW_THUMBNAILS
     }
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
     free(thumbnailFile);
    }

   }
  return 0;
}
