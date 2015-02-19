#include <stdio.h>

#include "index.h"
#include "database.h"


unsigned int indexLength=0;
unsigned char * indexContent=0;



int appendMenuList(struct website * configuration , char * entryPoint , char * buffer, unsigned int bufferLength , unsigned int totalBufferLength)
{
  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    //toDO Append
    fprintf(stderr,"<li class=\"page_item page-item-%u\"><a href=\"%s\">%s</a></li>" ,i, configuration->menu.item[i].link , configuration->menu.item[i].label);
  }

}

int appendWidgetList(struct website * configuration , char * entryPoint , char * buffer, unsigned int bufferLength , unsigned int totalBufferLength)
{
  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
    fprintf(stderr,"<li id=\"text-%u\" class=\"widget widget_text\">\
                       <h2 class=\"widgettitle\">%s</h2>\
                       <div class=\"textwidget\">%s/div>\
		           </li>" , i , configuration->widget.item[i].label , configuration->widget.item[i].content.totalDataLength);

  }
}

int appendPostList(struct website * configuration , char * entryPoint , char * buffer, unsigned int bufferLength , unsigned int totalBufferLength)
{
  unsigned int i=0;
  for (i=0; i<configuration->menu.currentItems; i++)
  {
     fprintf(stderr,"<div class=\"post-%u post type-post status-publish format-standard hentry category-post ", i);

     //Print Tag CSS Classes
     unsigned int z=0;
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       fprintf(stderr,"tag-%s ",configuration->post.item[i].tags.item[z].tag );
     }

     fprintf(stderr,"\" id=\"post-%u\">\
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


     fprintf(stderr,"Tags: ");
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       fprintf(stderr,"<a href=\"tag.html?id=%s\" rel=\"tag\">%s</a> ", configuration->post.item[i].tags.item[z].tag , configuration->post.item[i].tags.item[z].tag);
     }

    fprintf(stderr,"</small></div>\
	                  <div class=\"postcomments\"><a href=\"post.html?id=%u#respond\" title=\"Comment on %s..\">0</a></div>\
                      <div class=\"entry\">%s</div>\
	                 </div>"
            , i , configuration->post.item[i].title  , configuration->post.item[i].content.data );

  }
}



int appendLeftBlogRoll(struct website * configuration , char * entryPoint , char * buffer, unsigned int bufferLength , unsigned int totalBufferLength)
{
}

int appendRightBlogRoll(struct website * configuration , char * entryPoint , char * buffer, unsigned int bufferLength , unsigned int totalBufferLength)
{
}

int appendFooterLinks(struct website * configuration , char * entryPoint , char * buffer, unsigned int bufferLength , unsigned int totalBufferLength)
{
}


char * prepare_index_prototype(char * filename , struct website * configuration)
{
  indexContent=AmmServer_ReadFileToMemory(filename,&indexLength);
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++YEAR+++","20xx");
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++BLOGTITLE+++",configuration->blogTitle);
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++SITENAME+++",configuration->siteName);
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++SITEDESCRIPTION+++",configuration->siteDescription);
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++RSSLINK+++","rss.html");
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++RSSCOMMENT+++","rssComments.html");

  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++ATOMLINK+++","atom.html");
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++PINGBACKLINK+++","pingback.html");

  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++TAGLIST+++","no tags");
  AmmServer_ReplaceVarInMemoryFile(indexContent,indexLength,"+++ARCHIVELIST+++","no archives");

  appendMenuList(configuration,"+++MENULIST+++",indexContent,indexLength,indexLength);
  appendWidgetList(configuration,"+++WIDGETLIST+++",indexContent,indexLength,indexLength);

  appendLeftBlogRoll(configuration,"+++LEFTBLOGROLL+++",indexContent,indexLength,indexLength);
  appendRightBlogRoll(configuration,"+++RIGHTBLOGROLL+++",indexContent,indexLength,indexLength);
  appendFooterLinks(configuration,"+++FOOTERLINKS+++",indexContent,indexLength,indexLength);

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
