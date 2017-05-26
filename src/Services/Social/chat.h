#ifndef CHAT_H_INCLUDED
#define CHAT_H_INCLUDED

#include "../../AmmServerlib/AmmServerlib.h"

extern struct AmmServer_MemoryHandler * chatPage;

void * chat_callback(struct AmmServer_DynamicRequest  * rqst);
void * chatMessages_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // CHAT_H_INCLUDED
