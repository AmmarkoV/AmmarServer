#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poll.h"

#define RESULTS_BUFFER_CAPACITY 65536
#define PAGE_BUFFER_CAPACITY (256*1024)

static const char * voteLabel(unsigned char v)
{
  switch (v)
  {
    case VOTE_YES:   return "Yes";
    case VOTE_MAYBE: return "Maybe";
    case VOTE_NO:    return "No";
    default:         return "&ndash;";
  }
}

static const char * voteCellClass(unsigned char v)
{
  switch (v)
  {
    case VOTE_YES:   return "voteYes";
    case VOTE_MAYBE: return "voteMaybe";
    case VOTE_NO:    return "voteNo";
    default:         return "voteBlank";
  }
}


void appendResultsGridHTML(char * buffer , unsigned int bufferCapacity , struct poll * p)
{
  if (p->numberOfResponses==0)
  {
    strncat(buffer,"<p style=\"color:#777;\">No responses yet.</p>",bufferCapacity - strlen(buffer) - 1);
    return;
  }

  strncat(buffer,"<div style=\"overflow-x:auto;\"><table class=\"resultsTable\"><tr><th>Name</th>",bufferCapacity - strlen(buffer) - 1);

  unsigned int i=0,j=0;
  for (i=0; i<p->numberOfOptions; i++)
  {
    char label[64]={0};
    formatOptionHTML(&p->options[i],label,sizeof(label));
    char chunk[128]={0};
    snprintf(chunk,sizeof(chunk),"<th>%s</th>",label);
    strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
  }
  strncat(buffer,"</tr>",bufferCapacity - strlen(buffer) - 1);

  for (i=0; i<p->numberOfResponses; i++)
  {
    struct pollResponse * r = &p->responses[i];
    char escapedName[64*6]={0};
    AmmServer_HTMLEscape(r->name,escapedName,sizeof(escapedName));

    char row[MAX_OPTIONS*80+256]={0};
    snprintf(row,sizeof(row),"<tr><td>%s</td>",escapedName);

    for (j=0; j<p->numberOfOptions; j++)
    {
      char cell[128]={0};
      snprintf(cell,sizeof(cell),"<td class=\"%s\">%s</td>",voteCellClass(r->votes[j]),voteLabel(r->votes[j]));
      strncat(row,cell,sizeof(row)-strlen(row)-1);
    }
    strncat(row,"</tr>",sizeof(row)-strlen(row)-1);
    strncat(buffer,row,bufferCapacity - strlen(buffer) - 1);
  }

  strncat(buffer,"<tr class=\"tallyRow\"><td><b>Tally</b></td>",bufferCapacity - strlen(buffer) - 1);
  for (j=0; j<p->numberOfOptions; j++)
  {
    unsigned int yes=0,maybe=0,no=0;
    for (i=0; i<p->numberOfResponses; i++)
    {
      if      (p->responses[i].votes[j]==VOTE_YES)   { ++yes;   }
      else if (p->responses[i].votes[j]==VOTE_MAYBE) { ++maybe; }
      else if (p->responses[i].votes[j]==VOTE_NO)    { ++no;    }
    }
    char cell[96]={0};
    snprintf(cell,sizeof(cell),"<td style=\"font-size:11px;\">Y:%u M:%u N:%u</td>",yes,maybe,no);
    strncat(buffer,cell,bufferCapacity - strlen(buffer) - 1);
  }
  strncat(buffer,"</tr></table></div>",bufferCapacity - strlen(buffer) - 1);
}


void * pollResultsFragment_callback(struct AmmServer_DynamicRequest * rqst)
{
  char pollID[32]={0};
  _GETcpy(rqst,"poll",pollID,sizeof(pollID));
  struct poll * p = findPoll(pollID);

  rqst->content[0]=0;
  if (p!=0) { appendResultsGridHTML(rqst->content,rqst->MAXcontentSize,p); }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


static void appendOptionRowsHTML(char * buffer , unsigned int bufferCapacity , struct poll * p , struct pollResponse * existing)
{
  unsigned int i=0;
  for (i=0; i<p->numberOfOptions; i++)
  {
    char label[64]={0};
    formatOptionHTML(&p->options[i],label,sizeof(label));

    unsigned char prefill = (existing!=0) ? existing->votes[i] : VOTE_BLANK;

    char row[900]={0};
    snprintf(row,sizeof(row),
      "<tr><td>%s</td>"
      "<td><input type=\"radio\" name=\"vote%u\" value=\"1\" %s></td>"
      "<td><input type=\"radio\" name=\"vote%u\" value=\"2\" %s></td>"
      "<td><input type=\"radio\" name=\"vote%u\" value=\"3\" %s></td>"
      "<td><input type=\"radio\" name=\"vote%u\" value=\"0\" %s></td></tr>",
      label,
      i,(prefill==VOTE_YES)?"checked":"",
      i,(prefill==VOTE_MAYBE)?"checked":"",
      i,(prefill==VOTE_NO)?"checked":"",
      i,(prefill==VOTE_BLANK)?"checked":"");
    strncat(buffer,row,bufferCapacity - strlen(buffer) - 1);
  }
}


void * votePage_callback(struct AmmServer_DynamicRequest * rqst)
{
  char pollID[32]={0};
  _GETcpy(rqst,"poll",pollID,sizeof(pollID));

  struct poll * p = findPoll(pollID);
  if (p==0)
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Poll not found.</body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char prefillName[64]={0};
  _GETcpy(rqst,"name",prefillName,sizeof(prefillName));
  int existingIndex = findResponseByName(p,prefillName);
  struct pollResponse * existing = (existingIndex>=0) ? &p->responses[existingIndex] : 0;

  char escapedTitle[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(p->title,escapedTitle,sizeof(escapedTitle));

  char escapedName[64*6]={0};
  AmmServer_HTMLEscape(prefillName,escapedName,sizeof(escapedName));

  char escapedEmail[128*6]={0};
  if (existing!=0) { AmmServer_HTMLEscape(existing->email,escapedEmail,sizeof(escapedEmail)); }

  char * optionRows = (char*) malloc(MAX_OPTIONS*1024);
  char * resultsGrid = (char*) malloc(RESULTS_BUFFER_CAPACITY);
  if ( (optionRows==0) || (resultsGrid==0) ) { return 0; }
  optionRows[0]=0;
  resultsGrid[0]=0;

  appendOptionRowsHTML(optionRows,MAX_OPTIONS*1024,p,existing);
  appendResultsGridHTML(resultsGrid,RESULTS_BUFFER_CAPACITY,p);

  char voteFormHTML[512]={0};
  if (p->closed)
  {
    snprintf(voteFormHTML,sizeof(voteFormHTML),"<p><b>This poll is closed to new responses.</b></p>");
  }

  char * page = (char*) malloc(PAGE_BUFFER_CAPACITY);
  if (page==0) { return 0; }

  snprintf(page,PAGE_BUFFER_CAPACITY,
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>%s - Availability</title>"
    "<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\"></head><body>"
    "<div class=\"topbar\"><h1>Availability</h1></div>"
    "<div class=\"container\">"
    "<div class=\"card\"><h2>%s</h2>%s"
    "<form id=\"voteForm\" method=\"post\" enctype=\"multipart/form-data\" action=\"submitVote.html\">"
    "<input type=\"hidden\" name=\"poll\" value=\"%s\">"
    "<label>Your name</label><input type=\"text\" name=\"name\" id=\"nameField\" value=\"%s\" required %s>"
    "<label>Email (optional)</label><input type=\"email\" name=\"email\" value=\"%s\" %s>"
    "<table class=\"voteTable\"><tr><th>Option</th><th>Yes</th><th>Maybe</th><th>No</th><th>?</th></tr>%s</table>"
    "<button type=\"submit\" %s>Submit</button>"
    "</form>"
    "</div>"
    "<div class=\"card\"><h3>Responses so far</h3><div id=\"resultsGrid\">%s</div></div>"
    "</div>"
    "<input type=\"hidden\" id=\"pollID\" value=\"%s\">"
    "<script src=\"poll.js\"></script>"
    "</body></html>",
    escapedTitle,
    escapedTitle,voteFormHTML,
    pollID,
    escapedName,p->closed?"disabled":"",
    escapedEmail,p->closed?"disabled":"",
    optionRows,
    p->closed?"disabled":"",
    resultsGrid,
    pollID);

  unsigned long copyLength = strlen(page);
  if (copyLength>=rqst->MAXcontentSize) { copyLength=rqst->MAXcontentSize-1; }
  memcpy(rqst->content,page,copyLength);
  rqst->contentSize=copyLength;

  free(page);
  free(optionRows);
  free(resultsGrid);
  return 0;
}


void * submitVote_callback(struct AmmServer_DynamicRequest * rqst)
{
  char pollID[32]={0};
  _POSTcpy(rqst,"poll",pollID,sizeof(pollID));
  struct poll * p = findPoll(pollID);

  char name[64]={0};
  _POSTcpy(rqst,"name",name,sizeof(name));

  if ( (p!=0) && (!p->closed) && (strlen(name)>0) )
  {
    int idx = findOrCreateResponseSlot(p,name);
    if (idx>=0)
    {
      struct pollResponse * r = &p->responses[idx];

      char email[128]={0};
      if ( _POSTcpy(rqst,"email",email,sizeof(email)) && (strlen(email)>0) )
      {
        r->hasEmail=1;
        snprintf(r->email,sizeof(r->email),"%s",email);
      }

      unsigned int i=0;
      for (i=0; i<p->numberOfOptions; i++)
      {
        char fieldName[16]={0};
        snprintf(fieldName,sizeof(fieldName),"vote%u",i);
        char voteStr[8]={0};
        _POSTcpy(rqst,fieldName,voteStr,sizeof(voteStr));
        r->votes[i] = (unsigned char) atoi(voteStr);
      }

      saveResponse(p,(unsigned int)idx);
    }
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><meta http-equiv=\"refresh\" content=\"0; url=vote.html?poll=%s&name=%s\"></head>"
           "<body>Thanks , redirecting..</body></html>",
           pollID,name);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * createPoll_callback(struct AmmServer_DynamicRequest * rqst)
{
  char title[MAX_STRING_SIZE]={0};
  _POSTcpy(rqst,"title",title,sizeof(title));
  if (strlen(title)==0) { snprintf(title,sizeof(title),"Untitled Poll"); }

  struct pollOption options[MAX_OPTIONS]={{0}};
  unsigned int numberOfOptions=0;

  unsigned int i=0;
  for (i=0; i<MAX_OPTIONS; i++)
  {
    char fieldName[32]={0};
    snprintf(fieldName,sizeof(fieldName),"optionDate%u",i);
    char dateStr[32]={0};
    if ( (!_POSTcpy(rqst,fieldName,dateStr,sizeof(dateStr))) || (strlen(dateStr)==0) ) { continue; }

    unsigned int y=0,mo=0,d=0;
    if (sscanf(dateStr,"%u-%u-%u",&y,&mo,&d)!=3) { continue; }

    struct pollOption opt={0};
    opt.year=y; opt.month=mo; opt.day=d;

    snprintf(fieldName,sizeof(fieldName),"optionStartTime%u",i);
    char startStr[16]={0};
    if ( _POSTcpy(rqst,fieldName,startStr,sizeof(startStr)) && (strlen(startStr)>0) )
    {
      unsigned int sh=0,sm=0;
      if (sscanf(startStr,"%u:%u",&sh,&sm)==2) { opt.hasStartTime=1; opt.startHour=sh; opt.startMinute=sm; }
    }

    snprintf(fieldName,sizeof(fieldName),"optionEndTime%u",i);
    char endStr[16]={0};
    if ( _POSTcpy(rqst,fieldName,endStr,sizeof(endStr)) && (strlen(endStr)>0) )
    {
      unsigned int eh=0,em=0;
      if (sscanf(endStr,"%u:%u",&eh,&em)==2) { opt.hasEndTime=1; opt.endHour=eh; opt.endMinute=em; }
    }

    options[numberOfOptions]=opt;
    ++numberOfOptions;
  }

  if (numberOfOptions==0)
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Please add at least one valid date option. <a href=\"index.html\">Back</a></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  char newID[32]={0};
  char ownerToken[40]={0};
  if ( createPoll(title,options,numberOfOptions,newID,sizeof(newID),ownerToken,sizeof(ownerToken)) )
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<html><head><meta http-equiv=\"refresh\" content=\"0; url=manage.html?poll=%s&owner=%s\"></head>"
             "<body>Creating poll , redirecting..</body></html>",
             newID,ownerToken);
  } else
  {
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Could not create poll. <a href=\"index.html\">Back</a></body></html>");
  }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}
