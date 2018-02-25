/** @file http_header_analysis.h
* @brief Tools to process HTTP requests
* @author Ammar Qammaz (AmmarkoV)
* @bug HTTP header analysis can be improved ( code style etc ) although the recent use of stringscanners has greatly improved it and reduced lines of code
*/

#ifndef HTTP_HEADER_ANALYSIS_H_INCLUDED
#define HTTP_HEADER_ANALYSIS_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C" {
#endif


void printRecvError();


/**
* @brief Receive an HTTP Header from a socket and prepare it for further processing
* @ingroup HTTPanalysis
* @param An AmmarServer Instance
* @param Socket that we should recv to get the http header
* @param Output length of incoming header
* @retval Pointer to memory containing HTTPHeader,0=Failure
* @bug Reallocation code of ReceiveHTTPHeader when we jump from a regular GET memory block to a large POST memory block is shit and needs to be fixed
*/
char * ReceiveHTTPHeader(struct AmmServer_Instance * instance,int clientSock , unsigned long * headerLength);

int AnalyzeHTTPLineRequest(
                            struct AmmServer_Instance * instance,
                            struct HTTPHeader * output,
                            char * request,
                            unsigned int request_length,
                            unsigned int lines_gathered,
                            char * webserver_root
                          );

/**
* @brief Analyze HTTP header ( after it has been accumulated into memory )
* @ingroup HTTPanalysis
* @param An AmmarServer Instance
* @param HTTPTransaction we are talking about
* @retval 1=Success,0=Failure */
int AnalyzeHTTPHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction);





#ifdef __cplusplus
}
#endif

#endif // HTTP_HEADER_ANALYSIS_H_INCLUDED
