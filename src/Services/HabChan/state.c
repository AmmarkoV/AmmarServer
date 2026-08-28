
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


//Determines the REAL type of an uploaded file by sniffing its magic bytes , rather than trusting the extension
//on the filename the browser sent us ( which is trivial to fake , e.g. naming an arbitrary file "x.jpg" ).
//Only files that are genuinely one of these 3 formats are ever accepted for storage.
int detectImageType(const char * bytes , unsigned int size , char * outExt , unsigned int outExtSize)
{
  if ( (bytes==0) || (outExt==0) ) { return 0; }

  if ( (size>=3) && ((unsigned char)bytes[0]==0xFF) && ((unsigned char)bytes[1]==0xD8) && ((unsigned char)bytes[2]==0xFF) )
  {
    snprintf(outExt,outExtSize,"jpg");
    return 1;
  }

  if ( (size>=8) && (memcmp(bytes,"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",8)==0) )
  {
    snprintf(outExt,outExtSize,"png");
    return 1;
  }

  if ( (size>=6) && ( (memcmp(bytes,"GIF87a",6)==0) || (memcmp(bytes,"GIF89a",6)==0) ) )
  {
    snprintf(outExt,outExtSize,"gif");
    return 1;
  }

  return 0;
}


//Thumbnails live right next to the full image , named the same way just with the "image_" prefix swapped for
//"thumb_" , so no extra bookkeeping ( or header_N field ) is needed to know a thumbnail's name from the full one.
void deriveThumbnailName( const char * cachedImageName , char * outThumbName , unsigned int outThumbNameSize )
{
  const char * rest = cachedImageName;
  if ( (cachedImageName!=0) && (strncmp(cachedImageName,"image_",6)==0) ) { rest = cachedImageName+6; }
  snprintf(outThumbName,outThumbNameSize,"thumb_%s",(rest!=0)?rest:"");
}


//Best effort thumbnail generation via ImageMagick's `convert` , which is not a hard dependency : if it is not
//installed or fails for any reason , rendering simply falls back to serving the full sized image instead.
static int generateThumbnail(const char * sourcePath , const char * thumbPath)
{
  char command[MAX_STRING_SIZE*4]={0};
  snprintf(command,sizeof(command),"convert '%s' -resize 200x200 '%s' >/dev/null 2>&1",sourcePath,thumbPath);

  char scratch[16]={0};
  AmmServer_ExecuteCommandLine(command,scratch,sizeof(scratch));

  return AmmServer_FileExists(thumbPath);
}


//Fallback used only by loadPostHeader() for header_N files written before "imagecached(...)" existed , where the
//real on-disk filename was never recorded and has to be guessed from the ( untrusted , possibly wrong ) original
//upload name instead. addPostToThread() knows the real type from detectImageType() and never needs to guess.
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
       detectImageType(fileBytes,fileBytesSize,ext,sizeof(ext)) )
  {
    snprintf(storedPost->fileOriginalName,MAX_STRING_SIZE,"%s",newPost->fileOriginalName);
    snprintf(storedPost->fileCachedName,MAX_STRING_SIZE,"image_%u.%s",postIndex,ext);

    char imagePath[MAX_STRING_SIZE*2]={0};
    snprintf(imagePath,sizeof(imagePath),"data/board/%s/%s/%s",boardName,newThread->name,storedPost->fileCachedName);

    if ( AmmServer_WriteFileFromMemory(imagePath,fileBytes,fileBytesSize) )
    {
      storedPost->hasFile=1;
      storedPost->fileType=FILETYPE_IMAGE;
      ++newThread->numberOfImages;

      char thumbName[MAX_STRING_SIZE]={0};
      deriveThumbnailName(storedPost->fileCachedName,thumbName,sizeof(thumbName));
      char thumbPath[MAX_STRING_SIZE*2]={0};
      snprintf(thumbPath,sizeof(thumbPath),"data/board/%s/%s/%s",boardName,newThread->name,thumbName);
      generateThumbnail(imagePath,thumbPath); //Best effort : a full-size image is still shown if this fails
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
    fprintf(stderr,"addPostToThread : rejected attachment `%s` , not a recognized jpg/png/gif file\n",newPost->fileOriginalName);
    newPost->hasFile=0; //So the caller can tell the attachment did not make it in and say so
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












