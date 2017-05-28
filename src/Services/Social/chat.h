#ifndef CHAT_H_INCLUDED
#define CHAT_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"

extern struct AmmServer_MemoryHandler * chatPage;

void * chatSpeak_callback(struct AmmServer_DynamicRequest  * rqst);
void * chatMessages_callback(struct AmmServer_DynamicRequest  * rqst);

void * chatPage_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // CHAT_H_INCLUDED
