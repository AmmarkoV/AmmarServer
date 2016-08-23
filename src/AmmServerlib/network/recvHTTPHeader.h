/** @file recvHTTPHeader.h
* @brief Small code segments that receive HTTP responses
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef RECEIVEHTTPHEADER_H_INCLUDED
#define RECEIVEHTTPHEADER_H_INCLUDED


#include "../AmmServerlib.h"


/**
* @brief Receive and Parse Incoming HTTP Request
* @ingroup network
* @param An AmmarServer Instance
* @param HTTPTransaction this send file is part of
* @retval 1=Success,0=Failure*/
int receiveAndParseIncomingHTTPRequest(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction);

#endif // RECEIVEHTTPHEADER_H_INCLUDED
