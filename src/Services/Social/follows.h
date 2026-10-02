#ifndef FOLLOWS_H_INCLUDED
#define FOLLOWS_H_INCLUDED

#include "session.h"

/*One flat table of "who follows whom" , sized the same way the post store is - a fixed allocation with no
  reallocation to get wrong..*/
#define MAX_FOLLOWS 4096

int loadFollows(const char * filename);
int unloadFollows();

int followsIsFollowing(const char * follower,const char * followee);

/*Follows if this pair isn't following yet , unfollows if it is - one endpoint for one button , same as a like.
  @retval 1=Changed , 0=Failed ( table full / bad arguments )*/
int followsToggle(const char * follower,const char * followee);

unsigned int followsCountFollowing(const char * username);
unsigned int followsCountFollowers(const char * username);

void * follow_callback(struct AmmServer_DynamicRequest  * rqst);
void * people_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // FOLLOWS_H_INCLUDED
