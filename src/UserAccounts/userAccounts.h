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
  char * username;
  char * password;
  char * sessionID;
  UserAccount_UserID uid;
};

struct RegisteredUser
{
  char username[32];
  char password[32];
  char sessionID[32];
};


struct UserAccountDatabase
{
  char filename[512];
  struct UserAccountAuthenticationToken lastAuthenticationToken;

  unsigned int userListSize;
  unsigned int userListMaxSize;
  struct RegisteredUser * userList;
};


struct UserAccountDatabase * uadb_initializeUserAccountDatabase(const char * filename);

int uadb_closeUserAccountDatabase(struct UserAccountDatabase **  uadb);



int uadb_authenticateUser(
                           struct UserAccountDatabase *  uadb,
                           const char * username ,
                           const char * password ,
                           struct UserAccountAuthenticationToken * outputToken
                         );

int uadb_getUserTokenFromUserID(
                                 struct UserAccountDatabase *  uadb,
                                 struct UserAccountAuthenticationToken * outputToken ,
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
