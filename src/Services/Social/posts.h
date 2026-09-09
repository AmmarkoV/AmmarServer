#ifndef POSTS_H_INCLUDED
#define POSTS_H_INCLUDED

#include "session.h"
#include "media.h"

/*Fixed capacities keep the whole store one flat allocation with no reallocation to get wrong - the wall drops
  its oldest post once it is full , the same way the chat log just keeps growing..*/
#define MAX_POSTS 256
#define MAX_COMMENTS_PER_POST 16
#define MAX_LIKES_PER_POST 64
#define MAX_POST_TEXT 281 /*280 characters + null termination*/

struct socialComment
{
  char author[MAX_USERNAME];
  unsigned long timestamp;
  char text[MAX_POST_TEXT];
};

struct socialPost
{
  unsigned int id;
  char author[MAX_USERNAME];
  /*Whose wall this was written on - the author's own , unless somebody wrote on somebody else's..*/
  char wall[MAX_USERNAME];
  unsigned long timestamp;
  char text[MAX_POST_TEXT];
  /*The stored name of a picture or sound attached to this post , empty when there isn't one..*/
  char media[MAX_MEDIA_NAME];

  unsigned int numberOfComments;
  struct socialComment comments[MAX_COMMENTS_PER_POST];

  unsigned int numberOfLikes;
  char likedBy[MAX_LIKES_PER_POST][MAX_USERNAME];
};

int loadPosts(const char * filename);
int unloadPosts();

int addPost(const char * author,const char * wall,const char * text,const char * media);
int addComment(unsigned int postID,const char * author,const char * text);

/*Likes the post if this user hasn't liked it yet , unlikes it if they have - one endpoint for one button..*/
int toggleLike(unsigned int postID,const char * username);

/*Renders the newest posts as HTML into buffer , newest first. `onlyOnWall` limits it to the posts written on
  one user's wall ( their profile ) or is 0 for every wall ( the home feed ). `viewer` is who is looking , so
  their own likes can be marked , and csrfToken/backUser are what the like/comment forms carry back.
  @retval Number of posts rendered*/
unsigned int renderPosts(
                          char * buffer,
                          unsigned int bufferSize,
                          const char * onlyOnWall,
                          const char * viewer,
                          const char * csrfToken,
                          const char * backUser
                        );

#endif // POSTS_H_INCLUDED
