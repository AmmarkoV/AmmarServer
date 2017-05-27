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
  char sessionID[64];
};


struct UserAccountDatabase
{
  unsigned int dummy;
  struct UserAccountAuthenticationToken lastAuthenticationToken;

  char filename[512];
};


struct UserAccountDatabase * uadb_initializeUserAccountDatabase(const char * filename);

int uadb_closeUserAccountDatabase(struct UserAccountDatabase **  uadb);


int uadb_authenticateUser(
                           struct UserAccountDatabase *  uadb,
                           struct UserAccountAuthenticationToken * outputToken,
                           UserAccount_UserID userID
                         );

int uadb_getUserIDForSessionID(
                               struct UserAccountDatabase *  uadb,
                               const char * sessionID,
                               UserAccount_UserID *userID
                               );

int uadb_loginUser(
                   struct UserAccountDatabase *  uadb,
                   struct UserAccountAuthenticationToken * outputToken,
                   const char * username,
                   const char * password,
                   UserAccount_PasswordEncoding encoding,
                   const char * ip,
                   const char * browserFingerprint
                   );

int uadb_addUser(
                   struct UserAccountDatabase *  uadb,
                   const char * username,
                   const char * password,
                   const char * ip,
                   const char * browserFingerprint
                 );


#endif // USERACCOUNTS_H_INCLUDED
