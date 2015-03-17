#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "database.h"


struct AmmServer_MemoryHandler * indexPage=0;

#warning "Memory Managment in MyBlog while creating a buffer is a bit shabby :P"


unsigned char * getFooterLinksHTML(struct website * configuration )
{
  unsigned int totalSize=CONTENT_BUFFER*5,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"<a href=\"index.html\">Home</a> &nbsp;&nbsp;|	&nbsp;&nbsp;\n" );

  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    //TODO PROPER MEMORY HANDLING ,, REALLOC ETC ..
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
                          "<a href=\"%s\" title=\"%s\">%s</a>&nbsp; | &nbsp;&nbsp;\n" ,configuration->menu.item[i].link , configuration->menu.item[i].label, configuration->menu.item[i].label);
  }

 currentSize+=snprintf(buffer+currentSize,totalSize-currentSize," <a href=\"rss.html\" title=\"RSS\">Posts RSS</a> &nbsp;|&nbsp;\n" );
 currentSize+=snprintf(buffer+currentSize,totalSize-currentSize," <a href=\"rssComments.html\" title=\"Comments RSS\">Comments RSS</a> &nbsp;|&nbsp;\n" );

 return buffer;
}

unsigned char * getMenuListHTML(struct website * configuration)
{
  unsigned int totalSize=CONTENT_BUFFER*5,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    //TODO PROPER MEMORY HANDLING ,, REALLOC ETC ..
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"<li class=\"page_item page-item-%u\"><a href=\"%s\">%s</a></li>" ,i, configuration->menu.item[i].link , configuration->menu.item[i].label);
  }

 currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"<!--getMenuListHTML done-->");

 return buffer;
}

unsigned char * getWidgetListHTML(struct website * configuration)
{
  unsigned int totalSize=CONTENT_BUFFER*5,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
                      "<li id=\"text-%u\" class=\"widget widget_text\">\
                       <h2 class=\"widgettitle\">%s</h2>\
                       <div class=\"textwidget\">%s</div>\
		               </li>" , i , configuration->widget.item[i].label , configuration->widget.item[i].content.data);

  }

 return buffer;
}

unsigned char * getPostListHTML(struct website * configuration)
{
  unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
     currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"<div class=\"post-%u post type-post status-publish format-standard hentry category-post ", i);

     //Print Tag CSS Classes
     unsigned int z=0;
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"tag-%s ",configuration->post.item[i].tags.item[z].tag );
     }

      currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"\" id=\"post-%u\">\
	                  <div class=\"posttitle\">\
		                 <h2 class=\"pagetitle\">\
                          <a href=\"post.html?id=%u\" rel=\"bookmark\" title=\"%s\">%s</a></h2>\
		                   <small>Posted: %s by <strong>%s</strong> in <a href=\"post.html?id=%u\" title=\"View all posts in Post\" rel=\"category\">Post</a><br>\
			               " , i , i ,
	   configuration->post.item[i].title ,
	   configuration->post.item[i].title ,
	   configuration->post.item[i].dateStr ,
	   configuration->post.item[i].author ,
       i );


     currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"Tags: ");
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"<a href=\"tag.html?id=%s\" rel=\"tag\">%s</a> ", configuration->post.item[i].tags.item[z].tag , configuration->post.item[i].tags.item[z].tag);
     }

    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"</small></div>\
	                  <div class=\"postcomments\"><a href=\"post.html?id=%u#respond\" title=\"Comment on %s..\">0</a></div>\
                      <div class=\"entry\">%s</div>\
	                 </div>"
            , i , configuration->post.item[i].title  , configuration->post.item[i].content.data );

  }

 return buffer;
}



unsigned char * getLeftBlogRollHTML(struct website * configuration)
{
    return 0;
}

unsigned char * getRightBlogRollHTML(struct website * configuration )
{
    return 0;
}


int strlimcpy(char * output , unsigned int outputLimit , const char * source )
{
  unsigned int sourceLength = strlen(source);
  if ( sourceLength >= outputLimit )
  {
    fprintf(stderr,"strlimcpy - Source Does not fit in output\n");
    return 0;
  }

  if ( sourceLength < outputLimit )
  {
    strncpy(output,source,sourceLength);
    return 1;
  }
 return 0;
}

int setupMyBlog(struct website * configuration)
{
  strlimcpy( configuration->blogTitle , MAX_STR  , "AmmarkoV's Personal Website");
  strlimcpy( configuration->siteName  , MAX_STR  , "AmmarkoV's Website");
  strlimcpy( configuration->siteDescription  , MAX_STR  , "I would love to change the world , but they won`t give me the source code");





  //HARDCODED MENUS
  configuration->menu.currentItems=0;

  snprintf( configuration->menu.item[configuration->menu.currentItems].link , MAX_STR , "menu%u.html" , configuration->menu.currentItems );
  strncpy( configuration->menu.item[configuration->menu.currentItems].label, "About" , MAX_STR);
  ++configuration->menu.currentItems;
  //-------------------------------
  snprintf( configuration->menu.item[configuration->menu.currentItems].link , MAX_STR , "menu%u.html" , configuration->menu.currentItems );
  strncpy( configuration->menu.item[configuration->menu.currentItems].label, "Linux Coding" , MAX_STR);
  ++configuration->menu.currentItems;
  //-------------------------------
  snprintf( configuration->menu.item[configuration->menu.currentItems].link , MAX_STR , "menu%u.html" , configuration->menu.currentItems );
  strncpy( configuration->menu.item[configuration->menu.currentItems].label, "Windows Coding" , MAX_STR);
  ++configuration->menu.currentItems;
  //-------------------------------
  snprintf( configuration->menu.item[configuration->menu.currentItems].link , MAX_STR , "menu%u.html" , configuration->menu.currentItems );
  strncpy( configuration->menu.item[configuration->menu.currentItems].label, "GuarddoG Robot Project" , MAX_STR);
  ++configuration->menu.currentItems;
  //-------------------------------
  snprintf( configuration->menu.item[configuration->menu.currentItems].link , MAX_STR , "menu%u.html" , configuration->menu.currentItems );
  strncpy( configuration->menu.item[configuration->menu.currentItems].label, "DeviantArt Gallery" , MAX_STR);
  ++configuration->menu.currentItems;
  //-------------------------------


  const char * const widgetLabelList[] = { "Donation box!", "Featured Projects", "Project Statistics", "Browser Detector :P" };
  char tmpPath[512]={0};
  struct AmmServer_MemoryHandler *  tmp=0;
  configuration->widget.currentItems=0;

  unsigned int loadedWidgets=0;
  for (loadedWidgets=0; loadedWidgets<4; loadedWidgets++)
  {
   //-------------------------------
   snprintf(tmpPath,512,"src/Services/MyBlog/res/widgets/widget%u.html",loadedWidgets);
   tmp = AmmServer_ReadFileToMemoryHandler(tmpPath);
   if (tmp!=0)
   {
    snprintf(configuration->widget.item[configuration->widget.currentItems].label , MAX_STR , "%s", widgetLabelList[loadedWidgets] );
    snprintf(configuration->widget.item[configuration->widget.currentItems].link , MAX_STR , "widget%u.html",loadedWidgets );
    configuration->widget.item[configuration->widget.currentItems].content.data=tmp->content;
    configuration->widget.item[configuration->widget.currentItems].content.totalDataLength = tmp->contentSize;
    configuration->widget.item[configuration->widget.currentItems].content.currentDataLength  = tmp->contentCurrentLength;
    free(tmp); //We just free the wrapper
    ++configuration->widget.currentItems;
   }
  //-------------------------------
  }
}


int destroy_index_prototype()
{
  AmmServer_FreeMemoryHandler(&indexPage);
  return 0;
}

unsigned char * prepare_index_prototype(char * filename , struct website * configuration)
{
  fprintf(stderr,"Generating Index File ..!\n");

  fprintf(stderr,"Loading Setup..!\n");
  setupMyBlog(configuration);

  fprintf(stderr,"Reading master index file..!\n");
  indexPage=AmmServer_ReadFileToMemoryHandler(filename);

  fprintf(stderr,"Replacing Variables..!\n");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++YEAR+++++++++","20xx");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++BLOGTITLE+++++++++",configuration->blogTitle);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,6,"+++++++++SITENAME+++++++++",configuration->siteName);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++SITEDESCRIPTION+++++++++",configuration->siteDescription);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,4,"+++++++++RSSLINK+++++++++","rss.html");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,2,"+++++++++RSSCOMMENT+++++++++","rssComments.html");

  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++ATOMLINK+++++++++","atom.html");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++PINGBACKLINK+++++++++","pingback.html");

  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++TAGLIST+++++++++","no tags");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++ARCHIVELIST+++++++++","no archives");

  fprintf(stderr,"Injecting Menu List..!\n");
  unsigned char * htmlData = 0;
  htmlData = getMenuListHTML(configuration);
  AmmServer_InjectDataToBuffer("+++++++++MENULIST+++++++++",htmlData,indexPage);
  if (htmlData!=0) { free(htmlData); htmlData=0; }


  fprintf(stderr,"Injecting Widget List..!\n");
  htmlData = getWidgetListHTML(configuration);
  AmmServer_InjectDataToBuffer("+++++++++WIDGETLIST+++++++++",htmlData,indexPage);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Blog Roll Left..!\n");
  htmlData = getLeftBlogRollHTML(configuration);
  AmmServer_InjectDataToBuffer("+++++++++LEFTBLOGROLL+++++++++",htmlData,indexPage);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Blog Roll Right..!\n");
  htmlData = getRightBlogRollHTML(configuration);
  AmmServer_InjectDataToBuffer("+++++++++RIGHTBLOGROLL+++++++++",htmlData,indexPage);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Footer List ..!\n");
  htmlData = getFooterLinksHTML(configuration);
  AmmServer_InjectDataToBuffer("+++++++++FOOTERLINKS+++++++++",htmlData,indexPage);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Done with index..\n");

  return indexPage->content;
}



//This function prepares the content of  stats context , ( stats.content )
void * prepare_index(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int howLongToCopy = indexPage->contentCurrentLength;
  if ( howLongToCopy > rqst->MAXcontentSize ) { howLongToCopy=rqst->MAXcontentSize; }
  strncpy(rqst->content,indexPage->content,howLongToCopy);
  rqst->contentSize=howLongToCopy;
  return 0;
}
