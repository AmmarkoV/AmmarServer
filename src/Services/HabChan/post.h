#ifndef POST_H_INCLUDED
#define POST_H_INCLUDED

#include "state.h"


int loadPosts(struct board * ourBoard , struct thread * ourThread);

int savePostHeader(const char * postHeaderFilename , struct post * ourPost);
int savePostContent(const char * postFilename , struct post * ourPost);

#endif
