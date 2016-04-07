#ifndef INDEXER_H_INCLUDED
#define INDEXER_H_INCLUDED

#define MAX_STR 512


struct videoItem
{
  unsigned long hashID;
  unsigned long views;
  unsigned long likes;
  unsigned long dislikes;
  unsigned long stateChanges;
  unsigned int visibility;

  char title[MAX_STR];
  char tagsStr[MAX_STR]; //To become a list
  char filename[MAX_STR];
  char comment[MAX_STR];
  char thumbnail[MAX_STR];
  char stats[MAX_STR];
};


struct videoCollection
{
    struct videoItem * video;
    unsigned int numberOfLoadedVideos;
    unsigned int MAX_numberOfVideos;
};

extern unsigned int videoDefaultTestTranmission;

char * path_cat2 (const char *str1,const char *str2);
unsigned int getAVideoForQuery(struct videoCollection * db , const char * query , int * foundVideo);

int saveVideoStats(struct videoCollection* vc ,  const char * databasePath , unsigned int videoID);

int unloadVideoDatabase(struct videoCollection* vc);
struct videoCollection * loadVideoDatabase(const char * directoryPath,const char * databasePath);


#endif // INDEXER_H_INCLUDED
