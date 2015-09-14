#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AString.h"

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */


int reverseSyncMemcpy(char * target , char * source , unsigned int sourceLength)
{
  if (sourceLength==0) { return 0; }
  char * sourcePtr = source + sourceLength;
  char * targetPtr = target + sourceLength;
  while (sourceLength>0)
  {
    *targetPtr=*sourcePtr;
    --sourceLength;
    --targetPtr;
    --sourcePtr;
  }
 *targetPtr=*sourcePtr;
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
  fprintf(stderr,"astringInjectDataToMemoryHandlerOffset , offset %u inject \n Value=`%s` \n to \n Var=`%s` \n",*offset,value,var);

  if (value==0) { fprintf(stderr,"injectDataToBuffer / Zero Data To Inject we are happy..\n"); return 1; }
  if (var==0)   { fprintf(stderr,"injectDataToBuffer / No entry point defined..\n");           return 0; }
  if (mh==0)    { fprintf(stderr,"injectDataToBuffer / No Buffer To inject to..\n");           return 0; }

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

 char * varPtr   = (unsigned char* ) strstr ((const char*) where2startSearchForVar  ,(const char*) var);
 if (varPtr==0)
    {
     fprintf(stderr,"Cannot inject Data to Buffer , could not find our entry point!\n");
     return 0;
    } else
    {
     //Remember injection offset..!
      injectOffset =varPtr - mh->content;
     *offset = injectOffset;
    }


 //We need to know how long is our value and variable
 unsigned int valueLength = strlen(value);
 unsigned int varLength = strlen(var);

 char *       valuePtr = (char*) value;
 //char *     varPtr  --- Calculated above
 char *       startPtr = mh->content;
 unsigned int startLength = varPtr-startPtr;
 char *       endPtr = varPtr+varLength;
 unsigned int endLength = strlen(endPtr);

 fprintf(stderr,"End Pointer %s \n\n\n END POINTER \n\n\n ",endPtr);


 //If the value is small enough then we dont need to do a lot of stuff..!
 if (valueLength<=varLength)
 {
   fprintf(stderr,"No need for reallocations etc..!\n");
   memcpy( varPtr , valuePtr , valueLength );
   straightSyncMemcpy( varPtr+valueLength , endPtr , endLength );
   mh->contentCurrentLength = startLength + valueLength + endLength;
   startPtr[mh->contentCurrentLength] = 0;
 } else
 {
  unsigned int extraBufferLength = valueLength - varLength;

  char * newBuffer = realloc( mh->content , mh->contentCurrentLength + extraBufferLength + 1);
  if (newBuffer==0)
    {
     fprintf(stderr,"Could not Inject #1\n");
     return 0;
    } else
    {
      mh->content = newBuffer;
      mh->contentSize=mh->contentCurrentLength + extraBufferLength;

      //We want to allocate enough space for the part to be moved
      char * extraBuffer = (char* ) malloc( (extraBufferLength+1) * sizeof(char));
      if (extraBuffer==0)
        {
          fprintf(stderr,"Could not allocate enough space for the part to be moved\n");
          return 0;
        } else
        {
         //We save our extra data to a memory block
         memcpy(extraBuffer,varPtr+varLength,extraBufferLength);
         extraBuffer[extraBufferLength]=0; // Null termination..


         //We move the end urther away
         reverseSyncMemcpy(varPtr+valueLength+extraBufferLength,varPtr+valueLength,endLength);

         //We write our value..
         memcpy(varPtr,value,valueLength);

         //We append the extraBuffer
         memcpy(varPtr+valueLength,extraBuffer,extraBufferLength);


         mh->contentCurrentLength += extraBufferLength;
         mh->content[mh->contentCurrentLength]=0;

         free(extraBuffer);
        }
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
  if ( (mh==0) || (mh->content==0) || (instances==0) || (var==0) || (value==0) ) { return 0; }

  unsigned int offset = 0;
  unsigned int remainingInstances=instances;

  while  (remainingInstances>0)
  {
    if ( astringInjectDataToMemoryHandlerOffset(mh,&offset,var,value) )
    {
      --remainingInstances;
    } else
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
       fprintf(stderr,RED "AmmServer_ReadFileToMemory failed" NORMAL);
       fprintf(stderr,RED "Could not read file %s \n" NORMAL,filename);
       return 0;
     }

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  unsigned long lSize = ftell (pFile);
  rewind (pFile);

  // allocate memory to contain the whole file:
  char * buffer = (char*) malloc (sizeof(char)*(lSize+1));
  if (buffer == 0 ) { fprintf(stderr,RED "Could not allocate enough memory for file %s " NORMAL,filename); fclose(pFile); return 0; }

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize)
    {
      free(buffer);
      fprintf(stderr,RED "Could not read the whole file onto memory %s " NORMAL,filename);
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


int astringWriteFileFromMemory(const char * filename,char * memory , unsigned int memoryLength)
{
  if (memory==0) { fprintf(stderr,"Could not write null memory buffer\n"); return 0 ; }
  FILE * pFile=0;
  size_t result;

  pFile = fopen ( filename , "wb" );
  if (pFile==0) { fprintf(stderr,"Could not write file %s \n",filename); return 0; }

  result = fwrite (memory,1,memoryLength,pFile);
  if (result != memoryLength)
     {
       fprintf(stderr,"Could not write the whole file onto disk %s \n",filename);
       fprintf(stderr,"We wrote %zu / %u  \n",result,memoryLength);
       fclose(pFile);
       return 0;
     }

  // terminate
  fclose (pFile);
  return 1;
}
