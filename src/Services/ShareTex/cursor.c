#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cursor.h"
#include "auth.h"

#define MAX_CURSOR_ENTRIES 500
#define CURSOR_TTL_SECONDS 10

struct cursorEntry
{
  unsigned char used;
  char projectID[32];
  char relativePath[MAX_STRING_SIZE];
  char username[64];
  unsigned int offset;
  time_t lastSeen;
};

static struct cursorEntry cursors[MAX_CURSOR_ENTRIES]={{0}};

static int findCursorSlot(const char * projectID , const char * relativePath , const char * username)
{
  unsigned int i=0;
  for (i=0; i<MAX_CURSOR_ENTRIES; i++)
  {
    if ( (cursors[i].used) &&
         (strcmp(cursors[i].projectID,projectID)==0) &&
         (strcmp(cursors[i].relativePath,relativePath)==0) &&
         (strcmp(cursors[i].username,username)==0) )
    {
      return (int) i;
    }
  }
  return -1;
}

static int findFreeOrOldestCursorSlot()
{
  unsigned int i=0;
  time_t oldest=0;
  int oldestIndex=0;
  for (i=0; i<MAX_CURSOR_ENTRIES; i++)
  {
    if (!cursors[i].used) { return (int) i; }
    if ( (oldest==0) || (cursors[i].lastSeen<oldest) ) { oldest=cursors[i].lastSeen; oldestIndex=(int)i; }
  }
  return oldestIndex;
}


void * postCursor_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  unsigned int authenticated = getAuthenticatedUser(rqst,username,sizeof(username));

  char projectID[32]={0};
  _POSTcpy(rqst,"project",projectID,sizeof(projectID));
  char relativePath[MAX_STRING_SIZE]={0};
  _POSTcpy(rqst,"file",relativePath,sizeof(relativePath));
  char offsetStr[16]={0};
  _POSTcpy(rqst,"offset",offsetStr,sizeof(offsetStr));

  struct project * p = authenticated ? findProject(projectID) : 0;

  if ( (p!=0) && userCanAccessProject(p,username) && isSafeRelativePath(relativePath) )
  {
    int slot = findCursorSlot(projectID,relativePath,username);
    if (slot<0) { slot=findFreeOrOldestCursorSlot(); }

    cursors[slot].used=1;
    snprintf(cursors[slot].projectID,sizeof(cursors[slot].projectID),"%s",projectID);
    snprintf(cursors[slot].relativePath,sizeof(cursors[slot].relativePath),"%s",relativePath);
    snprintf(cursors[slot].username,sizeof(cursors[slot].username),"%s",username);
    cursors[slot].offset=(unsigned int) atoi(offsetStr);
    cursors[slot].lastSeen=time(0);
  }

  snprintf(rqst->content,rqst->MAXcontentSize,"ok");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * pollCursors_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  unsigned int authenticated = getAuthenticatedUser(rqst,username,sizeof(username));

  char projectID[32]={0};
  _GETcpy(rqst,"project",projectID,sizeof(projectID));
  char relativePath[MAX_STRING_SIZE]={0};
  _GETcpy(rqst,"file",relativePath,sizeof(relativePath));

  struct project * p = authenticated ? findProject(projectID) : 0;

  rqst->content[0]=0;

  if ( (p!=0) && userCanAccessProject(p,username) )
  {
    time_t now = time(0);
    unsigned int i=0;
    for (i=0; i<MAX_CURSOR_ENTRIES; i++)
    {
      if ( (cursors[i].used) &&
           (strcmp(cursors[i].projectID,projectID)==0) &&
           (strcmp(cursors[i].relativePath,relativePath)==0) &&
           (strcmp(cursors[i].username,username)!=0) &&
           ( (now-cursors[i].lastSeen) < CURSOR_TTL_SECONDS ) )
      {
        char line[64*6+32]={0};
        char escapedUsername[64*6]={0};
        AmmServer_HTMLEscape(cursors[i].username,escapedUsername,sizeof(escapedUsername));
        snprintf(line,sizeof(line),"%s|%u\n",escapedUsername,cursors[i].offset);
        strncat(rqst->content,line,rqst->MAXcontentSize - strlen(rqst->content) - 1);
      }
    }
  }

  rqst->contentSize=strlen(rqst->content);
  return 0;
}
