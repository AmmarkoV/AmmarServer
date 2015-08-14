#include "indexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

char * path_cat2 (const char *str1, char *str2)
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


struct videoCollection * loadVideoDatabase(char * directoryPath)
{
    struct videoCollection * newDB=(struct videoCollection * ) malloc(sizeof(struct videoCollection));
    if (newDB==0) { fprintf(stderr,"Could not allocate a video collection \n"); return 0; }


    newDB->MAX_numberOfVideos = 5000;
    newDB->video = (struct videoItem *) malloc(  newDB->MAX_numberOfVideos * sizeof(struct videoItem) );
    if (newDB->video==0) { fprintf(stderr,"Could not allocate a video item\n"); free(newDB); return 0;}



    unsigned int count=0;
    struct stat st;
    struct dirent *dp= {0};
// enter existing path to directory below
    DIR *dir = opendir(directoryPath);
    if (dir==0)
    {
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
                ++count;
                snprintf(newDB->video[count].filename,MAX_STR,dp->d_name);


                //Now lets try to get filesize and modification date using stat.h
                char * fullpath = path_cat(directoryPath,dp->d_name);
                if (fullpath!=0 )
                {

                fprintf(stderr,"%u - %s  ",count,dp->d_name);

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
