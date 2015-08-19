#ifndef INDEXER_H_INCLUDED
#define INDEXER_H_INCLUDED

#define MAX_STR 512


struct videoItem
{
  unsigned long hashID;
  unsigned long views;
  unsigned long likes;
  unsigned long dislikes;
  unsigned int visibility;

  char title[MAX_STR];
  char tagsStr[MAX_STR]; //To become a list
  char filename[MAX_STR];
  char comment[MAX_STR];
  char thumbnail[MAX_STR];
};


struct videoCollection
{
    struct videoItem * video;
    unsigned int numberOfLoadedVideos;
    unsigned int MAX_numberOfVideos;
};

extern unsigned int videoDefaultTestTranmission;

char * path_cat2 (const char *str1,const char *str2);

struct videoCollection * loadVideoDatabase(char * directoryPath);


#endif // INDEXER_H_INCLUDED
