#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

/**  @brief TM structures carry the year after 1900 (see http://www.cplusplus.com/reference/ctime/tm/ )  so  this is encoded here as a reminder
     @ingroup security */
#define EPOCH_YEAR_IN_TM_YEAR 1900

#include "../../AmmServerlib/AmmServerlib.h"

void generalFailureResponseToRequest(struct AmmServer_DynamicRequest  * rqst);
void generalSuccessResponseToRequest(struct AmmServer_DynamicRequest  * rqst);

int sendEmail(
               const char * receipient,
               const char * subject,
               const char * message
             );

#endif // DEVICE_H_INCLUDED
