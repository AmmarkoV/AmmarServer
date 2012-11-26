#ifndef HTTP_HEADER_ANALYSIS_H_INCLUDED
#define HTTP_HEADER_ANALYSIS_H_INCLUDED

#include "AmmServerlib.h"

int FreeHTTPRequest(struct HTTPRequest * output);
int HTTPRequestComplete(char * request,unsigned int request_length);
int AnalyzeHTTPRequest(struct AmmServer_Instance * instance,struct HTTPRequest * output,char * request,unsigned int request_length, char * webserver_root);

#endif // HTTP_HEADER_ANALYSIS_H_INCLUDED
