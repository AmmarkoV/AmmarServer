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
* @bug astringInjectDataToMemoryHandler is not implemented correctly , it is be buggy..!
*/
int astringInjectDataToMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value);

/**
* @brief Replace All Instances of a Variable inside a Memory handler
* @ingroup astring
* @param Pointer to a MemoryHandler struct that contains the buffer we want to modify
* @param Number of instances we want to replace ( upon reaching this number we will stop )
* @param needle we want to search in the haystack to replace
* @param String that will replace existing one
* @retval 1=Success,0=Fail
* @bug astringReplaceAllInstancesOfVarInMemoryFile is not implemented correctly , it is be buggy..!
*/
int astringReplaceAllInstancesOfVarInMemoryFile(struct AmmServer_MemoryHandler * mh,unsigned int instances,const char * var,const char * value);


/**
* @brief Allocate a memory block and read a file into it
* @ingroup astring
* @param Pointer to path of the file we want to read
* @param Pointer to output length of newly allocated memory buffer
* @retval Pointer to memory allocation,0=Fail
*/
char * astringReadFileToMemory(const char * filename,unsigned int *length );


/**
* @brief Write a memory block to a file
* @ingroup astring
* @param Pointer to path of the file we want to read
* @param Pointer to memory buffer
* @param Size of memory buffer
* @retval 1=Success,0=Fail
*/
int astringWriteFileFromMemory(const char * filename,const char * memory , unsigned int memoryLength);


#endif // ASTRING_H_INCLUDED
