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


void socialRenderTopbar(char * output,unsigned int outputSize,const char * escapedViewer,const char * csrfToken,unsigned int unseenNotifications)
{
  char badge[64]={0};
  if (unseenNotifications>0) { snprintf(badge,sizeof(badge)," <span class=\"badge\">%u</span>",unseenNotifications); }

  snprintf(output,outputSize,
           "<div class=\"topbar\">"
            "<img src=\"favicon.ico\" class=\"topbarlogo\"/>"
            "<div class=\"topbarlinks\">"
             "<a href=\"home.html\">Feed</a>"
             "<a href=\"home.html?all=1\">Everyone</a>"
             "<a href=\"people.html\">People</a>"
             "<a href=\"chat.html\">Chats</a>"
             "<a href=\"notifications.html\">Alerts%s</a>"
             "<a href=\"profile.html?u=%s\">%s</a>"
             "<form method=\"post\" enctype=\"multipart/form-data\" action=\"logout.html\">"
              "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
              "<button type=\"submit\">Log out</button>"
             "</form>"
            "</div>"
           "</div>",
           badge,escapedViewer,escapedViewer,csrfToken);
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
