#ifndef _JPGINPUT_H_INCLUDED
#define _JPGINPUT_H_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif


#include "codecs.h"
//Note : USE_JPG_FILES is defined ( with an #ifndef guard ) in codecs.h , which this file already
//includes above ; it must NOT be redefined here unconditionally , or it would silently force JPG
//support back on even on a build where BasicImaging's CMakeLists.txt detected libjpeg is missing
//and passed USE_JPG_FILES=0 in.

int ReadJPEG(const char *filename,struct Image * pic,char read_only_header);

int ReadJPEGMem(unsigned char *buffer, unsigned int bufferSize, struct Image *pic, char read_only_header);

int WriteJPEGFile(struct Image * pic,const char *filename);
int WriteJPEGMemory(struct Image * pic,char *mem,unsigned long * mem_size,int quality);

//Not part of the upstream header : lets BasicImaging write a file at a caller-chosen quality instead
//of WriteJPEGFile()'s hardcoded 75 , without duplicating WriteJPEGInternal()'s buffer/file handling.
int WriteJPEGInternal(const char *filename,struct Image * pic,char *mem,unsigned long * mem_size,int quality);

#ifdef __cplusplus
}
#endif


#endif // _JPG_H_INCLUDED
