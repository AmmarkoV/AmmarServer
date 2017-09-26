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

  char title[MAX_STR+1];
  char tagsStr[MAX_STR+1]; //To become a list
  char filename[MAX_STR+1];
  char comment[MAX_STR+1];
  char thumbnail[MAX_STR+1];
  char stats[MAX_STR+1];
};


struct videoCollection
{
    struct videoItem * video;
    unsigned int numberOfLoadedVideos;
    unsigned int MAX_numberOfVideos;
};

extern unsigned int videoDefaultTestTranmission;

char * path_cat2 (const char *str1,const char *str2);
unsigned int getAUserIDForSession(struct videoCollection * db , const char * sessionQuery , const char * sessionToken , int * foundSession);

int saveVideoStats(struct videoCollection* vc ,  const char * databasePath , unsigned int videoID);

int unloadVideoDatabase(struct videoCollection* vc);

unsigned int getDBIndexFromPermanentLink(const char* id);
int indexerSaveVideoDatabaseToIndexFile(const char * filename , struct videoCollection* vc);
struct videoCollection * indexerLoadVideoDatabaseFromIndexFile(const char * filename , const char * directoryPath,const char * databasePath);

#endif // INDEXER_H_INCLUDED
