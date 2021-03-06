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
        fprintf(stderr," %s - session=%s\n",uadb->userList[i].username,uadb->userList[i].sessionID);
       }
     }
     fclose (pFile);

     fprintf(stderr,"Sucessfully initialized from user list %s with %u users\n",filename,uadb->userListSize);

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
                           const char * username ,
                           const char * password ,
                           struct UserAccountAuthenticationToken * outputToken
                         )
{
 if (uadb==0)
 {
   fprintf(stderr,"Cannot authenticate user without an initialized database\n");
   return 0;
 }
 if (outputToken==0)
 {
   fprintf(stderr,"Cannot return values without a UserAccountAuthenticationToken\n");
   return 0;
 }
 unsigned int i=0;

 for (i=0; i<uadb->userListSize; i++)
 {
   if (strcmp(uadb->userList[i].username,username)==0)
   {
    if (strcmp(uadb->userList[i].password,password)==0)
    {

       fprintf(stderr,"Found Account %s\n",outputToken->username);
       outputToken->username =uadb->userList[i].username;
       outputToken->password =uadb->userList[i].password;
       outputToken->sessionID=uadb->userList[i].sessionID;
       outputToken->uid      =i;
       return 1;
    }
   }
 }

 return 0;
}


int uadb_getUserTokenFromUserID(
                                 struct UserAccountDatabase *  uadb,
                                 struct UserAccountAuthenticationToken * outputToken ,
                                 UserAccount_UserID userID
                              )
{
 if (uadb==0)        {  fprintf(stderr,"Cannot authenticate user without an initialized database\n");      return 0; }
 if (outputToken==0) {  fprintf(stderr,"Cannot return values without a UserAccountAuthenticationToken\n"); return 0; }
  outputToken->username =uadb->userList[userID].username;
  outputToken->password =uadb->userList[userID].password;
  outputToken->sessionID=uadb->userList[userID].sessionID;
  outputToken->uid      =userID;
 return 1;
}

int uadb_getUserIDFromSessionID(
                                    struct UserAccountDatabase *  uadb,
                                    const char * sessionID,
                                    UserAccount_UserID *userID
                               )
{
 if (uadb==0) { return 0; }
 unsigned int i=0;

 for (i=0; i<uadb->userListSize; i++)
 {
   if (strcmp(uadb->userList[i].sessionID,sessionID)==0)
   {
       *userID=i;
       return 1;
   }
 }
 return 0; //notFound
}

int uadb_getUserTokenFromSessionID(
                                   struct UserAccountDatabase *  uadb,
                                   const char * sessionID,
                                   struct UserAccountAuthenticationToken * outputToken
                                 )
{
 if (uadb==0) { return 0; }
 UserAccount_UserID userID=0;

 if (
     uadb_getUserIDFromSessionID(
                                 uadb ,
                                 sessionID,
                                 &userID
                                )
     )
     {
       if (
           uadb_getUserTokenFromUserID(
                                       uadb,
                                       outputToken ,
                                       userID
                                      )
           )
           {
             return 1;
           }
     }
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
 //Check UserAccount_PasswordEncoding here

 if (
 uadb_authenticateUser(
                       uadb,
                       username ,
                       password ,
                       outputToken
                      )
     )
     {
       return 1;
     }
  //DO LOG HERE..!
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
