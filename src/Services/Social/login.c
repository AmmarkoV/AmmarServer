#include "login.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*Both forms answer failures the same way - back to the gate with a reason..*/
static void serveGateError(struct AmmServer_DynamicRequest * rqst,const char * reason)
{
  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>The Social Gate</title>"
           "<link rel='stylesheet' type='text/css' href='social.css'></head>"
           "<body><div class=\"notice\">%s <a href=\"index.html\">Back</a></div></body></html>",reason);
  rqst->contentSize=strlen(rqst->content);
}


static void serveLoggedIn(struct AmmServer_DynamicRequest * rqst)
{
  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
           "<meta http-equiv=\"refresh\" content=\"0; url=home.html\"></head>"
           "<body>Logged in , redirecting..</body></html>");
  rqst->contentSize=strlen(rqst->content);
}


void * login_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  char password[128]={0};
  _POSTcpy(rqst,"name",username,sizeof(username));
  _POSTcpy(rqst,"password",password,sizeof(password));

  //AmmServer_Login() verifies against the shared account database , then rotates this request onto a brand new
  //session and sets its cookie - the session token never travels in a URL anymore..
  if ( AmmServer_Login(rqst,uadb,username,password) )
  {
    serveLoggedIn(rqst);
  } else
  {
    serveGateError(rqst,"Wrong username/password combination.");
  }

  return 0;
}


void * signup_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[MAX_USERNAME]={0};
  char password[128]={0};
  _POSTcpy(rqst,"name",username,sizeof(username));
  _POSTcpy(rqst,"password",password,sizeof(password));

  if ( ! uadbWeb_isValidUsername(username) )
    { serveGateError(rqst,"Username must be 3-31 characters , letters/digits/_/- only."); return 0; }

  if ( strlen(password)==0 )
    { serveGateError(rqst,"Password is required."); return 0; }

  if ( uadbWeb_userAccountExists(uadb,username) )
    { serveGateError(rqst,"That username is already taken."); return 0; }

  if ( ! uadb_addUser(uadb,username,password,"0.0.0.0","no fingerprint") )
    { serveGateError(rqst,"Could not create the account , please try again."); return 0; }

  if ( AmmServer_Login(rqst,uadb,username,password) )
  {
    serveLoggedIn(rqst);
  } else
  {
    serveGateError(rqst,"Account created , please log in.");
  }

  return 0;
}


void * logout_callback(struct AmmServer_DynamicRequest  * rqst)
{
  //Logging out is state changing , so it is a CSRF checked POST rather than a link anyone can make you follow..
  if ( socialPostedCSRFIsValid(rqst) ) { AmmServer_Logout(rqst); }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
           "<meta http-equiv=\"refresh\" content=\"0; url=index.html\"></head>"
           "<body>Logged out..</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
