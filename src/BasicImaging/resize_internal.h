/** @file resize_internal.h
* @brief NOT part of the public API ( basicImaging.h ) - this exists only so BasicImaging's own
*        benchmark/profiling tooling can force a specific resize implementation for a fair, controlled
*        timing comparison. Real callers must never include this ; the public BasicImaging_Resize() /
*        BasicImaging_Thumbnail() always auto-dispatch to the best implementation the running CPU
*        supports, with exactly the same function signature regardless of which one actually runs.
*/
#ifndef RESIZE_INTERNAL_H_INCLUDED
#define RESIZE_INTERNAL_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

enum BasicImaging_ResizePath
{
  BASICIMAGING_RESIZE_AUTO = 0, //default : runtime CPU dispatch - what every real caller gets
  BASICIMAGING_RESIZE_SCALAR,
  BASICIMAGING_RESIZE_SSE2,
  BASICIMAGING_RESIZE_AVX2
};

/**
* @brief Force BasicImaging_Resize()/BasicImaging_Thumbnail() to use one specific implementation from
*        now on, for benchmarking/testing only. Pass BASICIMAGING_RESIZE_AUTO to return to normal
*        runtime CPU dispatch. Requesting an implementation the running CPU ( or this build ) doesn't
*        actually support silently falls back to the best one that IS available - this can never crash
*        a real caller , it can only make a benchmark request something it won't actually get.
*/
void BasicImaging_Resize_ForcePath(enum BasicImaging_ResizePath path);

/**
* @brief Which implementation would run right now ( honoring any forced path and this build/CPU's real
*        capabilities ) - lets a benchmark report what it actually measured instead of assuming.
*/
enum BasicImaging_ResizePath BasicImaging_Resize_ActivePath(void);

/** @brief Human-readable name for a BasicImaging_ResizePath value, e.g. "AVX2". Never returns NULL. */
const char * BasicImaging_Resize_PathName(enum BasicImaging_ResizePath path);

#ifdef __cplusplus
}
#endif

#endif // RESIZE_INTERNAL_H_INCLUDED
