#ifndef INDEX_H_INCLUDED
#define INDEX_H_INCLUDED


#include "../../AmmServerlib/AmmServerlib.h"
#include "database.h"

int destroy_index_prototype();

unsigned char * prepare_index_prototype(char * filename , struct website * configuration ,unsigned int pageNumber);

void * prepare_index(struct AmmServer_DynamicRequest  * rqst);


void * rss_callback(struct AmmServer_DynamicRequest  * rqst);
void * page_callback(struct AmmServer_DynamicRequest  * rqst);
void * post_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // INDEX_H_INCLUDED
