#include "mmapBridge.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>


char * strstrDoubleNewline(char * request,unsigned int requestLength,unsigned int * endOfLine)
{
  if (request==0)             { return request; }
  if (requestLength==0)       { return request; }
  if (endOfLine==0)           { return request; }

  //HTTP ends its headers with \r\n\r\n , this used to look for \n\n only ( which never occurs in an HTTP response , so
  //the body was never found and whole responses were parsed ) , and returned the byte past any 0 it met as the "body"
  unsigned int i=0;
  for (i=0; i+1<requestLength; i++)
    {
      if (request[i]==0) { break; } //End of data
      unsigned int separator = 0;
      if ( (request[i]==10) && (request[i+1]==10) ) { separator=2; } else
      if ( (i+3<requestLength) && (request[i]==13) && (request[i+1]==10) && (request[i+2]==13) && (request[i+3]==10) ) { separator=4; }
      if (separator)
        {
         request[i]=0; //Also make null terminated string..
         *endOfLine = i;
         return request+i+separator;
        }
    }

  //fprintf(stderr,"not found\n");
 return request;
}


int initializeWritingBridge(struct bridgeContext * nbc ,const char * fileDescriptor,unsigned int sizeOfBridgeMsg)
{
  if (nbc==0)             { return 0; }
  if (fileDescriptor==0)  { return 0; }
  if (sizeOfBridgeMsg==0) { return 0; }

  fprintf(stderr,"initializeWritingBridge @ %s \n",fileDescriptor);
  memset(nbc,0,sizeof(struct bridgeContext ));
    /* Open a file for writing.
     *  - Creating the file if it doesn't exist.
     *  - Truncating it to 0 size if it already exists. (not really needed)
     *
     * Note: "O_WRONLY" mode is not sufficient when mmaping.
     */
 nbc->mode=0;

 //const char *fileDescriptor = "/tmp/NAOmmapped.bin";
 nbc->fd = open(fileDescriptor, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
 if (nbc->fd == -1)
    {
        perror("Error opening file for writing");
        return 0;
    }


 // Stretch the file size to the size of the (mmapped) array of char

    nbc->dataSize = sizeOfBridgeMsg; // + 1; // + \0 null character

    if (lseek(nbc->fd, nbc->dataSize-1, SEEK_SET) == -1) //-1
    {
        close(nbc->fd);
        perror("Error calling lseek() to 'stretch' the file");
        return 0;
    }

    /* Something needs to be written at the end of the file to
     * have the file actually have the new size.
     * Just writing an empty string at the current file position will do.
     *
     * Note:
     *  - The current position in the file is at the end of the stretched
     *    file due to the call to lseek().
     *  - An empty string is actually a single '\0' character, so a zero-byte
     *    will be written at the last byte of the file.
     */
   if (write(nbc->fd, "", 1) == -1)
    {
        close(nbc->fd);
        perror("Error writing last byte of the file");
        return 0;
    }


    // Now the file is ready to be mmapped.
    nbc->map = mmap(0, nbc->dataSize, PROT_READ | PROT_WRITE, MAP_SHARED, nbc->fd, 0);
    if (nbc->map == MAP_FAILED)
    {
        close(nbc->fd);
        nbc->fd=-1; nbc->map=0; //A later close*Bridge() must not close or unmap these again
        perror("Error mmapping the file");
        return 0;
    }


 nbc->mode=2;
 //fprintf(stderr,"initializeWritingBridge success \n");

 return 1;
}

int writeBridge(struct bridgeContext * nbc ,void * data , unsigned int dataSize , unsigned int waitForAcknowledgmentXMilliseconds)
{
  if (nbc==0)       { return 0; }
  if (data==0)      { return 0; }
  if (dataSize==0)  { return 0; }
  if (nbc->mode==0)
  {
    fprintf(stderr,"Cannot write to bridge , not properly initialized ..\n");
    return 0;
  }
  if (dataSize>nbc->dataSize) { fprintf(stderr,"Cannot write %u bytes to a %zu byte bridge\n",dataSize,nbc->dataSize); return 0; } //Was unchecked

  //unsigned int offset = 0;
  void * mmapPtr = nbc->map;
  if (mmapPtr!=0)
  {
     memcpy(mmapPtr , data , dataSize);

     // Write it to disk if needed
     //if (msync(nbc->map, nbc->dataSize, MS_SYNC) == -1)
     //{  perror("Could not sync the file to disk"); }
     if (waitForAcknowledgmentXMilliseconds)
      {
        fprintf(stderr,"Waiting for acknowledgment not implemented yet..\n");
      }

     return 1;
  }

 return 0;
}

int clearWritingBridge(struct bridgeContext * nbc)
{
if (nbc==0)  { return 0; }
if (nbc->mode==0)
  {
    fprintf(stderr,"Cannot write to bridge , not properly initialized ..\n");
    return 0;
  }

  printf("Writing New Empty State \n");
  //unsigned int offset = 0;

  void * mmapPtr = nbc->map;
  memset(mmapPtr , 0 , nbc->dataSize);
 return 1;
}


//Shared by both close calls : a bridge that was never ( successfully ) initialized has nothing to unmap , and its fd
//is 0 after the memset in initialize*Bridge() , which used to close stdin
static int closeBridge(struct bridgeContext * nbc)
{
    if (nbc==0) { return 0; }
    if (nbc->mode==0) { return 0; }
    nbc->mode=0;

    int result=1;
    // Don't forget to free the mmapped memory
    if (munmap(nbc->map, nbc->dataSize) == -1)
    {
        perror("Error un-mmapping the file");
        result=0;
    }

    // Un-mmaping doesn't close the file, so we still need to do that.
    close(nbc->fd);
    nbc->fd=-1;
    nbc->map=0;
   return result;
}

int closeWritingBridge(struct bridgeContext * nbc)
{
   return closeBridge(nbc);
}












int initializeReadingBridge(struct bridgeContext * nbc ,const char * fileDescriptor,unsigned int sizeOfBridgeMsg)
{
  if (nbc==0)             { return 0; }
  if (sizeOfBridgeMsg==0) { return 0; }
  if (fileDescriptor==0)  { return 0; }

  fprintf(stderr,"initializeReadingBridge @ %s \n",fileDescriptor);
  memset(nbc,0,sizeof(struct bridgeContext ));

  nbc->mode=0;
  nbc->fd = open(fileDescriptor, O_RDONLY, (mode_t)0600);

    if (nbc->fd == -1)
    {
        perror("Error opening file for writing");
        return 0;
    }

    nbc->dataSize = sizeOfBridgeMsg;
    printf("mmaping size is %ji\n", (intmax_t) nbc->dataSize);

    //Touching a mapping past the end of the file is SIGBUS , and a smaller file ( an older message layout , or a
    //writer that has not stretched it yet ) used to be mapped at the full size and read right away
    struct stat fileState;
    if ( (fstat(nbc->fd,&fileState)!=0) || ((uintmax_t) fileState.st_size < (uintmax_t) sizeOfBridgeMsg) )
    {
        fprintf(stderr,"Bridge file %s is smaller than the %u byte message , not mapping it\n",fileDescriptor,sizeOfBridgeMsg);
        close(nbc->fd); nbc->fd=-1;
        return 0;
    }

    nbc->map = mmap(0, nbc->dataSize , PROT_READ, MAP_SHARED, nbc->fd, 0);
    if (nbc->map == MAP_FAILED)
    {
        close(nbc->fd);
        nbc->fd=-1; nbc->map=0;
        perror("Error mmapping the file");
        return 0;
    }



   void * tmpBuf= malloc(sizeOfBridgeMsg);

   if (tmpBuf)
   {
      readBridge(nbc,tmpBuf,sizeOfBridgeMsg);
      free(tmpBuf);
   }

  // struct naoCommand incomingCommand={0};
  // readCmdBridge(nbc,&incomingCommand);
  // nbc->lastMsgTimestamp = incomingCommand.timestampInit;
  // printf("Initial random timestamp is %lu \n", nbc->lastMsgTimestamp);

 nbc->mode=1;
 fprintf(stderr,"initializeReadingBridge success \n");

 return 1;
}



int readBridge(struct bridgeContext * nbc,void * data , unsigned int dataSize )
{
  if (nbc==0)             { return 0; }
  if (data==0)            { return 0; }
  if (dataSize==0)        { return 0; }

  if (nbc->mode==0)
  {
    fprintf(stderr,"Cannot read from bridge , not properly initialized ..\n");
    return 0;
  }
  if (dataSize>nbc->dataSize) { fprintf(stderr,"Cannot read %u bytes from a %zu byte bridge\n",dataSize,nbc->dataSize); return 0; } //Was unchecked

  if (nbc->map!=0)
  {
    memcpy(data,nbc->map,dataSize);
    return 1;
  }

 return 0;
}






int closeReadingBridge(struct bridgeContext * nbc)
{
   return closeBridge(nbc);
}


