/*
 *  @file mmapBridge.h
 *  @brief A tool that generates code that can span messages across computers
 *  @author Ammar Qammaz (AmmarkoV)
 */

/*
 * This file was automatically generated @ 05-12-2018 18:43:28 using AmmMessages v1.00004
 * https://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmMessages
 * Please note that changes you make here may be automatically overwritten
 * if the AmmMessages generator runs again..!
 */


#ifndef MMAPBRIDGE_H_INCLUDED
#define MMAPBRIDGE_H_INCLUDED


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


#ifdef __cplusplus
extern "C"
{
#endif

struct bridgeContext
{
  int mode; // 0 = not working - 1 = read , 2 = write
  int fd;
  void * map;
  size_t dataSize;
  unsigned long lastMsgTimestamp;
  void * callbackOnNewData;
};

struct bridgePayloadHeader
{
   unsigned long timestampInit;
};

char * strstrDoubleNewline(char * request,unsigned int requestLength,unsigned int * endOfLine);

int initializeWritingBridge(struct bridgeContext * nbc ,const char * fileDescriptor,unsigned int sizeOfBridgeMsg);
int clearWritingBridge(struct bridgeContext * nbc);
int writeBridge(struct bridgeContext * nbc , void * data , unsigned int dataSize, unsigned int waitForAcknowledgmentXMilliseconds);
int closeWritingBridge(struct bridgeContext * nbc);



int initializeReadingBridge(struct bridgeContext * nbc ,const char * fileDescriptor,unsigned int sizeOfBridgeMsg);
int readBridge(struct bridgeContext * nbc,void * data , unsigned int dataSize );

int closeReadingBridge(struct bridgeContext * nbc);

#ifdef __cplusplus
}
#endif


#endif // ULTRALITENAOMMAPBRIDGE_H_INCLUDED
