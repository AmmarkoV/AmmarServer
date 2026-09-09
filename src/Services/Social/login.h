#ifndef LOGIN_H_INCLUDED
#define LOGIN_H_INCLUDED

#include "session.h"

void * login_callback(struct AmmServer_DynamicRequest  * rqst);
void * signup_callback(struct AmmServer_DynamicRequest  * rqst);
void * logout_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // LOGIN_H_INCLUDED
