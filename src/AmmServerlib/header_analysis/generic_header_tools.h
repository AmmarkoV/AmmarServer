/** @file generic_header_tools.h
* @brief Tools to process POST requests
* @author Ammar Qammaz (AmmarkoV)
* @bug POST header analysis is not fully implemented yet
*/

#ifndef GENERICHEADERTOOLSS_H_INCLUDED
#define GENERICHEADERTOOLSS_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C" {
#endif



/**
* @brief Ask if a header is a HEAD request, detected by the first four consecutive bytes being H E A D
* @ingroup HTTPanalysis
* @param Pointer to incoming request (streaming) string
* @param Length of incoming string
* @retval 1=HEAD,0=Not HEAD request*/
int HTTPHeaderIsHEAD(char * request , unsigned int requestLength);

/**
* @brief Ask if a header is a POST request, detected by the first four consecutive bytes being P O S T
* @ingroup HTTPanalysis
* @param Pointer to incoming request (streaming) string
* @param Length of incoming string
* @retval 1=POST,0=Not POST request*/
int HTTPHeaderIsPOST(char * request , unsigned int requestLength);

/**
* @brief Ask if a header is a GET request, detected by the first three consecutive bytes being G E T
* @ingroup HTTPanalysis
* @param Pointer to incoming request (streaming) string
* @param Length of incoming string
* @retval 1=GET,0=Not GET request*/
int HTTPHeaderIsGET(char * request , unsigned int requestLength);

int recalculateHeaderFieldsBasedOnANewBaseAddress(struct HTTPTransaction * transaction);
int growHeader(struct HTTPTransaction * transaction);


int keepAnalyzingHTTPHeader(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction);

/**
* @brief Ask if a header is complete inside an incoming request , detected by four consecutive bytes CR LF CR LF that mark the end of a header
* @ingroup HTTPanalysis
* @param Pointer to incoming request (streaming) string
* @param Length of incoming string
* @retval 1=Complete,0=Incomplete*/
int HTTPHeaderScanForEnding(char * request,unsigned int request_length);

int HTTPHeaderIsComplete(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction);

#ifdef __cplusplus
}
#endif

#endif // POSTHEADERS_H_INCLUDED
