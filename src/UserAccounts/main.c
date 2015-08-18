#include "userAccounts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UserAccountDatabase * initializeUserAccountDatabase(char * filename)
{
  return 0;
};

int closeUserAccountDatabase(struct UserAccountDatabase *  uadb)
{
  return 0;
};


int userTokenIsOk(
                  struct UserAccountDatabase *  uadb,
                  struct UserAccountAuthenticationToken * outputToken,
                  UserAccount_UserID userID
                  )
{
 return 0;
}


int userIsAuthentic(struct UserAccountDatabase *  uadb,
                    struct UserAccountAuthenticationToken * outputToken,
                    char * username,
                    char * password,
                    UserAccount_PasswordEncoding encoding,
                    char * ip,
                    char * fingerprint
                    )
{


 return 0;
}
