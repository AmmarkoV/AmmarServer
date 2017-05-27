#include "userAccounts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UserAccountDatabase * uadb_initializeUserAccountDatabase(const char * filename)
{
 FILE * pFile;
 pFile = fopen (filename,"rb");
 if (pFile!=0)
    {
     struct UserAccountDatabase * uadb = (struct UserAccountDatabase *) malloc(sizeof(struct UserAccountDatabase));
     if (uadb!=0)
     {
       snprintf(uadb->filename,512,"%s",filename);
       fscanf(pFile,"%d\n",&uadb->userListSize);
       uadb->userListMaxSize=uadb->userListSize+100;
       uadb->userList = (struct RegisteredUser *) malloc(sizeof(struct RegisteredUser) * uadb->userListMaxSize);

       unsigned int i=0;
       for (i=0; i<uadb->userListSize; i++)
       {
        fprintf(stderr,"User %u : ",i);
        fscanf(pFile,"%s\n",uadb->userList[i].username);
        fscanf(pFile,"%s\n",uadb->userList[i].password);
        fscanf(pFile,"%s\n",uadb->userList[i].sessionID);
        fprintf(stderr," %s/******  - session=%s\n",uadb->userList[i].username,uadb->userList[i].sessionID);
       }
     }
     fclose (pFile);
     return uadb;
    }
  return 0;
};


int uadb_saveUserAccountDatabase(struct UserAccountDatabase * uadb)
{
 FILE * pFile;
 pFile = fopen (uadb->filename,"wb");
 if (pFile!=0)
    {
     if (uadb!=0)
     {
       fprintf(pFile,"%d\n",uadb->userListSize);

       unsigned int i=0;
       for (i=0; i<uadb->userListSize; i++)
       {
        fprintf(pFile,"%s\n",uadb->userList[i].username);
        fprintf(pFile,"%s\n",uadb->userList[i].password);
        fprintf(pFile,"%s\n",uadb->userList[i].sessionID);
       }
     }
     fclose (pFile);
     return 1;
    }
  return 0;
}

int uadb_closeUserAccountDatabase(struct UserAccountDatabase **  uadb)
{
  uadb_saveUserAccountDatabase(*uadb);

  //TODO:

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
