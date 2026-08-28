#ifndef AUTH_H_INCLUDED
#define AUTH_H_INCLUDED

#include "state.h"

int initializeLoginSystem();
int stopLoginSystem();

int userAccountExists(const char * username);

//Resolves the "s" session param ( GET or POST , tries both ) into a username. Returns 1/0.
int getAuthenticatedUser(struct AmmServer_DynamicRequest * rqst , char * outUsername , unsigned int outUsernameSize);

void * signup_callback(struct AmmServer_DynamicRequest * rqst);
void * login_callback(struct AmmServer_DynamicRequest * rqst);

#endif // AUTH_H_INCLUDED
