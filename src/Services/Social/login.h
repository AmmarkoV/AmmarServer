#ifndef LOGIN_H_INCLUDED
#define LOGIN_H_INCLUDED


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../UserAccounts/userAccounts.h"

extern struct UserAccountDatabase * uadb;

int initializeLoginSystem();
int stopLoginSystem();

void  * login_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // LOGIN_H_INCLUDED
