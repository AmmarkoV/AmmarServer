#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AString.h"

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */


/*
struct AmmServer_MemoryHandler
{
  unsigned int contentSize;
  unsigned int contentCurrentLength;
  char * content;
};
*/

int myStupidMemcpy(char * target , char * source , unsigned int sourceLength)
{
  while (sourceLength>0)
  {
    *target=*source;
    --sourceLength;
  }

 return 1;
}

int astringInjectDataToMemoryHandlerOffset(struct AmmServer_MemoryHandler * mh,unsigned int *offset,const char * var,const char * value)
{
  fprintf(stderr,"We want to inject \n Value=`%s` \n to \n Var=`%s` \n",value,var);

  if (value==0)        { fprintf(stderr,"injectDataToBuffer / Zero Data To Inject we are happy..\n"); return 1; }
  if (var==0)  { fprintf(stderr,"injectDataToBuffer / No entry point defined..\n");           return 0; }
  if (mh==0)      { fprintf(stderr,"injectDataToBuffer / No Buffer To inject to..\n");           return 0; }

 /*
   WE HAVE :                              PART_TO_BE_MOVED
         S <----------------> VAR <-------------------------------> OLDEND

   WE WANT :                               PART_TO_BE_MOVED
         S <----------------> VALUE <-------------------------------> NEWEND
 */

 //We need to know how long is our value and variable
 unsigned int valueLength = strlen(value);
 unsigned int varLength = strlen(var);

 //Take advantage of offset when searching :) , this makes search faster
 char * where2start = mh->content + *offset;

 char * where2inject = (unsigned char* ) strstr ((const char*) where2start ,(const char*) var);
 mh->lastOperationPosition_NOT_ThreadSafe_Var = where2inject ; // Remember where we did our last operation..!
  if (where2inject==0) { fprintf(stderr,"Cannot inject Data to Buffer , could not find our entry point!\n"); return 0; }
 unsigned int injectOffset = where2inject - mh->content;

 unsigned int partToBeMovedLength = mh->contentCurrentLength - injectOffset - varLength;

 if (mh->contentCurrentLength + partToBeMovedLength + 1 > mh->contentSize )
 {
  fprintf(stderr,"Reallocating buffer to astringInjectDataToMemoryHandler \n");
  char * newBuffer = realloc( mh->content , mh->contentCurrentLength + partToBeMovedLength + 1);
  if (newBuffer==0) { fprintf(stderr,"Could not Inject #1\n"); return 0; }

  where2inject = newBuffer + injectOffset; //We recalculate the pointer for where 2 inject based on the new reallocated buffer..!

  mh->content = newBuffer;
  mh->contentSize=mh->contentCurrentLength + partToBeMovedLength + 1;
 }

 //We want to allocate enough space for the part to be moved
 char * partToBeMoved = (char* ) malloc( (partToBeMovedLength+1) * sizeof(char));
 if (partToBeMoved==0) { fprintf(stderr,"Could not allocate enough space for the part to be moved\n"); return 0; }

 //We save the part to be moved @ partToBeMoved
 memcpy(partToBeMoved,where2inject+varLength,partToBeMovedLength);
 //fprintf(stderr,"End Part is `%s`\n \n",partToBeMoved);


 //We write our value..
 memcpy(where2inject,value,valueLength);

 //We append the partToBeMoved
 memcpy(where2inject+valueLength,partToBeMoved,partToBeMovedLength);

 //Remember injection offset..!
 *offset = injectOffset;

 mh->contentCurrentLength += valueLength;
 mh->content[mh->contentCurrentLength]=0; //Make sure that the end is clearly signaled

 free(partToBeMoved);


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
  char * buffer = (char*) malloc (sizeof(char)*lSize);
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
