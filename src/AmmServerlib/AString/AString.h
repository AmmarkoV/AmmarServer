/** @file AString.h
* @brief A small toolset to handle long strings manage memory and append,inject other strings inside them
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef ASTRING_H_INCLUDED
#define ASTRING_H_INCLUDED


#include "../AmmServerlib.h"




/**
* @brief Inject a String inside a memory handler
* @ingroup astring
* @param Pointer to a MemoryHandler struct that contains the buffer we want to modify
* @param needle we want to search in the haystack to replace
* @param String that will replace existing one
* @retval 1=Success,0=Fail
* @bug astringInjectDataToMemoryHandler is not implemented correctly and can corrupt the data (!) , work needed here
*/
int astringInjectDataToMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value);



int astringReplaceAllInstancesOfVarInMemoryFile(struct AmmServer_MemoryHandler * mh,unsigned int instances,const char * var,const char * value);



char * astringReadFileToMemory(const char * filename,unsigned int *length );
int astringWriteFileFromMemory(const char * filename,char * memory , unsigned int memoryLength);


#endif // ASTRING_H_INCLUDED
