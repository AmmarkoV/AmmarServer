#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../UserAccounts/userAccountsWeb.h"

//Matches struct RegisteredUser::username ( userAccounts.h ) , which is what everything here ends up holding..
#define MAX_USERNAME 32
//AmmServer_HTMLEscape() can grow a string by at most 6x ( &quot; ) , so escaped copies get sized off this..
#define MAX_ESCAPED_USERNAME (MAX_USERNAME*6)
//Sized to match the token AmmServer_GenerateCSRFToken() hands back ( see main.c )
#define MAX_CSRF_TOKEN 64

extern struct UserAccountDatabase * uadb;

int initializeLoginSystem();
int stopLoginSystem();

/*The page every resource that needs a logged in visitor serves when there isn't one..*/
void socialServeLoginRequired(struct AmmServer_DynamicRequest * rqst);

/*Checks the "csrf" field a state changing request posted back against the token this session was issued.
  Every mutation ( post / comment / like / speak / logout ) goes through this..*/
int socialPostedCSRFIsValid(struct AmmServer_DynamicRequest * rqst);

/*Appends what fits of `chunk` to `buffer` and silently stops once it doesn't , so a full buffer truncates a
  listing instead of overflowing it. `position` carries the write offset between calls..*/
void socialAppendChunk(char * buffer,unsigned int bufferSize,unsigned int * position,const char * chunk);

/*Serves a redirect back to the page the visitor was on. `backUser` is the profile they came from , or 0 /
  empty for the wall - it is checked against the username rules rather than used as a URL , so it can never
  become an open redirect..*/
void socialRedirectBack(struct AmmServer_DynamicRequest * rqst,const char * backUser);

#endif // SESSION_H_INCLUDED
