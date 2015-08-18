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




struct UserAccountDatabase * loadUserAccountDatabase(char * filename);

#endif // USERACCOUNTS_H_INCLUDED
