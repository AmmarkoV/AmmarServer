#include "userAccounts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UserAccountDatabase * uadb_initializeUserAccountDatabase(const char * filename)
{
  return 0;
};


int uadb_saveUserAccountDatabase(struct UserAccountDatabase * uadb)
{
  return 0;
}

int uadb_closeUserAccountDatabase(struct UserAccountDatabase **  uadb)
{
  return 0;
};


int uadb_authenticateUser(
                           struct UserAccountDatabase *  uadb,
                           struct UserAccountAuthenticationToken * outputToken,
                           UserAccount_UserID userID
                         )
{
 return 0;
}


int uadb_loginUser(
                   struct UserAccountDatabase *  uadb,
                   struct UserAccountAuthenticationToken * outputToken,
                   const char * username,
                   const char * password,
                   UserAccount_PasswordEncoding encoding,
                   const char * ip,
                   const char * browserFingerprint
                   )
{


 return 0;
}
