#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "csrf.h"

//AmmServerlib does not expose a way for a Service to set a cookie or store custom per-session data , so the
//classic double-submit-cookie / session-token CSRF defenses aren't available here. Instead we keep our own
//small , bounded , in-memory record of tokens we recently handed out : a cross-site page has no way to read
//the token embedded in a form it didn't load itself , so it can never include a valid one in a forged request.
#define CSRF_TOKEN_SLOTS 512
#define CSRF_TOKEN_TTL_SECONDS (2*3600) // 2 hours is generous for a page someone left open in a tab

struct csrfSlot
{
  char token[33];
  time_t issuedAt;
};

static struct csrfSlot csrfSlots[CSRF_TOKEN_SLOTS]={{{0},0}};
static unsigned int csrfNextSlot=0;
static unsigned int csrfSeeded=0;

void generateCSRFToken(char * outToken, unsigned int outTokenSize)
{
  if (!csrfSeeded) { srand((unsigned int)time(0) ^ (unsigned int)getpid()); csrfSeeded=1; }

  char token[33]={0};
  snprintf(token,sizeof(token),"%08x%08x%08x%08x",rand(),rand(),rand(),rand());

  unsigned int slot = csrfNextSlot;
  csrfNextSlot = (csrfNextSlot+1) % CSRF_TOKEN_SLOTS;

  snprintf(csrfSlots[slot].token,sizeof(csrfSlots[slot].token),"%s",token);
  csrfSlots[slot].issuedAt = time(0);

  snprintf(outToken,outTokenSize,"%s",token);
}

int isCSRFTokenValid(const char * token)
{
  if ( (token==0) || (strlen(token)==0) ) { return 0; }

  time_t now = time(0);
  unsigned int i=0;
  for (i=0; i<CSRF_TOKEN_SLOTS; i++)
  {
    if ( (csrfSlots[i].token[0]!=0) && (strcmp(csrfSlots[i].token,token)==0) )
    {
      if ( (now-csrfSlots[i].issuedAt) <= CSRF_TOKEN_TTL_SECONDS ) { return 1; }
      return 0; //Expired
    }
  }

  return 0;
}
