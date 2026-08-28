#ifndef PAGE_H_INCLUDED
#define PAGE_H_INCLUDED

#include "state.h"

//No token at all : "make a new list" landing page
void * renderLandingPage(struct AmmServer_DynamicRequest * rqst);

//?new=1 : mint a fresh token and bounce the browser to it ( meta-refresh , this codebase has no real 302 from a dynamic callback )
void * renderNewCartRedirect(struct AmmServer_DynamicRequest * rqst);

//?i=TOKEN : the mobile cart UI itself , with CART already loaded ( see main.c )
void * renderCartPage(struct AmmServer_DynamicRequest * rqst,const char * token,struct cart * cart);

#endif // PAGE_H_INCLUDED
