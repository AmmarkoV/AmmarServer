/** @file session_list.h
* @brief Session list - a PHP-$_SESSION-style , cookie-keyed , in-memory key/value store per visitor. Sits
*        alongside cache/client_list.h as one of AmmServer_Instance's per-instance stores ( same hashmap-backed
*        pattern as the resource cache ). See knowledge/ammarserver.md and the session-implementation plan for
*        the full design - short version : one hashmap keyed by an unguessable session token string maps to a
*        small struct holding its own ( also hashmap-backed ) key/value data plus last-activity bookkeeping for
*        lazy idle-timeout eviction.
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef SESSION_LIST_H_INCLUDED
#define SESSION_LIST_H_INCLUDED

#include <pthread.h>
#include <time.h>

#include "../../Hashmap/hashmap.h"
#include "../server_configuration.h"

/** @brief One visitor's session : its own key/value store ( same shape as $_SESSION in PHP ) plus bookkeeping.
           `lock` guards `data` and `lastSeen` for this session specifically - session lookups/creation on the
           top-level table use sessionListContext::tableLock instead, so unrelated sessions never contend. */
struct sessionEntry
{
  char token[SESSION_TOKEN_STRING_SIZE];
  time_t created;
  time_t lastSeen;
  struct hashMap * data;
  pthread_mutex_t lock;
};

/** @brief The session list is a hashmap of token string -> struct sessionEntry* ( see hashmap.h ) */
struct sessionListContext
{
  struct hashMap * sessionTable;
  pthread_mutex_t tableLock;
};

struct sessionListContext * sessionList_initialize(const char * serverName);
int sessionList_close(struct sessionListContext * sessionList);

/** @brief Resolves the session for an incoming request from its session cookie value, creating a fresh one if
           `cookieValue` is empty/unknown/expired. Always succeeds ( barring OOM ) - PHP's session_start()
           auto-create behaviour, just triggered by the resource's useSessionLifecycle flag instead of an
           explicit call. Does not touch cookies/HTTP at all ; the caller ( dynamic_requests.c ) is responsible
           for calling AmmServer_SetCookie() when *isNewSession comes back 1.
* @param The session store
* @param Connection IP - currently unused, reserved for future IP-binding/anti-hijacking checks
* @param The session cookie value from the incoming request, or 0/empty if none was sent
* @param Browser identifier - currently unused, reserved for future fingerprint checks
* @param Output buffer that receives the resolved/created session token ( NUL-terminated )
* @param Size of that output buffer - should be at least SESSION_TOKEN_STRING_SIZE
* @param Output : set to 1 if a brand new session was created ( no cookie sent, or it didn't resolve ), else 0
* @retval 1=Success ( sessionTokenOut is valid ) , 0=Failure ( OOM ) */
int getSessionFromHeader(
                          struct sessionListContext * sessionList,
                          const char * connectionIP ,
                          const char * cookieValue ,
                          const char * BrowserIdentifier,
                          char * sessionTokenOut,
                          unsigned int sessionTokenOutSize,
                          unsigned int * isNewSession
                         );

/** @brief Destroys a session server-side immediately ( used by AmmServer_Logout() ) - the caller is still
           responsible for clearing the client's cookie separately via AmmServer_SetCookie().
* @retval 1=Destroyed,0=Not found */
int sessionList_Destroy(struct sessionListContext * sessionList,const char * sessionToken);

/** @brief Get a value previously stored with sessiontList_StoreInfo() for this session.
* @retval Pointer to a thread-local scratch copy of the value ( valid until this thread's next call into any
           sessiontList_*()/​_SESSION*() function - copy it out immediately if you need it longer ) , or 0 if
           not found / no such session. */
const char * sessiontList_GetInfo(struct sessionListContext * sessionList,const char * sessionToken,const char * name,unsigned int * valueLength);

/** @brief Set ( or overwrite ) a key/value pair on this session's data - the $_SESSION['name']=value; equivalent.
* @retval 1=Success,0=Failure ( no such session / OOM ) */
int sessiontList_StoreInfo(struct sessionListContext * sessionList,const char * sessionToken ,const char * name ,const char * value);

/** @brief Remove a single key from this session's data - the unset($_SESSION['name']); equivalent.
* @retval 1=Removed,0=Not found / no such session */
int sessiontList_UnsetInfo(struct sessionListContext * sessionList,const char * sessionToken,const char * name);

#endif // SESSION_LIST_H_INCLUDED
