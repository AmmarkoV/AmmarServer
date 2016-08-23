/** @file sendHTTPHeader.h
* @brief Small code segments that transmit HTTP responses
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef SENDHTTPHEADER_H_INCLUDED
#define SENDHTTPHEADER_H_INCLUDED

#include "../AmmServerlib.h"

/**
* @brief Send an Error Code header
* @ingroup network
* @param An AmmarServer instance
* @param Socket to send to
* @param ErrorCode to be transmitted to client
* @param Verified Filename of file to transmit ( appended with error code )
* @param Filename to directory when error template files are stored
* @bug   This call seems to fail ?
* @retval 1=Success,0=Failure*/
unsigned long SendErrorCodeHeader(struct AmmServer_Instance * instance,int clientsock,unsigned int error_code,const char * verified_filename,const char * templates_root);

/**
* @brief Send a Success header , meaning that what was asked for will follow
* @ingroup network
* @param An AmmarServer instance
* @param Socket to send to
* @param Success code ( typically 200 ok )
* @param Verified Filename of file to transmit
* @retval 1=Success,0=Failure*/
unsigned long SendSuccessCodeHeader(struct AmmServer_Instance * instance,int clientsock,int success_code,const char * verified_filename);

/**
* @brief Send a 304 Not Modified response
* @ingroup network
* @param An AmmarServer instance
* @param Socket to send to
* @retval 1=Success,0=Failure*/
unsigned long SendNotModifiedHeader(struct AmmServer_Instance * instance,int clientsock);

/**
* @brief Send a 401 Not Authorized response
* @ingroup network
* @param An AmmarServer instance
* @param Socket to send to
* @param String with message to be sent
* @param Verified Filename of file asked to be transmitted
* @retval 1=Success,0=Failure*/
unsigned long SendAuthorizationHeader(struct AmmServer_Instance * instance,int clientsock,char * message,const char * verified_filename);

#endif // SENDHTTPHEADER_H_INCLUDED
