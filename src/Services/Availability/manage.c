#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "manage.h"
#include "poll.h"

#define RESULTS_BUFFER_CAPACITY 65536
#define PAGE_BUFFER_CAPACITY (256*1024)

void * managePage_callback(struct AmmServer_DynamicRequest * rqst)
{
  char pollID[32]={0};
  _GETcpy(rqst,"poll",pollID,sizeof(pollID));
  char owner[40]={0};
  _GETcpy(rqst,"owner",owner,sizeof(owner));

  struct poll * p = findPoll(pollID);
  if ( (p==0) || (! isOwnerTokenValid(p,owner)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Poll not found or wrong manage link.</body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char escapedTitle[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(p->title,escapedTitle,sizeof(escapedTitle));

  char * resultsGrid = (char*) malloc(RESULTS_BUFFER_CAPACITY);
  if (resultsGrid==0) { return 0; }
  resultsGrid[0]=0;
  appendResultsGridHTML(resultsGrid,RESULTS_BUFFER_CAPACITY,p);

  char * optionRadios = (char*) malloc(MAX_OPTIONS*256);
  if (optionRadios==0) { free(resultsGrid); return 0; }
  optionRadios[0]=0;
  unsigned int i=0;
  for (i=0; i<p->numberOfOptions; i++)
  {
    char label[64]={0};
    formatOptionHTML(&p->options[i],label,sizeof(label));
    char row[300]={0};
    snprintf(row,sizeof(row),"<label style=\"display:block;font-weight:normal;\"><input type=\"radio\" name=\"optionIndex\" value=\"%u\" %s> %s</label>",
             i,(p->finalizedOptionIndex==(int)i)?"checked":"",label);
    strncat(optionRadios,row,MAX_OPTIONS*256 - strlen(optionRadios) - 1);
  }

  char finalizedNotice[128]={0};
  if (p->finalizedOptionIndex>=0)
  {
    char label[64]={0};
    formatOptionHTML(&p->options[p->finalizedOptionIndex],label,sizeof(label));
    snprintf(finalizedNotice,sizeof(finalizedNotice),"<p><b>Finalized: %s</b></p>",label);
  }

  char * page = (char*) malloc(PAGE_BUFFER_CAPACITY);
  if (page==0) { free(resultsGrid); free(optionRadios); return 0; }

  snprintf(page,PAGE_BUFFER_CAPACITY,
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Manage: %s - Availability</title>"
    "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"></head><body>"
    "<div class=\"topbar\"><h1>Availability</h1></div>"
    "<div class=\"container\">"
    "<div class=\"card\"><h2>Manage: %s</h2>"
    "<p>Share this link with participants: <code>vote.html?poll=%s</code></p>"
    "<p>Status: <b>%s</b></p>"
    "%s"
    "<form method=\"post\" enctype=\"multipart/form-data\" action=\"closePoll.html\" style=\"display:inline\">"
    "<input type=\"hidden\" name=\"poll\" value=\"%s\"><input type=\"hidden\" name=\"owner\" value=\"%s\">"
    "<button type=\"submit\">%s</button></form>"
    "</div>"
    "<div class=\"card\"><h3>Finalize a date</h3>"
    "<form method=\"post\" enctype=\"multipart/form-data\" action=\"finalizePoll.html\">"
    "<input type=\"hidden\" name=\"poll\" value=\"%s\"><input type=\"hidden\" name=\"owner\" value=\"%s\">"
    "%s"
    "<button type=\"submit\">Set as final</button></form>"
    "</div>"
    "<div class=\"card\"><h3>Responses</h3><div>%s</div></div>"
    "</div></body></html>",
    escapedTitle,
    escapedTitle,
    pollID,
    p->closed?"Closed":"Open",
    finalizedNotice,
    pollID,owner,p->closed?"Reopen":"Close poll",
    pollID,owner,
    optionRadios,
    resultsGrid);

  unsigned long copyLength = strlen(page);
  if (copyLength>=rqst->MAXcontentSize) { copyLength=rqst->MAXcontentSize-1; }
  memcpy(rqst->content,page,copyLength);
  rqst->contentSize=copyLength;

  free(page);
  free(resultsGrid);
  free(optionRadios);
  return 0;
}


void * closePoll_callback(struct AmmServer_DynamicRequest * rqst)
{
  char pollID[32]={0};
  _POSTcpy(rqst,"poll",pollID,sizeof(pollID));
  char owner[40]={0};
  _POSTcpy(rqst,"owner",owner,sizeof(owner));

  struct poll * p = findPoll(pollID);
  if ( (p!=0) && isOwnerTokenValid(p,owner) )
  {
    p->closed = !p->closed;
    savePollMeta(p);
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><meta http-equiv=\"refresh\" content=\"0; url=manage.html?poll=%s&owner=%s\"></head><body></body></html>",
           pollID,owner);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * finalizePoll_callback(struct AmmServer_DynamicRequest * rqst)
{
  char pollID[32]={0};
  _POSTcpy(rqst,"poll",pollID,sizeof(pollID));
  char owner[40]={0};
  _POSTcpy(rqst,"owner",owner,sizeof(owner));

  struct poll * p = findPoll(pollID);
  if ( (p!=0) && isOwnerTokenValid(p,owner) )
  {
    char optionIndexStr[16]={0};
    _POSTcpy(rqst,"optionIndex",optionIndexStr,sizeof(optionIndexStr));
    int optionIndex = atoi(optionIndexStr);
    if ( (optionIndex>=0) && (optionIndex<(int)p->numberOfOptions) )
    {
      p->finalizedOptionIndex = optionIndex;
      savePollMeta(p);
    }
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><meta http-equiv=\"refresh\" content=\"0; url=manage.html?poll=%s&owner=%s\"></head><body></body></html>",
           pollID,owner);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
