#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "userAccountsWeb.h"

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
