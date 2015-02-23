#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "database.h"


unsigned int indexLength=0;
unsigned char * indexContent=0;

#warning "Memory Managment in MyBlog while creating a buffer is a bit shabby :P"


unsigned char * getMenuListHTML(struct website * configuration)
{
  unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    //TODO PROPER MEMORY HANDLING ,, REALLOC ETC ..
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,"<li class=\"page_item page-item-%u\"><a href=\"%s\">%s</a></li>" ,i, configuration->menu.item[i].link , configuration->menu.item[i].label);
  }

 return buffer;
}

unsigned char * getWidgetListHTML(struct website * configuration)
{
  unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
                      "<li id=\"text-%u\" class=\"widget widget_text\">\
                       <h2 class=\"widgettitle\">%s</h2>\
                       <div class=\"textwidget\">%s/div>\
		           </li>" , i , configuration->widget.item[i].label , configuration->widget.item[i].content.totalDataLength);

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

unsigned char * getFooterLinksHTML(struct website * configuration )
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
  strlimcpy( configuration->blogTitle , MAX_STR , "AmmarkoV's Personal Website");

}



unsigned char * prepare_index_prototype(char * filename , struct website * configuration)
{
  fprintf(stderr,"Generating Index File ..!\n");

  fprintf(stderr,"Loading Setup..!\n");
  setupMyBlog(configuration);

  fprintf(stderr,"Reading master index file..!\n");
  indexContent=AmmServer_ReadFileToMemory(filename,&indexLength);

  fprintf(stderr,"Replacing Variables..!\n");
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++YEAR+++","20xx");
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++BLOGTITLE+++",configuration->blogTitle);
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,6,indexLength,"+++SITENAME+++",configuration->siteName);
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++SITEDESCRIPTION+++",configuration->siteDescription);
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,4,indexLength,"+++RSSLINK+++","rss.html");
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,2,indexLength,"+++RSSCOMMENT+++","rssComments.html");

  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++ATOMLINK+++","atom.html");
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++PINGBACKLINK+++","pingback.html");

  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++TAGLIST+++","no tags");
  AmmServer_ReplaceAllVarsInMemoryFile(indexContent,1,indexLength,"+++ARCHIVELIST+++","no archives");

  fprintf(stderr,"Injecting Menu List..!\n");
  unsigned char * htmlData = 0;
  htmlData = getMenuListHTML(configuration);
  AmmServer_InjectDataToBuffer("+++MENULIST+++",htmlData,indexContent,indexLength,indexLength);
  if (htmlData!=0) { free(htmlData); htmlData=0; }


  fprintf(stderr,"Injecting Widget List..!\n");
  htmlData = getWidgetListHTML(configuration);
  AmmServer_InjectDataToBuffer("+++WIDGETLIST+++",htmlData,indexContent,indexLength,indexLength);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Blog Roll Left..!\n");
  htmlData = getLeftBlogRollHTML(configuration);
  AmmServer_InjectDataToBuffer("+++LEFTBLOGROLL+++",htmlData,indexContent,indexLength,indexLength);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Blog Roll Right..!\n");
  htmlData = getRightBlogRollHTML(configuration);
  AmmServer_InjectDataToBuffer("+++RIGHTBLOGROLL+++",htmlData,indexContent,indexLength,indexLength);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Footer List ..!\n");
  htmlData = getFooterLinksHTML(configuration);
  AmmServer_InjectDataToBuffer("+++FOOTERLINKS+++",htmlData,indexContent,indexLength,indexLength);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  return indexContent;
}



//This function prepares the content of  stats context , ( stats.content )
void * prepare_index(struct AmmServer_DynamicRequest  * rqst)
{

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  /*
  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><title>Dynamic Content Enabled</title><meta http-equiv=\"refresh\" content=\"1\"></head>\
            <body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>\
            The string you see is updated dynamically every time you get a fresh copy of this file!<br><br>\n\
            To include your own content see the <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/main.c#L46\">\
            Dynamic content code label in ammarserver main.c</a><br>\n\
            If you dont need dynamic content at all consider disabling it from ammServ.conf or by setting DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=0; in \
            <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/file_caching.c\">file_caching.c</a> and recompiling.!</body></html>",
           tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);*/

  rqst->contentSize=strlen(rqst->content);
  return 0;
}
