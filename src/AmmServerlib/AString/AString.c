#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --------------------------------------------
#include "AString.h"
#include "../server_configuration.h"
// --------------------------------------------

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

int reverseSyncMemcpy(char * target , char * source , unsigned int sourceLength)
{
    if (sourceLength==0)
    {
        return 0;
    }
    char * sourcePtr = source + sourceLength-1;
    //fprintf(stderr,"reverseSyncMemcpy : Final Source , %u \n" , (unsigned int) *sourcePtr);
    char * targetPtr = target + sourceLength-1;
    //fprintf(stderr,"reverseSyncMemcpy : Final Target , %u \n" , (unsigned int) *targetPtr);

    while (sourceLength>0)
    {
        //fprintf(stderr,"Loop %u  ( %u <= %u )\n",sourceLength,*targetPtr,*sourcePtr);
        *targetPtr=*sourcePtr;
        --targetPtr;
        --sourcePtr;
        --sourceLength;
    }
    return 1;
}

int straightSyncMemcpy(char * target , char * source , unsigned int sourceLength)
{
    while (sourceLength>0)
    {
        *target=*source;
        --sourceLength;
        ++target;
        ++source;
    }
    return 1;
}

int astringInjectDataToMemoryHandlerOffset(struct AmmServer_MemoryHandler * mh,unsigned int *offset,const char * var,const char * value)
{
    if (value==0)
    {
        fprintf(stderr,"injectDataToBuffer / Zero Data To Inject , we should just remove the var..\n");
        return 0;
    }
    if (var==0)
    {
        fprintf(stderr,"injectDataToBuffer / No entry point defined..\n");
        return 0;
    }
    if (mh==0)
    {
        fprintf(stderr,"injectDataToBuffer / No Buffer To inject to..\n");
        return 0;
    }
    /*
     We Have :
                     StartLength                      VarLength                         EndLength
     StartPtr <----------------------------> VarPtr <---VARIABLE---> EndPtr <-----------------------------------> NullTerminator

     and we want to change VARIABLE with a new VALUE
     There are 2 cases , if the VALUE is small then we can just go ahead and do the following


                     StartLength                      VarLength                         EndLength
     StartPtr <----------------------------> VarPtr <---VARIABLE---> EndPtr <-----------------------------------> NullTerminator

     memcpy( VarPtr , ValuePtr , ValueLength );
     memcpy( VarPtr+ValueLength , EndPtr , EndLength );
     contentCurrentLength = StartLength + ValueLength + EndLength;
     StartPtr[contentCurrentLength] = 0;


     ELSE


     if the Value is bigger than the variable things get trickier..!
     We need to reallocate a bigger buffer , store the things that do not fit and then proceed with copying
    */
//Take advantage of offset when searching :) , this makes search faster
    unsigned int injectOffset = 0;
    char * where2startSearchForVar = mh->content + *offset;

    char * varPtr   = ( char* ) strstr ((const char*) where2startSearchForVar  ,(const char*) var);
    if (varPtr==0)
    {
        fprintf(stderr,"Cannot inject Data to Buffer , could not find our entry point (%s) !\n",var);
        return 0;
    }
    else
    {
        //Remember injection offset..!
        injectOffset =varPtr - mh->content;
        *offset = injectOffset;
    }


//We need to know how long is our value and variable
    unsigned int valueLength = strlen(value);
    unsigned int varLength = strlen(var);

    #if DEBUG_MESSAGES
     unsigned int printDMesg=1;
    #else
     unsigned int printDMesg=0;
    #endif // DEBUG_MESSAGES

    if (printDMesg)
    { fprintf(stderr,"astringInjectDataToMemoryHandlerOffset ( contentSize = %u , contentCurrentLength = %u , offset %u )\n",mh->contentSize,mh->contentCurrentLength,*offset); }
     //fprintf(stderr,"astringInjectDataToMemoryHandlerOffset ( contentSize = %u , contentCurrentLength = %u , offset %u )  inject \n Value[%u]=`%s`  \n to \n Var[%u]=`%s`  \n",
     //        mh->contentSize,mh->contentCurrentLength,*offset,valueLength,value,varLength,var);

    char *       startPtr = mh->content;
    unsigned int startLength = varPtr-startPtr;
    char *       endPtr = varPtr+varLength;
    unsigned int endLength = mh->contentCurrentLength - (endPtr-startPtr);

    //If the value is small enough then we dont need to do a lot of stuff..!
    if (valueLength==varLength)
    {
        if (printDMesg) { fprintf(stderr,"No need for anything just copy and be done ..\n"); }
        memcpy( varPtr , value , valueLength );
    } else
    if (valueLength<=varLength)
    {
        if (printDMesg) { fprintf(stderr,"No need for reallocations etc..!\n"); }
        memcpy( varPtr , value , valueLength );
        straightSyncMemcpy( varPtr+valueLength , endPtr , endLength );
        mh->contentCurrentLength = startLength + valueLength + endLength;
        startPtr[mh->contentCurrentLength] = 0;
    }
    else
    {
      #if DO_NOT_ALLOW_MEMORY_REALLOCATIONS
        fprintf(stderr,"astringInjectDataToMemoryHandlerOffset : Realloc code is disabled in this build\n");
        return 0;
      #else
        #warning "astringInjectDataToMemoryHandlerOffset will try reallocations , not 100% this part of the code is sane..!"
      #endif // DO_NOT_ALLOW_MEMORY_REALLOCATIONS

      unsigned int extraBufferLength = valueLength - varLength;
      unsigned int reallocatedBufferSize = mh->contentSize + extraBufferLength;

      char * newBuffer = realloc( mh->content , reallocatedBufferSize + 1 ); //Also making space for null termination
        if (newBuffer==0)
        {
            fprintf(stderr,RED "astringInjectDataToMemoryHandlerOffset could not allocate extra space to accommodate variable\n" NORMAL);
            return 0;
        }
        else
        {
            newBuffer[mh->contentSize]=0; // Keep our new buffer clean at all times
            //We just realloced so we will now have to move all of our pointers to the new memory locations

            //==========================================================================
            //Clean the rest of the buffer ( debug code )
            unsigned int i=0;
            for (i=mh->contentSize; i<mh->contentSize+extraBufferLength; i++)
            {
                newBuffer[i]=0;
            }
            //==========================================================================

            mh->content = newBuffer;
            mh->contentCurrentLength += extraBufferLength;
            mh->contentSize=mh->contentCurrentLength;
            //Reallocate indexes to new buffer
            startPtr = newBuffer;
            varPtr =  newBuffer  + injectOffset;
            endPtr = varPtr+varLength;

            //fprintf(stderr,"We move the end further away , save our extra data ( %u ) to a memory block\n ",extraBufferLength);
            //fprintf(stderr,"We will move : \n `%s` \n TO \n `%s` \n \n ",varPtr+valueLength,varPtr+valueLength+extraBufferLength);
            reverseSyncMemcpy(varPtr+valueLength,varPtr+varLength,endLength);

            //We write our value..
            memcpy(varPtr,value,valueLength);

            //We append Null Terminator..
            startPtr[mh->contentCurrentLength]=0;
        }
    }

    return 1;
}

int astringInjectDataToMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value)
{
    unsigned int offset = 0;
    return astringInjectDataToMemoryHandlerOffset(mh,&offset,var,value);
}

int astringReplaceAllInstancesOfVarInMemoryFile(struct AmmServer_MemoryHandler * mh,unsigned int instances,const char * var,const char * value)
{
    if ( (mh==0) || (mh->content==0) || (instances==0) || (var==0) || (value==0) )
    {
        return 0;
    }

    unsigned int offset = 0;
    unsigned int remainingInstances=instances;

    while  (remainingInstances>0)
    {
        if ( astringInjectDataToMemoryHandlerOffset(mh,&offset,var,value) )
        {
            --remainingInstances;
        }
        else
        {
            break;
        }
    }

    return (remainingInstances==0);
}
/*
    -------------- -------------- -------------- -------------- -------------- --------------
    -------------- -------------- -------------- -------------- -------------- --------------
                         The rest are easy , read / write operations
    -------------- -------------- -------------- -------------- -------------- --------------
    -------------- -------------- -------------- -------------- -------------- --------------

*/
char * astringReadFileToMemory(const char * filename,unsigned int *length )
{
    *length = 0;
    FILE * pFile = fopen ( filename , "rb" );

    if (pFile==0)
    {
        fprintf(stderr,RED "AmmServer_ReadFileToMemory failed\n" NORMAL);
        fprintf(stderr,RED "Could not read file %s \n" NORMAL,filename);
        return 0;
    }

    // obtain file size:
    fseek (pFile , 0 , SEEK_END);
    unsigned long lSize = ftell (pFile);
    rewind (pFile);

    // allocate memory to contain the whole file:
    char * buffer = (char*) malloc (sizeof(char)*(lSize+2));
    if (buffer == 0 )
    {
        fprintf(stderr,RED "Could not allocate enough memory for file %s \n" NORMAL,filename);
        fclose(pFile);
        return 0;
    }

    // copy the file into the buffer:
    size_t result = fread (buffer,1,lSize,pFile);
    if (result != lSize)
    {
        free(buffer);
        fprintf(stderr,RED "Could not read the whole file onto memory %s \n" NORMAL,filename);
        fclose(pFile);
        return 0;
    }

    /* the whole file is now loaded in the memory buffer. */

    // terminate
    fclose (pFile);

    buffer[lSize]=0; //Null Terminate Buffer!
    *length = (unsigned int) lSize;
    return buffer;
}

int astringWriteFileFromMemory(const char * filename,const char * memory , unsigned int memoryLength)
{
    if (memory==0)
    {
        fprintf(stderr,"Could not write null memory buffer\n");
        return 0 ;
    }
    if (memoryLength==0)
    {
        fprintf(stderr,"Could not write empty memory buffer\n");
        return 0 ;
    }

    FILE * pFile=0;
    size_t result;

    pFile = fopen ( filename , "wb" );
    if (pFile==0)
    {
        fprintf(stderr,RED "Could not write file %s \n" NORMAL,filename);
        return 0;
    }

    result = fwrite (memory,1,memoryLength,pFile);
    if (result != memoryLength)
    {
        fprintf(stderr,RED "Could not write the whole file onto disk %s \n" NORMAL,filename);
        fprintf(stderr,RED "We wrote %zu / %u  \n" NORMAL,result,memoryLength);
        fclose(pFile);
        return 0;
    }

    // terminate
    fclose (pFile);
    return 1;
}
