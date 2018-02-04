#ifndef FRAMEBUFFER_H_INCLUDED
#define FRAMEBUFFER_H_INCLUDED

#include <pthread.h>

struct imageStorage
{
  pthread_mutex_t accessMutex;
  char * data;
  unsigned int dataSize;
  unsigned int width;
  unsigned int height;
  unsigned int framenumber;
  unsigned int depth;

};

int storeImage(struct imageStorage * is , int id , char * data,unsigned int dataSize);

#endif // FRAMEBUFFER_H_INCLUDED
