#ifndef CHAT_H_INCLUDED
#define CHAT_H_INCLUDED

#include "session.h"

/*Creates the chat directory and makes sure the default room exists , so the room list is never empty..*/
int initializeChat();

void * chatPage_callback(struct AmmServer_DynamicRequest  * rqst);
void * chatSpeak_callback(struct AmmServer_DynamicRequest  * rqst);
void * chatMessages_callback(struct AmmServer_DynamicRequest  * rqst);
void * createRoom_callback(struct AmmServer_DynamicRequest  * rqst);
void * chatMedia_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // CHAT_H_INCLUDED
