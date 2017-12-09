#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "editor.h"

char editorBody[]= {
#include "include/editor.html"
};

void * editor_callback(struct AmmServer_DynamicRequest  * rqst)
{
 snprintf(rqst->content,rqst->MAXcontentSize,"%s",editorBody);
 rqst->contentSize=strlen(rqst->content);
 return 0;
}
