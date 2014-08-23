#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmServerlib/hashmap/hashmap.h"

#include "state.h"


extern struct AmmServer_Instance  * default_server;
extern struct AmmServer_Instance  * admin_server;
extern struct AmmServer_RequestOverride_Context GET_override;




extern unsigned int threadIndexPageLength;
extern char * threadIndexPage;



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











extern struct hashMap * boardHashMap ;

int loadBoards();

int unloadBoards();

#endif // STATE_H_INCLUDED
