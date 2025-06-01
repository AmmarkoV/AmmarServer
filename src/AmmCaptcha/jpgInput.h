#ifndef _JPGINPUT_H_INCLUDED
#define _JPGINPUT_H_INCLUDED


#ifdef __cplusplus
extern "C"
{
#endif


#include "imaging.h"
//#include "codecs.h"

#define USE_JPG_FILES 1

int ReadJPEG(const char *filename,struct Image * pic,char read_only_header);

int ReadJPEGMem(unsigned char *buffer, unsigned int bufferSize, struct Image *pic, char read_only_header);

int WriteJPEGFile(struct Image * pic,const char *filename);
int WriteJPEGMemory(struct Image * pic,char *mem,unsigned long * mem_size); //,int quality

#ifdef __cplusplus
}
#endif


#endif // _JPG_H_INCLUDED
