/** @file client_list.h
* @brief Client list for IPs that should also serve as a banlist manage QoS etc
* @author Ammar Qammaz (AmmarkoV)
* @bug Client Lists are a stub and not implemented yet
*/

#ifndef CLIENT_LIST_H_INCLUDED
#define CLIENT_LIST_H_INCLUDED

#include "../../Hashmap/hashmap.h"


/** @brief Typedef to make clientID stand out */
typedef unsigned int clientID;

/** @brief The client list is just a hashmap ( see hashmap.h )  */
struct clientListContext
{
  struct hashMap * userList;
};

/**
* @brief Get the internal index id of an IP
* @ingroup clientList
* @param ClientList
* @param String containing the IP of the client we want to query
* @retval ID of client we searched for  */
unsigned int clientList_GetClientId(struct clientListContext * clientList, char * ip);

/**
* @brief Check if client ID is banned , therefore we should deny all service to him
* @ingroup clientList
* @param ClientList
* @param ClientID we are asking about
* @retval 1=Banned,0=OK*/
int clientList_isClientBanned(struct clientListContext * clientList,clientID client_id);

/**
* @brief Ask if the client is allowed to use resource
* @ingroup clientList
* @param ClientList
* @param ClientID we are talking about
* @param String of the resource
* @retval 1=Allowed,0=Denied*/
int clientList_isClientAllowedToUseResource(struct clientListContext * clientList,clientID client_id,char * resource);


int clientList_isClientAllowedToMakeAConnection(char * ip);

/**
* @brief Signal that resource has stopped beeing used for internal statistics
* @ingroup clientList
* @param ClientList
* @param ClientID we are talking about
* @param String of the resource
* @retval 1=Ok,0=Failed*/
int clientList_signalClientStoppedUsingResource(struct clientListContext * clientList,clientID client_id,char * resource);

/**
* @brief Create , allocate and return a client list
* @ingroup clientList
* @retval Pointer to a freshly allocated client list or 0=Failure */
struct clientListContext *  clientList_initialize(const char * serverName);

/**
* @brief Close and destroy client list
* @ingroup clientList
* @param ClientList to destroy
* @retval 1=Success,0=Failure */
int clientList_close(struct clientListContext * clientList);

#endif // CLIENT_LIST_H_INCLUDED
