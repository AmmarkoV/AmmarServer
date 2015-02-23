#ifndef ASTRING_H_INCLUDED
#define ASTRING_H_INCLUDED




int astringReplaceVarInMemoryFile(char * page,unsigned int pageLength,const char * var,const char * value);
int astringReplaceAllInstancesOfVarInMemoryFile(char * page,unsigned int instances,unsigned int pageLength,const char * var,const char * value);

char * astringReadFileToMemory(const char * filename,unsigned int *length );

int astringWriteFileFromMemory(const char * filename,char * memory , unsigned int memoryLength);

int astringCopyOverlappingDataContent(unsigned char * buffer , unsigned int totalSize  , unsigned char * from , unsigned char * to , unsigned int blockSize);

int astringInjectDataToBuffer(unsigned char * entryPoint , unsigned char * data , unsigned char * buffer,  unsigned int currentBufferLength , unsigned int totalBufferLength);


#endif // ASTRING_H_INCLUDED
