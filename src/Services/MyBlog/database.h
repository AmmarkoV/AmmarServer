#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#define MAX_STR 512
#define MAX_CONTENT 16000


#define MAX_MENU_ITEMS 10
#define MAX_WIDGET_ITEMS 10

#define MAX_TAGS_PER_POST 10

struct htmlContent
{
  unsigned int totalDataLength;
  unsigned int currentDataLength;
  unsigned char * data;
};


struct socialLinks
{
   char facebookURL[MAX_STR];
   char twitterURL[MAX_STR];
   char youtubeURL[MAX_STR];
};


struct linkLabelItem
{
  unsigned char label[MAX_STR];
  unsigned char link[MAX_STR];
};

struct menuItemList
{
  unsigned int currentItems;
  unsigned int maxItems;
  struct linkLabelItem item[MAX_MENU_ITEMS];
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
  unsigned char title[MAX_STR];
  unsigned char dateStr[MAX_STR];
  unsigned char author[MAX_STR];
  struct tagItemList tags;
  struct htmlContent content;
};


struct postItemList
{
  unsigned int currentPosts;
  unsigned int maxPosts;
  struct postItem item[MAX_TAGS_PER_POST];
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
   struct menuItemList menu;
   struct linkItemList links;
   struct postItemList post;

   struct widgetItemList widget;
};

extern struct website myblog;



#endif // DATABASE_H_INCLUDED
