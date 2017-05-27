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



int uadb_getUserIDForSessionID(
                               struct UserAccountDatabase *  uadb,
                               const char * sessionID,
                               UserAccount_UserID *userID
                               )
{
 return 0; //notFound
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


int uadb_getBackRandomFileDigitsInplace(char * str , unsigned int numberOfDigits)
{
 unsigned int i=0,range=0;
 for (i=0; i<numberOfDigits; i++)
 {
   range='z'-'a';
   str[i]='a'+rand()%range;
 }
str[numberOfDigits]=0;
return 1;
}

char * uadb_getBackRandomFileDigits(unsigned int numberOfDigits)
{
 char * response= (char *) malloc(sizeof(char)* (numberOfDigits+1));
 uadb_getBackRandomFileDigitsInplace(response,numberOfDigits);
 return response;
}




int uadb_addUser(
                   struct UserAccountDatabase *  uadb,
                   const char * username,
                   const char * password,
                   const char * ip,
                   const char * browserFingerprint
                 )
{
 FILE *fp=fopen(uadb->filename,"a");
 if (fp!=0)
 {
   fprintf(fp,"%s\n",username);
   fprintf(fp,"%s\n",password);


   struct UserAccountAuthenticationToken token={0};
   uadb_getBackRandomFileDigitsInplace(token.sessionID,32);
   fprintf(fp,"%s\n",token.sessionID);
   fclose(fp);
 }

 return 0;
}
