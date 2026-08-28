#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED

#include "state.h"

void * editorPage_callback(struct AmmServer_DynamicRequest * rqst);
void * getFileContent_callback(struct AmmServer_DynamicRequest * rqst);
void * saveFileContent_callback(struct AmmServer_DynamicRequest * rqst);
void * newFile_callback(struct AmmServer_DynamicRequest * rqst);

#endif // EDITOR_H_INCLUDED
