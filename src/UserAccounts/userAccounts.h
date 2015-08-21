#ifndef USERACCOUNTS_H_INCLUDED
#define USERACCOUNTS_H_INCLUDED

enum UserAccountPasswordEncodingEnum
{
  ENCODING_PLAINTEXT=0,
  ENCODING_SHA1,
  ENCODING_AVAILIABLE_TYPES
};

typedef unsigned int UserAccount_PasswordEncoding;
typedef unsigned int UserAccount_UserID;

struct UserAccountAuthenticationToken
{
  unsigned int dummy;
};


struct UserAccountDatabase
{
  unsigned int dummy;
  struct UserAccountAuthenticationToken lastAuthenticationToken;
};


struct UserAccountDatabase * uadb_initializeUserAccountDatabase(char * filename);

int uadb_closeUserAccountDatabase(struct UserAccountDatabase **  uadb);


int uadb_authenticateUser(
                           struct UserAccountDatabase *  uadb,
                           struct UserAccountAuthenticationToken * outputToken,
                           UserAccount_UserID userID
                         );

int uadb_loginUser(struct UserAccountDatabase *  uadb,
                    struct UserAccountAuthenticationToken * outputToken,
                    char * username,
                    char * password,
                    UserAccount_PasswordEncoding encoding,
                    char * ip,
                    char * browserFingerprint
                    );

#endif // USERACCOUNTS_H_INCLUDED
