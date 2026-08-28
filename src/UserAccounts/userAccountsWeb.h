#ifndef USERACCOUNTSWEB_H_INCLUDED
#define USERACCOUNTSWEB_H_INCLUDED

#include "userAccounts.h"
#include "../AmmServerlib/AmmServerlib.h"

//Resolves the "s" session param ( tried as GET then POST , matching the sessionID-in-URL idiom every
//AmmarServer Service that uses UserAccounts follows , since AmmServerlib has no way to set a cookie )
//into a username. Returns 1 on success , 0 if there is no session param or it doesn't resolve to a user.
int uadbWeb_getAuthenticatedUser(struct UserAccountDatabase * uadb , struct AmmServer_DynamicRequest * rqst , char * outUsername , unsigned int outUsernameSize);

//3-31 characters , letters/digits/_/- only. Usernames end up echoed back into HTML pages and into plain-text
//polling endpoints meant to be inserted client-side with .textContent rather than innerHTML , across every
//Service that uses this library , so restricting the character set once here is simpler and safer than
//auditing every future display site for escaping.
int uadbWeb_isValidUsername(const char * username);

int uadbWeb_userAccountExists(struct UserAccountDatabase * uadb , const char * username);

#endif // USERACCOUNTSWEB_H_INCLUDED
