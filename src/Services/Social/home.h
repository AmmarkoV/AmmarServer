#ifndef HOME_H_INCLUDED
#define HOME_H_INCLUDED

#include "session.h"

void * home_callback(struct AmmServer_DynamicRequest  * rqst);
void * profile_callback(struct AmmServer_DynamicRequest  * rqst);

void * post_callback(struct AmmServer_DynamicRequest  * rqst);
void * comment_callback(struct AmmServer_DynamicRequest  * rqst);
void * like_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // HOME_H_INCLUDED
