#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#define MAX_STR 512
#define MAX_CONTENT 16000


#define MAX_MENU_ITEMS 10
#define MAX_WIDGET_ITEMS 10

#define MAX_TAGS_PER_POST 10
#define MAX_POSTS_IN_DB 1600

#define CONTENT_BUFFER 64000// 64kb



#include "../../AmmServerlib/AmmServerlib.h"


struct htmlContent
{
  unsigned int totalDataLength;
  unsigned int currentDataLength;
  char * data;
};

struct tagItem
{
  char tag[MAX_STR];
  unsigned int tagHash;
};

struct tagItemList
{
  unsigned int currentTags;
  unsigned int maxTags;
  struct tagItem item[MAX_TAGS_PER_POST];
};

struct postItem
{
  char title[MAX_STR];
  char dateStr[MAX_STR];
  char author[MAX_STR];
  struct tagItemList tags;
  struct htmlContent content;
};

struct socialLinks
{
   char facebookURL[MAX_STR];
   char twitterURL[MAX_STR];
   char youtubeURL[MAX_STR];
};


struct linkLabelItem
{
  char label[MAX_STR];
  char link[MAX_STR];
};

struct pageItemList
{
  unsigned int currentItems;
  unsigned int maxItems;
  struct postItem item[MAX_POSTS_IN_DB];
};

struct linkItemList
{
  unsigned int currentItems;
  unsigned int maxItems;
  struct linkLabelItem item[MAX_MENU_ITEMS];
};


struct widgetItem
{
  char label[MAX_STR];
  char link[MAX_STR];
  struct htmlContent content;
};

struct widgetItemList
{
  unsigned int currentItems;
  unsigned int maxItems;
  struct widgetItem item[MAX_WIDGET_ITEMS];
};




struct postItemList
{
  unsigned int currentPosts;
  unsigned int maxPosts;
  struct postItem item[MAX_POSTS_IN_DB];
};


struct website
{
   int allowComments;
   int allowPing;


   char blogTitle[MAX_STR];
   char siteName[MAX_STR];
   char siteDescription[MAX_STR];
   char siteURL[MAX_STR];

   struct socialLinks social;
   struct pageItemList pages;
   struct linkItemList linksLeft;
   struct linkItemList linksRight;
   struct postItemList post;
   unsigned int postsPerPage;

   struct widgetItemList widget;
};

extern struct website myblog;

int isThisLastPostPage(struct website * configuration,unsigned int pageNum);

int loadPosts(struct website * configuration);
int addPost(const char * title , const char * tags , const char * text);

int loadWidgets(struct website * configuration);
int loadPages(struct website * configuration);

#endif // DATABASE_H_INCLUDED
