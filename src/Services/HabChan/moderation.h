#ifndef MODERATION_H_INCLUDED
#define MODERATION_H_INCLUDED

#include "state.h"
#include "../../AmmServerlib/AmmServerlib.h"

//Deletes a single reply ( postIndex>0 ) if password matches that post's own password.
//The post stays in place ( so numbering / permalinks don't shift ) but is marked deleted and its
//message/image are wiped from memory and disk.
int deleteReply(const char * boardName , struct thread * t , unsigned int postIndex , const char * password);

//Deletes an entire thread ( identified by its OP , postIndex 0 ) if password matches the thread's password.
//The thread is hidden from every listing/view from then on ; its OP image is removed , the rest of its
//files are left on disk.
int deleteThread(const char * boardName , struct thread * t , const char * password);

void * processDeletePost(struct AmmServer_DynamicRequest * rqst);

#endif // MODERATION_H_INCLUDED
