/** @file epollFastPathServer.h
* @brief Serves simple cached-static GET/HEAD requests directly from the accept epoll layer's thread , without
*        ever creating/reusing a worker thread. Anything not obviously simple ( POST , query strings , Range ,
*        ETag/If-Modified-Since , compression negotiation , password protection , uncached/on-disk files ,
*        dynamic SAME_PAGE/DIFFERENT_PAGE resources ) falls back to the existing worker-thread pipeline
*        completely unchanged - see /home/ammar/.claude/plans/cheeky-fluttering-valley.md for the full design.
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef EPOLL_FAST_PATH_SERVER_H_INCLUDED
#define EPOLL_FAST_PATH_SERVER_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Called by the accept epoll layer thread on EPOLLIN for a plain HTTP connection , before it would
*        otherwise fall into dispatch_accepted_client. Uses MSG_PEEK to check eligibility before consuming
*        anything , so a return of 0 guarantees the socket is untouched and the normal dispatch path will see
*        the exact same bytes it would have seen if this function had never run.
* @ingroup threads
* @param An AmmarServer Instance
* @param The client socket that just became readable
* @retval 1=Fully handled ( served , queued a partial write , or closed - caller must not touch clientsock again )
           0=Not fast-path eligible , nothing was consumed , caller should dispatch to a worker thread as usual */
int EpollFastPath_TryServe(struct AmmServer_Instance * instance,int clientsock);

/**
* @brief Called by the accept epoll layer thread when an EPOLLOUT event's data.ptr is a fast-path pending write
*        ( a previous EpollFastPath_TryServe call hit EAGAIN partway through sending a response ). Resumes the
*        send from where it left off. Takes ownership of pending_write and frees it.
* @ingroup threads
* @param An AmmarServer Instance
* @param The struct FastPathPendingWrite* carried in the epoll event's data.ptr */
void EpollFastPath_ResumeWrite(struct AmmServer_Instance * instance,void * pending_write);

#ifdef __cplusplus
}
#endif

#endif // EPOLL_FAST_PATH_SERVER_H_INCLUDED
