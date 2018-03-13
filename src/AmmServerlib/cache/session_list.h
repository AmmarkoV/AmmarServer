/** @file session_list.h
* @brief Session list that can facilitate server storage for login/logout control etc..
* @author Ammar Qammaz (AmmarkoV)
* @bug Session Lists are a stub and not implemented yet
*/

#ifndef SESSION_LIST_H_INCLUDED
#define SESSION_LIST_H_INCLUDED

#include "../../Hashmap/hashmap.h"


/** @brief Typedef to make clientID stand out */
typedef unsigned int sessionID;

/** @brief The session list is just a hashmap ( see hashmap.h )  */
struct sessionListContext
{
  struct hashMap * userList;
};

int sessiontList_StoreInfo(struct sessionListContext * sessionList,const char * sessionName ,const char * name ,const char * value);
const char * sessiontList_GetInfo(struct sessionListContext * sessionList,sessionID id,const char * name);

int getSessionFromHeader(
                          struct sessionListContext * sessionList,
                          const char * connectionIP ,
                          const char * cookieValue ,
                          const char * BrowserIdentifier);

struct sessionListContext *  sessionList_initialize(const char * serverName);
int sessionList_close(struct sessionListContext * sessionList);


#endif // CLIENT_LIST_H_INCLUDED
