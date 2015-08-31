/** @file clientServer.h
* @brief This is the entry point to serve a client that picks a prespawned thread or creates a fresh new one and then handles the requests..
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef CLIENTSERVER_H_INCLUDED
#define CLIENTSERVER_H_INCLUDED

/**
* @brief Main Call to Serve a client , this will in turn pick a prespawned thread or create a new one
* @ingroup threads
* @param PassToHTTPThread with information to pass to the new thread ( prespawned or not )
* @retval This function returns 0  */
void * ServeClient(void * ptr);

#endif // CLIENTSERVER_H_INCLUDED
