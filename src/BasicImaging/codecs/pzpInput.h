#ifndef _PZPINPUT_H_INCLUDED
#define _PZPINPUT_H_INCLUDED

#include "codecs.h"

#if USE_PZP_FILES

int ReadPZPMemory(const char *mem, unsigned int memSize, struct Image *pic);
int ReadPZP(const char *filename, struct Image *pic);
int WritePZP(const char *filename, struct Image *pic);
#endif // USE_PZP_FILES

#endif

