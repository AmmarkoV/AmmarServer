#ifndef PAGES_H_INCLUDED
#define PAGES_H_INCLUDED

#include "library.h"
#include "media.h"

void page_login(struct AmmServer_DynamicRequest * rqst,const char * message);
void page_redirect(struct AmmServer_DynamicRequest * rqst,const char * target);
void page_library(struct AmmServer_DynamicRequest * rqst,const char * username,const char * query,unsigned int page);
void page_watch(struct AmmServer_DynamicRequest * rqst,const char * username,const struct Video * video,
                const struct SubtitleTrack * tracks,unsigned int trackCount);
void page_message(struct AmmServer_DynamicRequest * rqst,unsigned int httpCode,const char * message);

//Decodes %XX and '+' in place ( GET values reach callbacks still URL-encoded )
void urlDecodeInPlace(char * text);

#endif // PAGES_H_INCLUDED
