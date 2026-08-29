#ifndef USERACCOUNTSWEB_H_INCLUDED
#define USERACCOUNTSWEB_H_INCLUDED

#include "userAccounts.h"
#include "../AmmServerlib/AmmServerlib.h"

//Resolves the "s" session param ( tried as GET then POST , matching the sessionID-in-URL idiom the existing
//Services that use UserAccounts ( Social, ShareTex ) were built around, back when AmmServerlib had no way to
//set a cookie ). Returns 1 on success , 0 if there is no session param or it doesn't resolve to a user.
//New code should prefer AmmServer_Login()/AmmServer_CurrentUsername() below instead - real cookie-based
//sessions, not a URL param carrying the account's permanent secret token forever ( see knowledge/issues.md ).
int uadbWeb_getAuthenticatedUser(struct UserAccountDatabase * uadb , struct AmmServer_DynamicRequest * rqst , char * outUsername , unsigned int outUsernameSize);

//3-31 characters , letters/digits/_/- only. Usernames end up echoed back into HTML pages and into plain-text
//polling endpoints meant to be inserted client-side with .textContent rather than innerHTML , across every
//Service that uses this library , so restricting the character set once here is simpler and safer than
//auditing every future display site for escaping.
int uadbWeb_isValidUsername(const char * username);

int uadbWeb_userAccountExists(struct UserAccountDatabase * uadb , const char * username);

//---------------------------------------------------------------------------------------------------------------
// AmmServerlib-session-backed login - the modern, cookie-based alternative to uadbWeb_getAuthenticatedUser()'s
// URL-param idiom above. Requires the calling resource to be registered with useSessionLifecycle=1 ( see
// AmmServer_RH_Context / AmmServer_AddResourceHandler ) - that's what makes rqst->sessionToken meaningful.
//---------------------------------------------------------------------------------------------------------------

//Verifies username+password against uadb, and on success ties the current session to that account : rotates to
//a brand new session token ( session-fixation defense - a pre-login anonymous session ID must never become the
//authenticated one ), issues the new session cookie, and stores "username"/"uid" into the session's own data.
//@retval 1=Logged in , 0=Wrong credentials / bad arguments
int AmmServer_Login(struct AmmServer_DynamicRequest * rqst,struct UserAccountDatabase * uadb,const char * username,const char * password);

//Destroys the current session server-side and clears its cookie client-side ( Max-Age in the past ).
//@retval 1=Success , 0=No session to log out of / bad arguments
int AmmServer_Logout(struct AmmServer_DynamicRequest * rqst);

//The one call a resource handler should use to find out "is this request logged in, and as whom" - reads the
//"username" session key AmmServer_Login() sets.
//@retval 1=Logged in ( outUsername holds the username ) , 0=Not logged in
int AmmServer_CurrentUsername(struct AmmServer_DynamicRequest * rqst,char * outUsername,unsigned int outUsernameSize);

#endif // USERACCOUNTSWEB_H_INCLUDED
