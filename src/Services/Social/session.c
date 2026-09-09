#include "session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct UserAccountDatabase * uadb = 0;

int initializeLoginSystem()
{
  uadb = uadb_initializeUserAccountDatabase("db/users.db");
  return (uadb!=0);
}


int stopLoginSystem()
{
  uadb_closeUserAccountDatabase(&uadb);
  return 1;
}


void socialAppendChunk(char * buffer,unsigned int bufferSize,unsigned int * position,const char * chunk)
{
  unsigned int chunkLength=strlen(chunk);
  if (*position + chunkLength + 1 >= bufferSize) { return; }

  memcpy(buffer+*position,chunk,chunkLength);
  *position+=chunkLength;
  buffer[*position]=0;
}


void socialServeLoginRequired(struct AmmServer_DynamicRequest * rqst)
{
  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>The Social Gate</title>"
           "<link rel='stylesheet' type='text/css' href='social.css'></head>"
           "<body><div class=\"notice\">You need to be logged in for this. <a href=\"index.html\">Log in</a></div></body></html>");
  rqst->contentSize=strlen(rqst->content);
}


int socialPostedCSRFIsValid(struct AmmServer_DynamicRequest * rqst)
{
  char token[MAX_CSRF_TOKEN]={0};
  if ( ! _POSTcpy(rqst,"csrf",token,sizeof(token)) ) { return 0; }
  return AmmServer_ValidateCSRFToken(rqst,token);
}


void socialRedirectBack(struct AmmServer_DynamicRequest * rqst,const char * backUser)
{
  if ( (backUser!=0) && (uadbWeb_isValidUsername(backUser)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
             "<meta http-equiv=\"refresh\" content=\"0; url=profile.html?u=%s\"></head><body></body></html>",backUser);
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
             "<meta http-equiv=\"refresh\" content=\"0; url=home.html\"></head><body></body></html>");
  }
  rqst->contentSize=strlen(rqst->content);
}
