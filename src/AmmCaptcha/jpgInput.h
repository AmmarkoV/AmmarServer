#ifndef _JPGINPUT_H_INCLUDED
#define _JPGINPUT_H_INCLUDED

#include "imaging.h"

#define USE_JPG_FILES 1


#if USE_JPG_FILES

int ReadJPEG( char *filename,struct Image * pic,char read_only_header);

int WriteJPEGFile(struct Image * pic,char *filename);
int WriteJPEGMemory(struct Image * pic,char *mem,unsigned long * mem_size);

#endif

#endif // _JPG_H_INCLUDED
