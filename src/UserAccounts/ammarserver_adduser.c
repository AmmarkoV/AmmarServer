/*
ammarserver_adduser , adds a new account to an AmmarServer user database
Usage : ammarserver_adduser USERNAME [path/to/users.db]
The database defaults to the shared <repo>/db/users.db ( found relative to this executable ) and is created if it does not exist yet ,
the password is read from stdin ( never from argv , where ps and shell history would see it )
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <unistd.h>
#include <termios.h>

#include "userAccounts.h"

#define MIN_PASSWORD_LENGTH 8

static void readPassword(const char * prompt,char * password,unsigned int passwordSize)
{
  struct termios original,silent;
  int isTerminal = isatty(0) && (tcgetattr(0,&original)==0);
  if (isTerminal)
  {
    fprintf(stderr,"%s",prompt);
    silent=original;
    silent.c_lflag &= ~ECHO;
    tcsetattr(0,TCSANOW,&silent);
  }

  password[0]=0;
  if (fgets(password,passwordSize,stdin)==0) { password[0]=0; }
  password[strcspn(password,"\r\n")]=0;

  if (isTerminal) { tcsetattr(0,TCSANOW,&original); fprintf(stderr,"\n"); }
}

//<directory of this executable>/../../db/users.db , i.e. <repo>/db/users.db
static int defaultDatabasePath(char * path,unsigned int pathSize)
{
  char exePath[1024]={0};
  ssize_t length = readlink("/proc/self/exe",exePath,sizeof(exePath)-1);
  if (length<=0) { return 0; }
  exePath[length]=0;
  char * lastSlash = strrchr(exePath,'/');
  if (lastSlash==0) { return 0; }
  *(lastSlash+1)=0;
  return (snprintf(path,pathSize,"%s../../db/users.db",exePath)<(int) pathSize);
}

//Same rule as uadbWeb_isValidUsername() ( userAccountsWeb.c ) , copied because that file pulls in AmmServerlib
//and this tool only links the plain UserAccounts library. The database is whitespace separated , so this also keeps it parseable.
static int isValidUsername(const char * username)
{
  unsigned int len = strlen(username);
  if ( (len<3) || (len>31) ) { return 0; }

  unsigned int i=0;
  for (i=0; i<len; i++)
  {
    if ( ! ( isalnum((unsigned char)username[i]) || (username[i]=='_') || (username[i]=='-') ) ) { return 0; }
  }
  return 1;
}

int main(int argc,char ** argv)
{
  if ( (argc<2) || (argc>3) )
  {
    fprintf(stderr,"Usage : %s USERNAME [path/to/users.db]\n",argv[0]);
    fprintf(stderr,"Adds a new user , the password is read from stdin\n");
    return 1;
  }

  const char * username = argv[1];
  if (!isValidUsername(username)) { fprintf(stderr,"Usernames are 3-31 characters , letters/digits/_/- only\n"); return 1; }

  char database[1024];
  if (argc==3) { snprintf(database,sizeof(database),"%s",argv[2]); } else
  if (!defaultDatabasePath(database,sizeof(database))) { fprintf(stderr,"Could not find the user database , give its path\n"); return 1; }

  //uadb_addUser does not report a failed save , so make sure the file ( or the directory it will be created in ) is writable first
  int creating = (access(database,F_OK)!=0);
  if (creating)
  {
    char directory[1024];
    snprintf(directory,sizeof(directory),"%s",database);
    if (access(dirname(directory),W_OK|X_OK)!=0) { fprintf(stderr,"Cannot create the user database %s\n",database); return 1; }
  } else
  if (access(database,R_OK|W_OK)!=0) { fprintf(stderr,"Cannot read/write the user database %s\n",database); return 1; }

  struct UserAccountDatabase * uadb = uadb_initializeUserAccountDatabase(database);
  if (uadb==0) { fprintf(stderr,"Could not open the user database %s\n",database); return 1; }

  int result=1;
  unsigned int i=0, exists=0;
  for (i=0; i<uadb->userListSize; i++) { if (strcmp(uadb->userList[i].username,username)==0) { exists=1; break; } }

  if (exists) { fprintf(stderr,"User %s already exists in %s\n",username,database); } else
  {
    char password[128],again[128];
    readPassword("Password: ",password,sizeof(password));
    if (isatty(0)) { readPassword("Repeat password: ",again,sizeof(again)); }
              else { snprintf(again,sizeof(again),"%s",password); }

    if (strlen(password)<MIN_PASSWORD_LENGTH) { fprintf(stderr,"Passwords need at least %u characters\n",MIN_PASSWORD_LENGTH); } else
    if (strcmp(password,again)!=0)            { fprintf(stderr,"Passwords do not match\n"); } else
    if (!uadb_addUser(uadb,username,password,"0.0.0.0","cli")) { fprintf(stderr,"Could not add user\n"); } else
    {
      fprintf(stderr,"User %s added to %s%s\n",username,database,(creating) ? " ( new database )" : "");
      fprintf(stderr,"Running services only load this database when they start , restart them ( e.g. Ammarflix ) to see the new user.\n");
      fprintf(stderr,"Social and ShareTex save the database when they stop , if one was running during this change it will drop the new user on exit.\n");
      result=0;
    }
    memset(password,0,sizeof(password));
    memset(again,0,sizeof(again));
  }

  //Not uadb_closeUserAccountDatabase , that would save the file a second time ( and create it even when nothing was added )
  free(uadb->userList);
  free(uadb);
  return result;
}
