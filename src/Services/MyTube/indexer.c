#include "indexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define DEFAULT_TEST_TRANSMISSION_VIDEO_TITLE "MyTube Test Broadcast"
extern unsigned int videoDefaultTestTranmission=0;

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

unsigned int getAVideoForQuery(struct videoCollection * db , const char * query)
{

}

unsigned int clearExtensionFAST(char * inputOutputStr)
{
  unsigned int lengthOfStr=strlen(inputOutputStr);
  if (lengthOfStr>4)
  {
    if (inputOutputStr[lengthOfStr-3]=='.') {  inputOutputStr[lengthOfStr-3]=0; return 1; }
    if (inputOutputStr[lengthOfStr-4]=='.') {  inputOutputStr[lengthOfStr-4]=0; return 1; }

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

struct videoCollection * loadVideoDatabase(char * directoryPath)
{
    struct videoCollection * newDB=(struct videoCollection * ) malloc(sizeof(struct videoCollection));
    if (newDB==0) { fprintf(stderr,"Could not allocate a video collection \n"); return 0; }


    newDB->MAX_numberOfVideos = 9000;
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
            fprintf(stderr,"Got garbage out of readdir(%s)\n",dir);
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
                ++newDB->numberOfLoadedVideos;
                snprintf(newDB->video[newDB->numberOfLoadedVideos].filename,MAX_STR,dp->d_name);
                snprintf(newDB->video[newDB->numberOfLoadedVideos].title,MAX_STR,dp->d_name);
                clearExtensionFAST(newDB->video[newDB->numberOfLoadedVideos].title);

                if (strcmp(newDB->video[newDB->numberOfLoadedVideos].title,DEFAULT_TEST_TRANSMISSION_VIDEO_TITLE)==0)
                {
                   AmmServer_Success("Found our default transmission video");
                   videoDefaultTestTranmission=newDB->numberOfLoadedVideos;
                }



                //Now lets try to get filesize and modification date using stat.h
                char * fullpath = path_cat2(directoryPath,dp->d_name);
                if (fullpath!=0 )
                {

                fprintf(stderr,"%u - %s  ",newDB->numberOfLoadedVideos,dp->d_name);

                if (AmmServer_FileIsVideo(fullpath))
                {
                 fprintf(stderr," is video ");
                } else
                {
                  fprintf(stderr," is nothing ");
                }
                 fprintf(stderr,"\n");



                    if ( stat(fullpath, &st) == 0 )
                    {
                        char sizeStr[128]= {0};
                        snprintf(sizeStr,128,"%li",st.st_size);

                        //Append FileSize information
                        /*
                        strncat(memory,tag_pre_date,mem_remaining);
                        mem_remaining-=tag_pre_date_size;
                        strftime(sizeStr, 128, "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime ) );
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

            }

        }


    }

    closedir(dir);
    return newDB;
}
