#ifndef ULTRALITENAOMMAPBRIDGE_H_INCLUDED
#define ULTRALITENAOMMAPBRIDGE_H_INCLUDED


#include <stdlib.h>


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */

struct bridgeContext
{
  int mode; // 0 = not working - 1 = read , 2 = write
  int fd;
  void * map;
  size_t dataSize;
  unsigned long lastMsgTimestamp;
};

struct bridgePayloadHeader
{
   unsigned long timestampInit;
};



int initializeWritingBridge(struct bridgeContext * nbc ,const char * fileDescriptor,unsigned int sizeOfBridgeMsg);
int clearWritingBridge(struct bridgeContext * nbc);
int writeBridge(struct bridgeContext * nbc , void * data , unsigned int dataSize);
int closeWritingBridge(struct bridgeContext * nbc);



int initializeReadingBridge(struct bridgeContext * nbc ,const char * fileDescriptor,unsigned int sizeOfBridgeMsg);
int readBridge(struct bridgeContext * nbc,void * data , unsigned int dataSize );

int newBridgeMessageAvailiable(struct bridgeContext * nbc);
int closeReadingBridge(struct bridgeContext * nbc);
#endif // ULTRALITENAOMMAPBRIDGE_H_INCLUDED
