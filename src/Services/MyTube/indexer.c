#include "indexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "../../AmmServerlib/AmmServerlib.h"

#define DEFAULT_TEST_TRANSMISSION_VIDEO_TITLE "MyTube Test Broadcast"
unsigned int videoDefaultTestTranmission=0;

char * path_cat2 (const char *str1,const char *str2)
{
    size_t str1_len = strlen(str1);
    size_t str2_len = strlen(str2);
    char *result;
    result = malloc((str1_len+str2_len+1)*sizeof *result);
    strcpy (result,str1);
    unsigned int i,j;
    for(i=str1_len, j=0; ((i<(str1_len+str2_len)) && (j<str2_len)); i++, j++)
    {
        result[i]=str2[j];
    }
    result[str1_len+str2_len]='\0';
    return result;
}


unsigned int clearExtensionFAST(char * inputOutputStr)
{
  unsigned int lengthOfStr=strlen(inputOutputStr);
  if (lengthOfStr>=5)
  {
    if (inputOutputStr[lengthOfStr-3]=='.') {  inputOutputStr[lengthOfStr-3]=0; return 1; }
    if (inputOutputStr[lengthOfStr-4]=='.') {  inputOutputStr[lengthOfStr-4]=0; return 1; }
    if (inputOutputStr[lengthOfStr-5]=='.') {  inputOutputStr[lengthOfStr-5]=0; return 1; }
  }
 return 0;
}

int unloadVideoDatabase(struct videoCollection* vc)
{
   if (vc!=0)
   {
       if (vc->video!=0)
       {
         free(vc->video);
       }
    free(vc);
   }
  return 1;
}


int loadVideoStats(struct videoCollection* vc ,  const char * databasePath , unsigned int videoID)
{
 char statsFilePath[512]={0};
 snprintf(statsFilePath,512,"%s/%s_stats",databasePath,vc->video[videoID].filename);


 FILE *fp = fopen(statsFilePath,"r");
 if( fp )
    {
      fscanf(fp, "%lu\n", &vc->video[videoID].views);
      fscanf(fp, "%lu\n", &vc->video[videoID].likes);
      fscanf(fp, "%lu\n", &vc->video[videoID].dislikes);
      vc->video[videoID].stateChanges=0;
      fclose(fp);
      return 1;
    }

 return 0;
}


int saveVideoStats(struct videoCollection* vc ,  const char * databasePath , unsigned int videoID)
{
 char statsFilePath[512]={0};
 snprintf(statsFilePath,512,"%s/%s_stats",databasePath,vc->video[videoID].filename);

 FILE *fp = fopen(statsFilePath,"w");
 if( fp )
    {
      fprintf(fp, "%lu\n",vc->video[videoID].views);
      fprintf(fp, "%lu\n",vc->video[videoID].likes);
      fprintf(fp, "%lu\n",vc->video[videoID].dislikes);
      vc->video[videoID].stateChanges=0;
      fclose(fp);
      return 1;
    }

 return 0;
}



void clear_line()
{
  //return ;
  fputs("\033[A\033[2K\033[A\033[2K",stdout);
  rewind(stdout);
  int i=ftruncate(1,0);
  if (i!=0) { /*fprintf(stderr,"Error with ftruncate\n");*/ }
}




int growVideoDatabase(struct videoCollection * db,unsigned int entriesToAdd)
{
  if (entriesToAdd == 0) { return 0 ; }
  if (db == 0) { fprintf(stderr,"Given an empty video database to grow \n"); return 0 ; }

  struct videoItem * new_video;
  new_video = (struct videoItem *) realloc( db->video, sizeof(struct videoItem)*( db->MAX_numberOfVideos+entriesToAdd ));

   if (new_video == 0 )
    {
       fprintf(stderr,"Cannot add %u frames to our currently %u sized frame buffer\n",entriesToAdd,db->MAX_numberOfVideos);
       return 0;
    } else
     {
      //Clean up all new object types allocated
      void * clear_from_here  =  new_video+db->MAX_numberOfVideos;
      memset(clear_from_here,0,entriesToAdd * sizeof(struct videoItem));
    }

   db->MAX_numberOfVideos+=entriesToAdd;
   db->video = new_video ;
  return 1;
}


int addSingleVideoFile(struct videoCollection * db,const char * title,const char * filename, char * fullFilename)
{
  ++db->numberOfLoadedVideos;
  snprintf(db->video[db->numberOfLoadedVideos].filename,MAX_STR,"%s",filename);
  snprintf(db->video[db->numberOfLoadedVideos].title,MAX_STR,"%s",title);

 return db->numberOfLoadedVideos;
}



struct videoCollection * updateVideoDatabaseFromFilesystem(const char * directoryPath,const char * databasePath)
{
    struct videoCollection * newDB=(struct videoCollection * ) malloc(sizeof(struct videoCollection));
    if (newDB==0) { fprintf(stderr,"Could not allocate a video collection \n"); return 0; }


    newDB->MAX_numberOfVideos = 19000;
    newDB->numberOfLoadedVideos=0;
    newDB->video = (struct videoItem *) malloc(  newDB->MAX_numberOfVideos * sizeof(struct videoItem) );
    if (newDB->video==0) { fprintf(stderr,"Could not allocate a video item\n"); free(newDB); return 0;}



    struct stat st;
    struct dirent *dp= {0};
// enter existing path to directory below
    DIR *dir = opendir(directoryPath);
    if (dir==0)
    {
        free(newDB->video);
        free(newDB);
        return 0;
    }
    while ((dp=readdir(dir)) != 0)
    {
        //TODO: remove // from requests.. of dp->d_name is like /filename.ext
        //char *tmp = path_cat(client_path, dp->d_name);

        if (dp->d_name==0)
        {
            fprintf(stderr,"Got garbage out of readdir(%s)\n",directoryPath);
        }
        else if ( (strcmp(dp->d_name,".")!=0) && (strcmp(dp->d_name,"..")!=0) )
        {
            if (
                (strlen(dp->d_name)>0) &&
                (dp->d_name[0]=='.')
            )
            {
                fprintf(stderr,"Hidding hidden file %s from directory list\n" ,dp->d_name);
            }
            else
            {
              if (newDB->numberOfLoadedVideos+1>=newDB->MAX_numberOfVideos)
              {
                //We just run out of space , allocate another 1k entries..
                growVideoDatabase(newDB,1000);
              }


              if (newDB->numberOfLoadedVideos+1<newDB->MAX_numberOfVideos)
              {
                //fprintf(stderr,"do 0 %u (%s) ",newDB->numberOfLoadedVideos,dp->d_name);
                ++newDB->numberOfLoadedVideos;
                snprintf(newDB->video[newDB->numberOfLoadedVideos].filename,MAX_STR,"%s",dp->d_name);
                snprintf(newDB->video[newDB->numberOfLoadedVideos].title,MAX_STR,"%s",dp->d_name);

                //fprintf(stderr,"do 1 %u ",newDB->numberOfLoadedVideos);
                loadVideoStats(newDB , databasePath , newDB->numberOfLoadedVideos);

                //fprintf(stderr,"do 2 %u ",newDB->numberOfLoadedVideos);
                clearExtensionFAST(newDB->video[newDB->numberOfLoadedVideos].title);

                if (strcmp(newDB->video[newDB->numberOfLoadedVideos].title,DEFAULT_TEST_TRANSMISSION_VIDEO_TITLE)==0)
                {
                   AmmServer_Success("Found our default transmission video");
                   videoDefaultTestTranmission=newDB->numberOfLoadedVideos;
                }



                //Now lets try to get filesize and modification date using stat.h

                //fprintf(stderr,"do 3 %u ",newDB->numberOfLoadedVideos);
                char * fullpath = path_cat2(directoryPath,dp->d_name);
                if (fullpath!=0 )
                {

                clear_line();
                fprintf(stdout,"%u (%0.2f%%) - %s ",
                         newDB->numberOfLoadedVideos,
                         (float) (100*newDB->numberOfLoadedVideos)/ newDB->MAX_numberOfVideos,
                         dp->d_name);
                if (AmmServer_FileIsVideo(fullpath))  { fprintf(stdout," is video "); } else
                                                      { fprintf(stdout," is nothing "); }
                fprintf(stdout,"\n");



                    if ( stat(fullpath, &st) == 0 )
                    {
                        char sizeStr[512]= {0};
                        snprintf(sizeStr,512,"%li",st.st_size);

                        //Append FileSize information
                        /*
                        strncat(memory,tag_pre_date,mem_remaining);
                        mem_remaining-=tag_pre_date_size;
                        strftime(sizeStr, 512, "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime ) );
                        strncat(memory,sizeStr,mem_remaining);
                        mem_remaining-=strlen(sizeStr);
                        strncat(memory,tag_after_date,mem_remaining);
                        mem_remaining-=tag_after_date_size;*/

                    }
                    else
                    {
                        fprintf(stderr,"Error stating file %s -> %s\n",fullpath,strerror(errno));
                    }
                    free(fullpath);
                    fullpath=0;
                }
                //---------------------------------
              }// <- we still have space
            }

        }


    }

    closedir(dir);
    return newDB;
}




unsigned int getDBIndexFromPermanentLink(const char* id)
{
  return atoi(id);
}



int indexerSaveVideoDatabaseToIndexFile(const char * filename , struct videoCollection* vc)
{
 FILE * fp = fopen(filename,"w");
 if (fp!=0)
 {
   unsigned int i=0;
   for (i=0; i<vc->numberOfLoadedVideos; i++)
   {
     fprintf(fp,"%s\n",vc->video[i].filename);
   }
   fclose(fp);
   return 1;
 }
 return 0;
}




struct videoCollection * indexerLoadVideoDatabaseFromIndexFile(const char * filename , const char * directoryPath,const char * databasePath)
{
  //This call should be reading the indexer file instead of doing a read-dir to populate the file database ..
  //this way the video numbering will be stable ( as long as this file is appended there will be no reordering )
  // of course memory allocation should be dynamic and there shouldn't be any file polling to keep startup as fast as possible..

  return updateVideoDatabaseFromFilesystem(directoryPath,databasePath);
}
