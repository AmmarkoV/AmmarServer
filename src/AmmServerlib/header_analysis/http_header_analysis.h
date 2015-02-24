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

/**
* @brief POST requests also have a payload appended that we consider part of the whole "header" so we need to keep on reading it..!
* @ingroup POSTanalysis
* @param HTTPTransaction
* @retval 1=Success,0=Failure */
int AppendPOSTRequestToHTTPHeader(struct HTTPTransaction * transaction);

/**
* @brief Deallocate memory occupied by an HTTP Header
* @ingroup HTTPanalysis
* @param HTTPHeader to be deallocated
* @retval 1=Success,0=Failure */
int FreeHTTPHeader(struct HTTPHeader * output);

/**
* @brief Ask if a header is complete inside an incoming request , detected by four consecutive bytes CR LF CR LF that mark the end of a header
* @ingroup HTTPanalysis
* @param Pointer to incoming request (streaming) string
* @param Length of incoming string
* @retval 1=Complete,0=Incomplete*/
int HTTPHeaderComplete(char * request,unsigned int request_length);

/**
* @brief Ask if a header is a POST request, detected by the first four consecutive bytes being P O S T
* @ingroup HTTPanalysis
* @param Pointer to incoming request (streaming) string
* @param Length of incoming string
* @retval 1=POST,0=Not POST request*/
int HTTPHeaderIsPOST(char * request , unsigned int requestLength);

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
