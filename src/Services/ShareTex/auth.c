#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "auth.h"
#include "../../UserAccounts/userAccountsWeb.h"

int initializeLoginSystem()
{
  char dbPath[MAX_FILE_PATH]={0};
  snprintf(dbPath,sizeof(dbPath),"%sdb/users.db",dataRoot);

  uadb = uadb_initializeUserAccountDatabase(dbPath);
  return (uadb!=0);
}

int stopLoginSystem()
{
  uadb_closeUserAccountDatabase(&uadb);
  return 1;
}


int userAccountExists(const char * username)
{
  return uadbWeb_userAccountExists(uadb,username);
}


int getAuthenticatedUser(struct AmmServer_DynamicRequest * rqst , char * outUsername , unsigned int outUsernameSize)
{
  //Cookie-based session ( AMMSESSID ) instead of the old ?s=<sessionID> URL param : the session ID no longer
  //travels in URLs/links/redirects where it leaks through browser history , the Referer header and access logs.
  return AmmServer_CurrentUsername(rqst,outUsername,outUsernameSize);
}


void * signup_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[32]={0};
  char password[32]={0};
  _POSTcpy(rqst,"username",username,sizeof(username));
  _POSTcpy(rqst,"password",password,sizeof(password));

  if ( ! uadbWeb_isValidUsername(username) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Username must be 3-31 characters , letters/digits/_/- only. <a href=\"index.html\">Back</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  if ( strlen(password)==0 )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Password is required. <a href=\"index.html\">Back</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  if ( userAccountExists(username) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>That username is already taken. <a href=\"index.html\">Back</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  if ( ! uadb_addUser(uadb,username,password,"0.0.0.0","no fingerprint") )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Could not create account , please try again. <a href=\"index.html\">Back</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  if ( AmmServer_Login(rqst,uadb,username,password) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<html><head><meta http-equiv=\"refresh\" content=\"0; url=dashboard.html\"></head>"
             "<body>Account created , redirecting..</body></html>");
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Account created , please <a href=\"index.html\">log in</a></body></html>");
  }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * login_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[32]={0};
  char password[32]={0};
  _POSTcpy(rqst,"username",username,sizeof(username));
  _POSTcpy(rqst,"password",password,sizeof(password));

  if ( (strlen(username)>0) && (strlen(password)>0) &&
       AmmServer_Login(rqst,uadb,username,password) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<html><head><meta http-equiv=\"refresh\" content=\"0; url=dashboard.html\"></head>"
             "<body>Logging in , redirecting..</body></html>");
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Wrong username/password combination. <a href=\"index.html\">Back</a></body></html>");
  }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
