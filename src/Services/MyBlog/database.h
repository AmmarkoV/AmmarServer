#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#define MAX_STR 512
#define MAX_CONTENT 16000


#define MAX_MENU_ITEMS 10
#define MAX_WIDGET_ITEMS 10

struct htmlContent
{
  unsigned int allocatedContent;
  unsigned int currentContent;
  unsigned char * content;
};



struct menuItem
{
  unsigned char menuLabel[MAX_STR];
  unsigned char menuLink[MAX_STR];
};

struct menuItemList
{
  unsigned int currentItems;
  unsigned int maxItems;
  struct menuItem item[MAX_MENU_ITEMS];
};


struct widgetItem
{
  unsigned char widgetLabel[MAX_STR];
  unsigned char widgetLink[MAX_STR];
  struct htmlContent widgetContent;
};

struct widgetItemList
{
  unsigned int currentItems;
  unsigned int maxItems;
  struct widgetItem item[MAX_WIDGET_ITEMS];
};






struct website
{
   char blogTitle[MAX_STR];
   char siteName[MAX_STR];
   char siteURL[MAX_STR];

   struct menuItemList menu;
   struct widgetItemList widget;
};


#endif // DATABASE_H_INCLUDED
