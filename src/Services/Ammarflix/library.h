#ifndef LIBRARY_H_INCLUDED
#define LIBRARY_H_INCLUDED

#include <time.h>
#include "../../AmmServerlib/AmmServerlib.h"

#define MAX_MEDIA_ROOTS 16
#define VIDEO_ID_SIZE 17 //16 hex chars + NUL

struct Video
{
  char id[VIDEO_ID_SIZE];          //Hash of the path , stable across rescans
  char path[MAX_FILE_PATH+1];      //Absolute path of the file
  char title[256];                 //File name without extension , dots/underscores as spaces
  char folder[512];                //Media root name + sub directory , for display
  unsigned long long size;
  time_t mtime;
};

//Registers a media directory to scan ( stored resolved , call before library_scan )
int library_addRoot(const char * directory);
unsigned int library_rootCount();
const char * library_getRoot(unsigned int i);

//Walks every media root and atomically replaces the index , returns the number of videos found
unsigned int library_scan();
void library_free();

//The index may only be walked between library_lock()/library_unlock()
void library_lock();
void library_unlock();
unsigned int library_count();
const struct Video * library_get(unsigned int i);

//Copies the video with this id into out , locks internally , 1=found
int library_find(const char * id,struct Video * out);

//1 if every whitespace separated word of query appears ( case insensitive ) in the title or folder
int library_matches(const struct Video * video,const char * query);

#endif // LIBRARY_H_INCLUDED
