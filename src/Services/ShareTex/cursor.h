#ifndef CURSOR_H_INCLUDED
#define CURSOR_H_INCLUDED

#include "state.h"

void * postCursor_callback(struct AmmServer_DynamicRequest * rqst);
void * pollCursors_callback(struct AmmServer_DynamicRequest * rqst);

#endif // CURSOR_H_INCLUDED
