#include "captcha.h"



char * generateImage(char * content,unsigned int width,unsigned int height)
{
  char * frame = (char*) malloc(width*height*3);
  if (frame==0) { return 0; }





  free(frame);
  frame=0;
  return frame;
}

