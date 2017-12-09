#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../Hashmap/hashmap.h"

#include "state.h"


extern struct AmmServer_Instance  * default_server;
extern struct AmmServer_Instance  * admin_server;
extern struct AmmServer_RequestOverride_Context GET_override;

extern struct site ourSite;

extern struct AmmServer_MemoryHandler * threadIndexPage;

#define MAX_BOARDS 1000
#define MAX_THREADS_PER_BOARD 1000
#define LINE_MAX_LENGTH 1024
#define MAX_STRING_SIZE 512

enum FILETYPES_ENUM
{
  FILETYPE_FORBIDDEN=0,
  FILETYPE_IMAGE,
  FILETYPE_AUDIO,
  FILETYPE_VIDEO_FILE,
  FILETYPE_VIDEO_YOUTUBE,
  //--------------------
  NUMBER_OF_FILETYPES
};


struct timestamp
{
  unsigned int day,month,year,hour,minute,second;
};

struct post
{
  unsigned int numberOfComplaints;

  char op[MAX_STRING_SIZE];
  char password[MAX_STRING_SIZE];


  unsigned char hasFile;
  unsigned char fileType;
  char fileOriginalName[MAX_STRING_SIZE];
  char fileCachedName[MAX_STRING_SIZE];
  unsigned int fileDimensionWidth;
  unsigned int fileDimensionHeight;


  struct timestamp creation;

  unsigned int messageSize;
  char * message;
};



struct thread
{
  unsigned char sticky;
  unsigned char repliable;

  char op[MAX_STRING_SIZE];
  char password[MAX_STRING_SIZE];
  char title[MAX_STRING_SIZE];

  struct timestamp creation;
  struct timestamp lastReply;

  unsigned int maxNumberOfReplies;
  unsigned int numberOfReplies;
  unsigned int numberOfImages;

  struct post * replies;
};


struct board
{
  char name[MAX_STRING_SIZE];

  unsigned int maxThreads;
  unsigned int currentThreads;

  unsigned int currentUsers;

  unsigned int active;
  unsigned int hidden;

  unsigned int postUID;
  unsigned int threadUID;
  unsigned int imageUID;

  unsigned int * threadQueue;
  struct thread * threads;
};

struct site
{
  unsigned int maxNumberOfBoards;
  unsigned int numberOfBoards;
  struct board *  boards;

  char siteName[MAX_STRING_SIZE];
  char siteDescription[MAX_STRING_SIZE];
};

extern struct hashMap * boardHashMap ;
extern struct hashMap * threadHashMap;

int loadSite( char *filename );

int unloadSite();


int addPostToThread( char * boardName ,  struct thread * newThread ,  struct post * newPost );

#endif // STATE_H_INCLUDED
