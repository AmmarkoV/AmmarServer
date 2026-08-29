#include "session_list.h"
#include "../AmmServerlib.h"
#include "../tools/http_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Value copies handed back by sessiontList_GetInfo() live here - safe against another thread concurrently
//mutating/freeing the same key's storage in the hashmap ( sessions are shared across every request that
//presents the same cookie, unlike GET/POST/COOKIE items which are private to one request's own stack ), and
//still valid past the lock being released. Scoped per-thread, so different requests never clobber each other's
//copy ; a given thread's previous copy is only overwritten by that same thread's next session-value read.
#define SESSION_VALUE_SCRATCH_SIZE 4096
static __thread char sessionValueScratch[SESSION_VALUE_SCRATCH_SIZE];

static void destroySessionEntryCallback(void * payload)
{
  struct sessionEntry * entry = (struct sessionEntry *) payload;
  if (entry==0) { return; }
  if (entry->data!=0) { hashMap_Destroy(entry->data); }
  pthread_mutex_destroy(&entry->lock);
  free(entry);
}

struct sessionListContext * sessionList_initialize(const char * serverName)
{
  struct sessionListContext * ctx = (struct sessionListContext *) malloc(sizeof(struct sessionListContext));
  if (ctx==0) { return 0; }

  ctx->sessionTable = hashMap_Create(64,64,(void*)destroySessionEntryCallback,0);
  if (ctx->sessionTable==0) { free(ctx); return 0; }

  pthread_mutex_init(&ctx->tableLock,0);
  return ctx;
}

int sessionList_close(struct sessionListContext * sessionList)
{
  if (sessionList==0) { return 0; }
  if (sessionList->sessionTable!=0) { hashMap_Destroy(sessionList->sessionTable); }
  pthread_mutex_destroy(&sessionList->tableLock);
  free(sessionList);
  return 1;
}

static int sessionIsExpired(struct sessionEntry * entry)
{
  return ( (time(0) - entry->lastSeen) > SESSION_IDLE_TIMEOUT_SECONDS );
}

//Caller must already hold sessionList->tableLock
static void evictOldestSessionIfAtCapacity(struct sessionListContext * sessionList)
{
  if (hashMap_GetCurrentNumberOfEntries(sessionList->sessionTable) < MAX_SESSIONS) { return; }

  unsigned int n = (unsigned int) hashMap_GetCurrentNumberOfEntries(sessionList->sessionTable);
  unsigned int i=0;
  int found=0;
  time_t oldest=0;
  char oldestKey[SESSION_TOKEN_STRING_SIZE]={0};

  for (i=0; i<n; i++)
  {
    unsigned int payloadLength=0;
    struct sessionEntry * e = (struct sessionEntry *) hashMap_GetPayloadAtIndex(sessionList->sessionTable,i,&payloadLength);
    if (e==0) { continue; }
    if ( (!found) || (e->lastSeen<oldest) )
    {
      found=1;
      oldest=e->lastSeen;
      char * key = hashMap_GetKeyAtIndex(sessionList->sessionTable,i);
      if (key!=0) { snprintf(oldestKey,SESSION_TOKEN_STRING_SIZE,"%s",key); }
    }
  }

  if (found) { hashMap_RemoveKey(sessionList->sessionTable,oldestKey); }
}

//Caller must already hold sessionList->tableLock
static struct sessionEntry * createSessionLocked(struct sessionListContext * sessionList,char * tokenOut,unsigned int tokenOutSize)
{
  evictOldestSessionIfAtCapacity(sessionList);

  struct sessionEntry * entry = (struct sessionEntry *) malloc(sizeof(struct sessionEntry));
  if (entry==0) { return 0; }
  memset(entry,0,sizeof(struct sessionEntry));

  //A collision here would need two independent 256-bit CSPRNG draws to match - the bounded retry is purely
  //defensive ( e.g. against a broken/exhausted entropy source ), not a realistic scenario in practice.
  unsigned int tries=0;
  do
  {
    if (!AmmServer_GenerateSecureToken(entry->token,sizeof(entry->token),SESSION_TOKEN_RANDOM_BYTES))
    {
      free(entry);
      return 0;
    }
    ++tries;
  } while ( (hashMap_ContainsKey(sessionList->sessionTable,entry->token)) && (tries<5) );

  entry->created  = time(0);
  entry->lastSeen = entry->created;
  entry->data = hashMap_Create(8,8,0,0);
  if (entry->data==0) { free(entry); return 0; }
  pthread_mutex_init(&entry->lock,0);

  //valLength=0 : store the pointer directly, freed later via destroySessionEntryCallback ( same pattern the
  //cache/dynamic-request code already uses for callback function pointers stored as void* )
  if (!hashMap_Add(sessionList->sessionTable,entry->token,entry,0))
  {
    hashMap_Destroy(entry->data);
    pthread_mutex_destroy(&entry->lock);
    free(entry);
    return 0;
  }

  snprintf(tokenOut,tokenOutSize,"%s",entry->token);
  return entry;
}

//Caller must already hold sessionList->tableLock. Lazily evicts ( and returns 0 for ) an expired session.
static struct sessionEntry * findSessionLocked(struct sessionListContext * sessionList,const char * token)
{
  if ( (token==0) || (token[0]==0) ) { return 0; }

  unsigned long index=0;
  if (!hashMap_FindIndexSerial(sessionList->sessionTable,token,&index)) { return 0; }

  unsigned int payloadLength=0;
  struct sessionEntry * entry = (struct sessionEntry *) hashMap_GetPayloadAtIndex(sessionList->sessionTable,(unsigned int)index,&payloadLength);
  if (entry==0) { return 0; }

  if (sessionIsExpired(entry))
  {
    hashMap_RemoveKey(sessionList->sessionTable,token);
    return 0;
  }

  return entry;
}

int getSessionFromHeader(
                          struct sessionListContext * sessionList,
                          const char * connectionIP ,
                          const char * cookieValue ,
                          const char * BrowserIdentifier,
                          char * sessionTokenOut,
                          unsigned int sessionTokenOutSize,
                          unsigned int * isNewSession
                         )
{
  if ( (sessionList==0) || (sessionTokenOut==0) ) { return 0; }
  if (isNewSession!=0) { *isNewSession=0; }

  pthread_mutex_lock(&sessionList->tableLock);

  struct sessionEntry * entry = findSessionLocked(sessionList,cookieValue);
  if (entry!=0)
  {
    entry->lastSeen = time(0);
    snprintf(sessionTokenOut,sessionTokenOutSize,"%s",entry->token);
    pthread_mutex_unlock(&sessionList->tableLock);
    return 1;
  }

  entry = createSessionLocked(sessionList,sessionTokenOut,sessionTokenOutSize);
  pthread_mutex_unlock(&sessionList->tableLock);

  if (entry==0) { return 0; }
  if (isNewSession!=0) { *isNewSession=1; }
  return 1;
}

int sessionList_Destroy(struct sessionListContext * sessionList,const char * sessionToken)
{
  if ( (sessionList==0) || (sessionToken==0) ) { return 0; }
  pthread_mutex_lock(&sessionList->tableLock);
  int ok = hashMap_RemoveKey(sessionList->sessionTable,sessionToken);
  pthread_mutex_unlock(&sessionList->tableLock);
  return ok;
}

const char * sessiontList_GetInfo(struct sessionListContext * sessionList,const char * sessionToken,const char * name,unsigned int * valueLength)
{
  if (valueLength!=0) { *valueLength=0; }
  if ( (sessionList==0) || (sessionToken==0) || (name==0) ) { return 0; }

  pthread_mutex_lock(&sessionList->tableLock);
  struct sessionEntry * entry = findSessionLocked(sessionList,sessionToken);
  pthread_mutex_unlock(&sessionList->tableLock);
  if (entry==0) { return 0; }

  const char * result=0;

  pthread_mutex_lock(&entry->lock);
  unsigned long index=0;
  if (hashMap_FindIndexSerial(entry->data,name,&index))
  {
    unsigned int payloadLength=0;
    char * stored = (char *) hashMap_GetPayloadAtIndex(entry->data,(unsigned int)index,&payloadLength);
    if (stored!=0)
    {
      //Stored payloadLength includes the NUL sessiontList_StoreInfo() appended ( strlen(value)+1 )
      unsigned int copyLength = (payloadLength>0) ? (payloadLength-1) : 0;
      if (copyLength>=SESSION_VALUE_SCRATCH_SIZE) { copyLength=SESSION_VALUE_SCRATCH_SIZE-1; }
      memcpy(sessionValueScratch,stored,copyLength);
      sessionValueScratch[copyLength]=0;
      result = sessionValueScratch;
      if (valueLength!=0) { *valueLength=copyLength; }
    }
  }
  pthread_mutex_unlock(&entry->lock);

  return result;
}

int sessiontList_StoreInfo(struct sessionListContext * sessionList,const char * sessionToken ,const char * name ,const char * value)
{
  if ( (sessionList==0) || (sessionToken==0) || (name==0) || (value==0) ) { return 0; }

  pthread_mutex_lock(&sessionList->tableLock);
  struct sessionEntry * entry = findSessionLocked(sessionList,sessionToken);
  pthread_mutex_unlock(&sessionList->tableLock);
  if (entry==0) { return 0; }

  pthread_mutex_lock(&entry->lock);
  //hashMap_Add() is append-only ( no in-place update ) - remove any previous value for this key first so
  //repeated _SESSIONset() calls on the same key don't leak stale duplicate entries or leave lookups ambiguous.
  if (hashMap_ContainsKey(entry->data,name)) { hashMap_RemoveKey(entry->data,name); }
  int ok = hashMap_Add(entry->data,name,(void*)value,strlen(value)+1);
  pthread_mutex_unlock(&entry->lock);

  return ok;
}

int sessiontList_UnsetInfo(struct sessionListContext * sessionList,const char * sessionToken,const char * name)
{
  if ( (sessionList==0) || (sessionToken==0) || (name==0) ) { return 0; }

  pthread_mutex_lock(&sessionList->tableLock);
  struct sessionEntry * entry = findSessionLocked(sessionList,sessionToken);
  pthread_mutex_unlock(&sessionList->tableLock);
  if (entry==0) { return 0; }

  pthread_mutex_lock(&entry->lock);
  int ok = hashMap_RemoveKey(entry->data,name);
  pthread_mutex_unlock(&entry->lock);

  return ok;
}
