#include "editor.h"

char editorBody[]= {
#include "editor.html"
};

void * editor_callback(struct AmmServer_DynamicRequest  * rqst)
{
 snprintf(rqst->content,rqst->MAXcontentSize,"%s",editorBody);
 rqst->contentSize=strlen(rqst->content);
 return 0;
}
