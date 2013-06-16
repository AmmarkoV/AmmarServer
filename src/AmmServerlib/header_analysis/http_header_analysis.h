#ifndef HTTP_HEADER_ANALYSIS_H_INCLUDED
#define HTTP_HEADER_ANALYSIS_H_INCLUDED

#include "../AmmServerlib.h"

char * ReceiveHTTPHeader(struct AmmServer_Instance * instance,int clientSock , unsigned long * headerLength);
int AppendPOSTRequestToHTTPHeader(struct HTTPTransaction * transaction);

int FreeHTTPHeader(struct HTTPHeader * output);
int HTTPHeaderComplete(char * request,unsigned int request_length);
int HTTPHeaderIsPOST(char * request , unsigned int requestLength);

int AnalyzeHTTPHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction);
#endif // HTTP_HEADER_ANALYSIS_H_INCLUDED
