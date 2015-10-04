/** @file recvHTTPHeader.h
* @brief Small code segments that receive HTTP responses
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef RECEIVEHTTPHEADER_H_INCLUDED
#define RECEIVEHTTPHEADER_H_INCLUDED


#include "../AmmServerlib.h"

receiveAndParseIncomingHTTPRequest(struct AmmServer_Instance * instance,struct HTTPTransaction * transaction);

#endif // RECEIVEHTTPHEADER_H_INCLUDED
