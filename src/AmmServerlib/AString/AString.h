/** @file AString.h
* @brief A small toolset to handle long strings manage memory and append,inject other strings inside them
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef ASTRING_H_INCLUDED
#define ASTRING_H_INCLUDED


#include "../AmmServerlib.h"

int astringReplaceVarInMemoryFile(char * page,unsigned int pageLength,const char * var,const char * value);
int astringReplaceAllInstancesOfVarInMemoryFile(char * page,unsigned int instances,unsigned int pageLength,const char * var,const char * value);

char * astringReadFileToMemory(const char * filename,unsigned int *length );

int astringWriteFileFromMemory(const char * filename,char * memory , unsigned int memoryLength);

//int astringCopyOverlappingDataContent(unsigned char * buffer , unsigned int totalSize  , unsigned char * from , unsigned char * to , unsigned int blockSize);

int astringInjectDataToBuffer(unsigned char * entryPoint , unsigned char * data , unsigned char * buffer,  unsigned int currentBufferLength , unsigned int totalBufferLength);


/**
* @brief Inject a String inside a memory handler
* @ingroup astring
* @param Pointer to a MemoryHandler struct that contains the buffer we want to modify
* @param needle we want to search in the haystack to replace
* @param String that will replace existing one
* @retval 1=Success,0=Fail
* @bug This does not yet reallocate the buffer to make it bigger in case it is not big enough to accomodate the new string..
*/
int astringInjectDataToMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value);

#endif // ASTRING_H_INCLUDED
