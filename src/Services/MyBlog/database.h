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
  unsigned char * data;
};

struct tagItem
{
  unsigned char tag[MAX_STR];
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
  unsigned char title[MAX_STR];
  unsigned char dateStr[MAX_STR];
  unsigned char author[MAX_STR];
  struct tagItemList tags;
  struct htmlContent content;
};

struct socialLinks
{
   unsigned char facebookURL[MAX_STR];
   unsigned char twitterURL[MAX_STR];
   unsigned char youtubeURL[MAX_STR];
};


struct linkLabelItem
{
  unsigned char label[MAX_STR];
  unsigned char link[MAX_STR];
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
  unsigned char label[MAX_STR];
  unsigned char link[MAX_STR];
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


   unsigned char blogTitle[MAX_STR];
   unsigned char siteName[MAX_STR];
   unsigned char siteDescription[MAX_STR];
   unsigned char siteURL[MAX_STR];

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
int loadWidgets(struct website * configuration);
int loadPages(struct website * configuration);

#endif // DATABASE_H_INCLUDED
