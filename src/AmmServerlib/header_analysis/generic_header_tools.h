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


int recalculateHeaderFieldsBasedOnANewBaseAddress(struct HTTPTransaction * transaction);
char * growHeader(unsigned int * MaxIncomingRequestLength , char * incomingRequest);



#ifdef __cplusplus
}
#endif

#endif // POSTHEADERS_H_INCLUDED
