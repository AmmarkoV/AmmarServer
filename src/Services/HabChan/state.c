
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <sys/types.h>
#include <dirent.h>

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../Hashmap/hashmap.h"
#include "../../InputParser/InputParser_C.h"


#include "state.h"
#include "board.h"
#include "thread.h"
#include "post.h"


struct AmmServer_Instance  * default_server=0;
struct AmmServer_Instance  * admin_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct hashMap * boardHashMap =0;
struct hashMap * threadHashMap =0;

struct site ourSite={0};

struct AmmServer_MemoryHandler * threadIndexPage = 0;




int loadSite( char * filename )
{
  boardHashMap = hashMap_Create( 100 , 100 , 0 , 1 /*We should have sorting enabled..!*/ );
  threadHashMap = hashMap_Create( 10000 , 1000 , 0 , 1 /*We should have sorting enabled..!*/ );

    unsigned int numberOfElements=0;
    char what2GetBack[1024]={0};
    AmmServer_ExecuteCommandLine("ls data/board -al | cut -d ' ' -f10 | wc -l ", what2GetBack , 1024 );
    numberOfElements = atoi(what2GetBack);

    ourSite.boards = (struct board * ) malloc(sizeof(struct board) * MAX_BOARDS);
    if (ourSite.boards == 0 ) { fprintf(stderr,"Cannot allocate memory to hold boards , failed to load "); return 0; }

    ourSite.maxNumberOfBoards = MAX_BOARDS;
    ourSite.numberOfBoards = 0;
    strncpy(ourSite.siteName ,filename  ,MAX_STRING_SIZE  );


    threadIndexPage      = AmmServer_ReadFileToMemoryHandler("data/simple.html");

   //------------------------------------------------------

   char line [LINE_MAX_LENGTH]={0};
   //Try and open filename , this is an optional configuration file so its absence should not stop boards from loading
   FILE * fp = fopen(filename,"r");
   if (fp == 0 )
   {
     fprintf(stderr,"Cannot open %s , continuing with default site settings \n",filename);
   } else
   {
    //Allocate a token parser
    struct InputParserC * ipc=0;
    ipc = InputParser_Create(LINE_MAX_LENGTH,5);
    if (ipc==0) { fprintf(stderr,"Cannot allocate memory for new stream\n"); fclose(fp); return 0; }

    while (!feof(fp))
       {
         //We get a new line out of the file
         unsigned int readOpResult = (fgets(line,LINE_MAX_LENGTH,fp)!=0);
         if ( readOpResult != 0 )
           {
             //We tokenize it
             unsigned int words_count = InputParser_SeperateWords(ipc,line,0);
             if ( words_count > 0 )
              {
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"SITENAME",8)==1)
                {
                     InputParser_GetWord(ipc,1, ourSite.siteName , MAX_STRING_SIZE );
                } else
                if (InputParser_WordCompareNoCase(ipc,0,(char*)"SITEDESCRIPTION",8)==1)
                {
                     InputParser_GetWord(ipc,1, ourSite.siteDescription , MAX_STRING_SIZE );
                }
              }
          }
       }

    InputParser_Destroy(ipc);
    fclose(fp);
   }




   DIR *dp;
   struct dirent *ep;
   dp = opendir ("data/board");
   if (dp != NULL)
    {
      while (ep = readdir (dp))
       {
         if (strcmp(ep->d_name,".")==0)  { } else
         if (strcmp(ep->d_name,"..")==0) { } else
            {
              //fprintf(stderr,"Adding board %s \n",ep->d_name);
              addBoardToSite( &ourSite , ep->d_name );
            }
       }
      closedir (dp);
    } else
    {
     fprintf(stderr,"Cannot open directory to list channels \n");
    }



  return 1;
}

int unloadSite()
{

  hashMap_Destroy( boardHashMap );
  hashMap_Destroy( threadHashMap );

  free(threadIndexPage);
  return 1;
}




void fillTimestampNow( struct timestamp * t )
{
  if (t==0) { return; }
  time_t now = time(0);
  struct tm * lt = localtime(&now);
  t->year   = lt->tm_year+1900;
  t->month  = lt->tm_mon+1;
  t->day    = lt->tm_mday;
  t->hour   = lt->tm_hour;
  t->minute = lt->tm_min;
  t->second = lt->tm_sec;
}


//Only allow a small whitelist of image extensions to be written to disk , everything else gets dropped
static int getSafeImageExtension(const char * originalFilename , char * outExt , unsigned int outExtSize)
{
  if ( (originalFilename==0) || (outExt==0) ) { return 0; }

  const char * dot = strrchr(originalFilename,'.');
  if (dot==0) { return 0; }
  ++dot;
  if (strlen(dot)==0) { return 0; }

  char ext[16]={0};
  unsigned int i=0;
  for (i=0; (dot[i]!=0) && (i<sizeof(ext)-1); i++) { ext[i]=tolower((unsigned char) dot[i]); }
  ext[i]=0;

  if ( (strcmp(ext,"jpg")==0)  || (strcmp(ext,"jpeg")==0) ||
       (strcmp(ext,"png")==0)  || (strcmp(ext,"gif")==0) )
  {
    snprintf(outExt,outExtSize,"%s",ext);
    return 1;
  }

  return 0;
}


//Builds the on-disk filename an image is/will be stored as , independent of whatever name it was originally uploaded with.
//Both addPostToThread ( at save time ) and loadPostHeader ( at load time ) call this with the same arguments so they
//always agree on the filename , without needing to persist a second field in header_N..!
void deriveCachedImageName( const char * originalFilename , unsigned int postIndex , char * outCachedName , unsigned int outCachedNameSize )
{
  char ext[16]="jpg"; //Legacy default : every image HabChan ever saved before fileCachedName existed was a plain image_N.jpg

  if (originalFilename!=0)
  {
    const char * dot = strrchr(originalFilename,'.');
    if ( (dot!=0) && (strlen(dot+1)>0) )
    {
      char candidate[16]={0};
      unsigned int i=0;
      for (i=0; (dot[1+i]!=0) && (i<sizeof(candidate)-1); i++) { candidate[i]=tolower((unsigned char) dot[1+i]); }
      candidate[i]=0;

      if ( (strcmp(candidate,"jpg")==0)  || (strcmp(candidate,"jpeg")==0) ||
           (strcmp(candidate,"png")==0)  || (strcmp(candidate,"gif")==0) )
      {
        snprintf(ext,sizeof(ext),"%s",candidate);
      }
    }
  }

  snprintf(outCachedName,outCachedNameSize,"image_%u.%s",postIndex,ext);
}


int addPostToThread( const char * boardName ,  struct thread * newThread ,  struct post * newPost , const char * fileBytes , unsigned int fileBytesSize )
{
  if ( (boardName==0) || (newThread==0) || (newPost==0) ) { fprintf(stderr,"addPostToThread called with incorrect parameters\n"); return 0; }
  if ( newThread->replies==0 ) { fprintf(stderr,"addPostToThread : thread `%s` has no allocated replies\n",newThread->name); return 0; }
  if ( newThread->numberOfReplies >= newThread->maxNumberOfReplies ) { fprintf(stderr,"addPostToThread : thread `%s` is full\n",newThread->name); return 0; }

  unsigned int postIndex = newThread->numberOfReplies;
  struct post * storedPost = &newThread->replies[postIndex];
  memset(storedPost,0,sizeof(struct post));

  snprintf(storedPost->op,MAX_STRING_SIZE,"%s",newPost->op);
  snprintf(storedPost->password,MAX_STRING_SIZE,"%s",newPost->password);
  fillTimestampNow(&storedPost->creation);

  char ext[16]={0};
  if ( newPost->hasFile && (fileBytes!=0) && (fileBytesSize>0) &&
       getSafeImageExtension(newPost->fileOriginalName,ext,sizeof(ext)) )
  {
    snprintf(storedPost->fileOriginalName,MAX_STRING_SIZE,"%s",newPost->fileOriginalName);
    deriveCachedImageName(storedPost->fileOriginalName,postIndex,storedPost->fileCachedName,MAX_STRING_SIZE);

    char imagePath[MAX_STRING_SIZE*2]={0};
    snprintf(imagePath,sizeof(imagePath),"data/board/%s/%s/%s",boardName,newThread->name,storedPost->fileCachedName);

    if ( AmmServer_WriteFileFromMemory(imagePath,fileBytes,fileBytesSize) )
    {
      storedPost->hasFile=1;
      storedPost->fileType=FILETYPE_IMAGE;
      ++newThread->numberOfImages;
    } else
    {
      fprintf(stderr,"addPostToThread : failed to write image `%s`\n",imagePath);
      storedPost->hasFile=0;
      storedPost->fileOriginalName[0]=0;
      storedPost->fileCachedName[0]=0;
    }
  } else
  if ( newPost->hasFile )
  {
    fprintf(stderr,"addPostToThread : rejected attachment `%s` , unsupported extension\n",newPost->fileOriginalName);
  }

  char postHeaderFilename[MAX_STRING_SIZE*2]={0};
  snprintf(postHeaderFilename,sizeof(postHeaderFilename),"data/board/%s/%s/header_%u",boardName,newThread->name,postIndex);
  savePostHeader(postHeaderFilename,storedPost);

  char postFilename[MAX_STRING_SIZE*2]={0};
  snprintf(postFilename,sizeof(postFilename),"data/board/%s/%s/post_%u",boardName,newThread->name,postIndex);
  storedPost->message = (newPost->message!=0) ? strdup(newPost->message) : strdup("");
  storedPost->messageSize = strlen(storedPost->message);
  savePostContent(postFilename,storedPost);

  ++newThread->numberOfReplies;
  fillTimestampNow(&newThread->lastReply);
  saveThreadStatus(boardName,newThread);

  return 1;
}












