#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AString.h"



int astringReplaceVarInMemoryFile(char * page,unsigned int pageLength,const char * var,const char * value)
{
  if (page==0) { fprintf(stderr,"Replacing var in empty page\n"); return 0; }
  if (var==0) { fprintf(stderr,"Given an empty variable to replace\n"); return 0; }

  unsigned int varLength = strlen(var);
  unsigned int valueLength = strlen(value);
  if (varLength>pageLength) { fprintf(stderr,"This variable is larger than the whole page\n"); return 0; }
  if (varLength<valueLength) { fprintf(stderr,"This variable payload is larger than the variable, this is not implemented yet\n"); return 0; }

  char * location = strstr(page,var);
  if (location == 0 ) { return 0; }
  char * scanEnd = location+valueLength;
  char * locationEnd = location+varLength;
  const char * curValue = value;

  while (location<scanEnd)
  {
    *location = *curValue;
    ++location;
    ++curValue;
  }

  while (location<locationEnd)
  {
    *location = ' ';
    ++location;
  }

  return 1;
}




char * astringReadFileToMemory(const char * filename,unsigned int *length )
{
  *length = 0;
  FILE * pFile = fopen ( filename , "rb" );

  if (pFile==0)
     {
       fprintf(stderr,"AmmServer_ReadFileToMemory failed");
       fprintf(stderr,"Could not read file %s \n",filename);
       return 0;
     }

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  unsigned long lSize = ftell (pFile);
  rewind (pFile);

  // allocate memory to contain the whole file:
  char * buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == 0 ) { fprintf(stderr,"Could not allocate enough memory for file %s ",filename); fclose(pFile); return 0; }

  // copy the file into the buffer:
  size_t result = fread (buffer,1,lSize,pFile);
  if (result != lSize)
    {
      free(buffer);
      fprintf(stderr,"Could not read the whole file onto memory %s ",filename);
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



int astringCopyOverlappingDataContent(unsigned char * buffer , unsigned int totalSize  , unsigned char * from , unsigned char * to , unsigned int blockSize)
{
  unsigned char * tmp = (unsigned char * ) malloc(sizeof(unsigned char) * blockSize);
  if (tmp==0) { return 0; }

     memcpy(tmp,from,blockSize);
     memcpy(to,tmp,blockSize);

  free(tmp);
  return 1;
}

int astringInjectDataToBuffer(unsigned char * entryPoint , unsigned char * data , unsigned char * buffer,  unsigned int currentBufferLength , unsigned int totalBufferLength )
{
  if (data==0)        { fprintf(stderr,"injectDataToBuffer / Zero Data To Inject we are happy..\n"); return 1; }
  if (entryPoint==0)  { fprintf(stderr,"injectDataToBuffer / No entry point defined..\n");           return 0; }
  if (buffer==0)      { fprintf(stderr,"injectDataToBuffer / No Buffer To inject to..\n");           return 0; }


  unsigned int dataLength = strlen(data);
  if (dataLength + currentBufferLength >= totalBufferLength )
  {
    fprintf(stderr,"Not enough space for data injection on buffer.. ( todo realloc here.. ) \n");
    return 0;
  }

  unsigned int entryPointLength = strlen(entryPoint);
  unsigned char * where2inject = (unsigned char* ) strstr ((const char*) buffer,(const char*) entryPoint);
  if (where2inject==0) { fprintf(stderr,"Cannot inject Data to Buffer , could not find our entry point!\n"); return 0; }


  //We create space for our new data..
  if ( AmmServer_CopyOverlappingDataContent(buffer,totalBufferLength,where2inject,where2inject+entryPointLength,dataLength) )
  {
    //If we have enough space , we inject our data and everyone is happy
    memcpy(where2inject,data,dataLength);
    return 1;
  }
  else
  {
    fprintf(stderr,"Could not find enough space to reallocate old data .. Injection failed \n");
  }


  return 0;
}
