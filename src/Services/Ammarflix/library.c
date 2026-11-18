#define _GNU_SOURCE //strcasestr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

#include "library.h"

#define MAX_SCAN_DEPTH 32

struct VideoList
{
  struct Video * videos;
  unsigned int count;
  unsigned int capacity;
};

static char roots[MAX_MEDIA_ROOTS][MAX_FILE_PATH+1];
static unsigned int rootCount=0;

static struct VideoList library={0};
static pthread_mutex_t libraryLock=PTHREAD_MUTEX_INITIALIZER;

static const char * videoExtensions[]={ "mkv","mp4","m4v","webm","mov","ogv",0 };


int library_addRoot(const char * directory)
{
  if (rootCount>=MAX_MEDIA_ROOTS) { return 0; }

  char resolved[PATH_MAX];
  if (realpath(directory,resolved)==0) { return 0; }
  if (strlen(resolved)>MAX_FILE_PATH) { return 0; }

  struct stat st;
  if ( (stat(resolved,&st)!=0) || (!S_ISDIR(st.st_mode)) ) { return 0; }

  snprintf(roots[rootCount],sizeof(roots[rootCount]),"%s",resolved);
  ++rootCount;
  return 1;
}

unsigned int library_rootCount() { return rootCount; }
const char * library_getRoot(unsigned int i) { return (i<rootCount) ? roots[i] : 0; }


static void hashPath(const char * path,char * outID)
{
  //FNV-1a 64bit
  unsigned long long hash=1469598103934665603ULL;
  while (*path) { hash^=(unsigned char) *path; hash*=1099511628211ULL; ++path; }
  snprintf(outID,VIDEO_ID_SIZE,"%016llx",hash);
}

static int hasVideoExtension(const char * name)
{
  const char * dot=strrchr(name,'.');
  if (dot==0) { return 0; }
  unsigned int i=0;
  for (i=0; videoExtensions[i]!=0; i++)
   { if (strcasecmp(dot+1,videoExtensions[i])==0) { return 1; } }
  return 0;
}

static int addVideo(struct VideoList * list,const char * path,const char * name,const char * folder,const struct stat * st)
{
  if (list->count>=list->capacity)
  {
    unsigned int newCapacity = (list->capacity==0) ? 256 : list->capacity*2;
    struct Video * grown = (struct Video *) realloc(list->videos,sizeof(struct Video)*newCapacity);
    if (grown==0) { return 0; }
    list->videos=grown;
    list->capacity=newCapacity;
  }

  struct Video * v = &list->videos[list->count];
  memset(v,0,sizeof(struct Video));
  snprintf(v->path,sizeof(v->path),"%s",path);
  snprintf(v->folder,sizeof(v->folder),"%s",folder);
  hashPath(path,v->id);
  v->size=(unsigned long long) st->st_size;
  v->mtime=st->st_mtime;

  //Title : file name without extension , "The.Movie_2001" -> "The Movie 2001"
  snprintf(v->title,sizeof(v->title),"%s",name);
  char * dot=strrchr(v->title,'.');
  if (dot!=0) { *dot=0; }
  char * c=v->title;
  while (*c) { if ( (*c=='.') || (*c=='_') ) { *c=' '; } ++c; }

  ++list->count;
  return 1;
}

static void scanDirectory(struct VideoList * list,const char * directory,const char * folder,unsigned int depth)
{
  if (depth>MAX_SCAN_DEPTH) { return; }

  DIR * dir=opendir(directory);
  if (dir==0) { return; }

  struct dirent * entry;
  while ( (entry=readdir(dir))!=0 )
  {
    if (entry->d_name[0]=='.') { continue; } //Skip . , .. and hidden files

    char path[MAX_FILE_PATH+1];
    if (snprintf(path,sizeof(path),"%s/%s",directory,entry->d_name)>=(int) sizeof(path)) { continue; }

    //lstat : symlinks are skipped , they could point outside the media roots ( which can't be streamed ) or loop
    struct stat st;
    if (lstat(path,&st)!=0) { continue; }

    if (S_ISDIR(st.st_mode))
    {
      char subFolder[512];
      if (snprintf(subFolder,sizeof(subFolder),"%s/%s",folder,entry->d_name)>=(int) sizeof(subFolder)) { continue; }
      scanDirectory(list,path,subFolder,depth+1);
    } else
    if ( (S_ISREG(st.st_mode)) && (hasVideoExtension(entry->d_name)) )
    {
      addVideo(list,path,entry->d_name,folder,&st);
    }
  }
  closedir(dir);
}

static int compareVideos(const void * a,const void * b)
{
  const struct Video * va=(const struct Video *) a;
  const struct Video * vb=(const struct Video *) b;
  int result=strcasecmp(va->title,vb->title);
  if (result!=0) { return result; }
  return strcmp(va->path,vb->path);
}

unsigned int library_scan()
{
  struct VideoList fresh={0};

  unsigned int i=0;
  for (i=0; i<rootCount; i++)
  {
    const char * rootName=strrchr(roots[i],'/');
    rootName = ( (rootName!=0) && (rootName[1]!=0) ) ? rootName+1 : roots[i];
    scanDirectory(&fresh,roots[i],rootName,0);
  }

  if (fresh.count>0) { qsort(fresh.videos,fresh.count,sizeof(struct Video),compareVideos); }

  pthread_mutex_lock(&libraryLock);
   struct VideoList old=library;
   library=fresh;
  pthread_mutex_unlock(&libraryLock);

  free(old.videos);
  return fresh.count;
}

void library_free()
{
  pthread_mutex_lock(&libraryLock);
   free(library.videos);
   memset(&library,0,sizeof(library));
  pthread_mutex_unlock(&libraryLock);
}

void library_lock()   { pthread_mutex_lock(&libraryLock); }
void library_unlock() { pthread_mutex_unlock(&libraryLock); }
unsigned int library_count() { return library.count; }
const struct Video * library_get(unsigned int i) { return (i<library.count) ? &library.videos[i] : 0; }

int library_find(const char * id,struct Video * out)
{
  int found=0;
  pthread_mutex_lock(&libraryLock);
  unsigned int i=0;
  for (i=0; i<library.count; i++)
  {
    if (strcmp(library.videos[i].id,id)==0)
    {
      *out=library.videos[i];
      found=1;
      break;
    }
  }
  pthread_mutex_unlock(&libraryLock);
  return found;
}

int library_matches(const struct Video * video,const char * query)
{
  char word[256];
  const char * c=query;
  while (*c)
  {
    while (*c==' ') { ++c; }
    if (*c==0) { break; }

    unsigned int length=0;
    while ( (*c!=0) && (*c!=' ') ) { if (length<sizeof(word)-1) { word[length++]=*c; } ++c; }
    word[length]=0;

    if ( (strcasestr(video->title,word)==0) && (strcasestr(video->folder,word)==0) ) { return 0; }
  }
  return 1;
}
