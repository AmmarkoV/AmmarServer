#include "renderVideoPage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int renderVideoPage(struct videoCollection *  myTube , struct AmmServer_MemoryHandler * videoMH , unsigned int videoID , unsigned int userID , unsigned int secondsStart , unsigned int stillDownloading)
{
                AmmServer_Warning("Replacing Variables for (%s) ..!\n",myTube->video[videoID].filename );
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,3,"++++++++++++++++++++++++++++++++++++++++++++++++++++++TITLE++++++++++++++++++++++++++++++++++++++++++++++++++++++",myTube->video[videoID].title);

                char data[513];


                snprintf(data,512,"%u",stillDownloading);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++STILLDOWNLOADING+++++++++",data);

                snprintf(data,512,"%u",secondsStart);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++STARTTIME+++++++++",data);

                if (strstr(myTube->video[videoID].filename,".flv")!=0)
                {
                 snprintf(data,512,"<source src=\"video?v=%u\" type=\"video/x-flv\">",videoID);
                } else
                {
                 snprintf(data,512,"<source src=\"video?v=%u\" type=\"video/mp4\">",videoID);
                }
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++++++++++++++++++++SOURCE+++++++++++++++++++++++++++",data);

                snprintf(data,512,"dthumb.jpg?v=%u",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,2,"+++++++++++++++++++++++++++THUMB+++++++++++++++++++++++++++",data);

                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"+++++++++USER+++++++++","MyTube");

                snprintf(data,512,"watch?v=%u",videoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,2,"++++++++++++CURRENTVIDEOLINK++++++++++++",data);


                ++myTube->video[videoID].views;
                ++myTube->video[videoID].stateChanges;
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


                char tag[512];
                unsigned int randVideoID=0;
                unsigned int nextVideoID=rand()%myTube->numberOfLoadedVideos;

                snprintf(tag,512,"%u",nextVideoID);
                AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,2,"$NEXT_VIDEO$",tag);

                unsigned int i=0;
                for (i=1; i<=14; i++)
                {
                 snprintf(tag,512,"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++PLAYLIST%u+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++",i);
                 if (i==1) {
                            randVideoID=videoID;
                           } else
                 if (i==2) {
                            randVideoID=nextVideoID;
                           } else
                           {
                            randVideoID=rand()%myTube->numberOfLoadedVideos;
                           }
                  snprintf(data,512,"<table><tr><td><a href=\"/watch?v=%u\"><img src=\"dthumb.jpg?v=%u\" width=100></a></td><td class=\"dsc\"><a href=\"/watch?v=%u\"><b>%s</b></a><br>by MyTube<br>%lu views</td></tr></table>",randVideoID,randVideoID,randVideoID,myTube->video[randVideoID].title,myTube->video[randVideoID].views );
                  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,tag, data );
                }

  return 1;
}
