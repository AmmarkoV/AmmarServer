#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "userAccountsWeb.h"
#include "../AmmServerlib/cache/session_list.h"

int uadbWeb_getAuthenticatedUser(struct UserAccountDatabase * uadb , struct AmmServer_DynamicRequest * rqst , char * outUsername , unsigned int outUsernameSize)
{
  char sessionID[64]={0};
  if ( (!_GETcpy(rqst,"s",sessionID,sizeof(sessionID))) && (!_POSTcpy(rqst,"s",sessionID,sizeof(sessionID))) )
  {
    return 0;
  }

  struct UserAccountAuthenticationToken token={0};
  if ( ! uadb_getUserTokenFromSessionID(uadb,sessionID,&token) ) { return 0; }
  if (token.username==0) { return 0; }

  snprintf(outUsername,outUsernameSize,"%s",token.username);
  return 1;
}


int uadbWeb_isValidUsername(const char * username)
{
  if (username==0) { return 0; }
  unsigned int len = strlen(username);
  if ( (len<3) || (len>31) ) { return 0; }

  unsigned int i=0;
  for (i=0; i<len; i++)
  {
    if ( ! ( isalnum((unsigned char)username[i]) || (username[i]=='_') || (username[i]=='-') ) ) { return 0; }
  }
  return 1;
}


int uadbWeb_userAccountExists(struct UserAccountDatabase * uadb , const char * username)
{
  if ( (uadb==0) || (username==0) ) { return 0; }
  unsigned int i=0;
  for (i=0; i<uadb->userListSize; i++)
  {
    if (strcmp(uadb->userList[i].username,username)==0) { return 1; }
  }
  return 0;
}


int AmmServer_Login(struct AmmServer_DynamicRequest * rqst,struct UserAccountDatabase * uadb,const char * username,const char * password)
{
  if ( (rqst==0) || (rqst->instance==0) || (uadb==0) || (username==0) || (password==0) ) { return 0; }

  struct UserAccountAuthenticationToken token={0};
  if ( ! uadb_authenticateUser(uadb,username,password,&token) ) { return 0; }

  //Session-fixation defense : never let a pre-login ( anonymous ) session ID become the authenticated one -
  //destroy whatever session this request currently has ( if any - useSessionLifecycle must be set for the
  //login resource for one to exist yet ) and issue a completely fresh one.
  if (rqst->sessionToken[0]!=0) { sessionList_Destroy(rqst->instance->sessionList,rqst->sessionToken); }

  unsigned int isNewSession=0;
  if ( ! getSessionFromHeader(rqst->instance->sessionList,0,0,0,rqst->sessionToken,sizeof(rqst->sessionToken),&isNewSession) )
  {
    return 0;
  }
  AmmServer_SetCookie(rqst,SESSION_COOKIE_NAME,rqst->sessionToken,0,1);

  _SESSIONset(rqst,"username",username);
  char uidStr[16]={0};
  snprintf(uidStr,sizeof(uidStr),"%u",token.uid);
  _SESSIONset(rqst,"uid",uidStr);

  return 1;
}

int AmmServer_Logout(struct AmmServer_DynamicRequest * rqst)
{
  if ( (rqst==0) || (rqst->instance==0) ) { return 0; }
  if (rqst->sessionToken[0]==0) { return 0; }

  sessionList_Destroy(rqst->instance->sessionList,rqst->sessionToken);
  AmmServer_SetCookie(rqst,SESSION_COOKIE_NAME,"",-1,1);
  rqst->sessionToken[0]=0;

  return 1;
}

int AmmServer_CurrentUsername(struct AmmServer_DynamicRequest * rqst,char * outUsername,unsigned int outUsernameSize)
{
  return _SESSIONcpy(rqst,"username",outUsername,outUsernameSize);
}
