#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "index.h"
#include "database.h"


struct AmmServer_MemoryHandler * indexPageWithoutContent=0;
struct AmmServer_MemoryHandler * indexPage=0;

#warning "Memory Managment in MyBlog while creating a buffer is a bit shabby :P"


unsigned char * getPreviousNextPageHTML(struct website * configuration,unsigned int currentpage)
{
  unsigned int totalSize=CONTENT_BUFFER*5,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }
  buffer[0]=0;


if (currentpage>0)
{
currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
    "<div class=\"rightnav\"><a href=\"index.html?page=%u\" >Newer Entries</a></div>"
	,currentpage-1);
}

if ((unsigned int) configuration->post.currentPosts / configuration->postsPerPage > 0 )
{
  currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
    "<div class=\"leftnav\"><a href=\"index.html?page=%u\" >Older Entries</a></div>"
	,currentpage+1);
}

 return buffer;

}


unsigned char * getLeftBlogRollHTML(struct website * configuration)
{
  unsigned int totalSize=CONTENT_BUFFER*5,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->linksLeft.currentItems; i++)
  {
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
                          "<li><a href=\"%s\">%s</a></li>\n" ,configuration->linksLeft.item[i].link , configuration->linksLeft.item[i].label);
  }

 return buffer;
}

unsigned char * getRightBlogRollHTML(struct website * configuration )
{
  unsigned int totalSize=CONTENT_BUFFER*5,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->linksRight.currentItems; i++)
  {
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
                          "<li><a href=\"%s\">%s</a></li>\n" ,configuration->linksRight.item[i].link , configuration->linksRight.item[i].label);
  }

 return buffer;
}

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
  fprintf(stderr,"widget list consists of %u items \n",configuration->widget.currentItems);

  unsigned int totalSize=(3*CONTENT_BUFFER)*configuration->widget.currentItems,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }

  fprintf(stderr," allocating %u bytes for widgets \n Populating : ",totalSize);

  unsigned int i=0;
  for (i=0; i<configuration->widget.currentItems; i++)
  {
    currentSize+=snprintf(buffer+currentSize,totalSize-currentSize,
                      "\n<li id=\"text-%u\" class=\"widget widget_text\">\
                       \n<h2 class=\"widgettitle\">%s</h2>\
                       \n<div class=\"textwidget\">%s</div>\
		               \n</li>" , i , configuration->widget.item[i].label , configuration->widget.item[i].content.data);

  fprintf(stderr," %u , ",currentSize);
  }
  fprintf(stderr," done\n ",currentSize);

 return buffer;
}





int appendPost(struct website * configuration , int postNum , char * buffer , unsigned int * currentSize , unsigned int totalSize)
{
 unsigned int i= postNum;
 *currentSize+=snprintf(buffer+*currentSize,totalSize-*currentSize,"\n<div class=\"post-%u post type-post status-publish format-standard hentry category-post ", i);

     //Print Tag CSS Classes
     unsigned int z=0;
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       *currentSize+=snprintf(buffer+*currentSize,totalSize-*currentSize,"tag-%s ",configuration->post.item[i].tags.item[z].tag );
     }

      *currentSize+=snprintf(buffer+*currentSize,totalSize-*currentSize,"\" id=\"post-%u\">\n\
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


     *currentSize+=snprintf(buffer+*currentSize,totalSize-*currentSize,"Tags: ");
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       *currentSize+=snprintf(buffer+*currentSize,totalSize-*currentSize,"<a href=\"tag.html?id=%s\" rel=\"tag\">%s</a> ", configuration->post.item[i].tags.item[z].tag , configuration->post.item[i].tags.item[z].tag);
     }

    *currentSize+=snprintf(buffer+*currentSize,totalSize-*currentSize,"</small></div>\
	                  <div class=\"postcomments\"><a href=\"post.html?id=%u#respond\" title=\"Comment on %s..\">0</a></div>\
                      <div class=\"entry\">%s</div>\
	                 \n</div><!-- end of post -->\n"
            , i , configuration->post.item[i].title  , configuration->post.item[i].content.data );



}







unsigned char * getPostListHTML(struct website * configuration,int pageNum)
{
  unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer==0) { fprintf(stderr,"Cannot allocate a big enough buffer for string"); return 0; }


  unsigned int totalPages = (unsigned int ) configuration->post.currentPosts/configuration->postsPerPage;
  if (pageNum>totalPages) { pageNum=totalPages;}

  unsigned int startPost = pageNum*configuration->postsPerPage;
  unsigned int endPost =  startPost+configuration->postsPerPage;
  unsigned int i=0;



  for (i=startPost; i<endPost; i++)
  {
    appendPost(configuration , i , buffer , &currentSize , totalSize);
  }

 return buffer;
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
  //strlimcpy( configuration->siteDescription  , MAX_STR  , "I would love to change the world , but they won`t give me the source code");
  strlimcpy( configuration->siteDescription  , MAX_STR  , "powered by AmmarServer&trade;");


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

   const char * const leftBlogRollList[] = { "Best Links in the world", "bestlinks.html"          ,
                                             "ELLAK Planet"           , "http://planet.ellak.gr/" ,
                                             "FOSS AUEB"              , "http://foss.aueb.gr/" };

  unsigned int i=0;
  configuration->linksLeft.currentItems=0;
  for (i=0; i<3; i++)
  {
      snprintf(configuration->linksLeft.item[i].label, MAX_STR , "%s", leftBlogRollList[i*2+0] );
      snprintf(configuration->linksLeft.item[i].link , MAX_STR , "%s", leftBlogRollList[i*2+1] );
      ++configuration->linksLeft.currentItems;
  }

   const char * const rightBlogRollList[] = { "Free Software Foundation", "http://www.fsf.org/"  ,
                                              "Guarddog project blog"   , "+++++++++WEBROOT+++++++++gddg.html"   };

  configuration->linksRight.currentItems=0;
  for (i=0; i<2; i++)
  {
      snprintf(configuration->linksRight.item[i].label, MAX_STR , "%s", rightBlogRollList[i*2+0] );
      snprintf(configuration->linksRight.item[i].link , MAX_STR , "%s", rightBlogRollList[i*2+1] );
      ++configuration->linksRight.currentItems;
  }


  configuration->postsPerPage=5;

  loadWidgets(configuration);
  loadPosts(configuration);
}


int destroy_index_prototype()
{
  AmmServer_FreeMemoryHandler(&indexPage);
  AmmServer_FreeMemoryHandler(&indexPageWithoutContent);
  return 0;
}

unsigned char * prepare_index_prototype(char * filename , struct website * configuration ,unsigned int pageNumber)
{
  fprintf(stderr,"Generating Index File ..!\n");

  fprintf(stderr,"Loading Setup..!\n");
  setupMyBlog(configuration);

  fprintf(stderr,"Reading master index file..!\n");
  indexPage=AmmServer_ReadFileToMemoryHandler(filename);

  fprintf(stderr,"Injecting Menu List..!\n");
  unsigned char * htmlData = 0;
  htmlData = getMenuListHTML(configuration);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++MENULIST+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }


  fprintf(stderr,"Injecting Widget List..!\n");
  htmlData = getWidgetListHTML(configuration);
  //This segfaults
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++WIDGETLIST+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Blog Roll Left..!\n");
  htmlData = getLeftBlogRollHTML(configuration);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++LEFTBLOGROLL+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Blog Roll Right..!\n");
  htmlData = getRightBlogRollHTML(configuration);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++RIGHTBLOGROLL+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  fprintf(stderr,"Injecting Footer List ..!\n");
  htmlData = getFooterLinksHTML(configuration);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++FOOTERLINKS+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }


  fprintf(stderr,"Replacing Variables..!\n");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++YEAR+++++++++","20xx");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++BLOGTITLE+++++++++",configuration->blogTitle);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,6,"+++++++++SITENAME+++++++++",configuration->siteName);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++SITEDESCRIPTION+++++++++",configuration->siteDescription);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,4,"+++++++++RSSLINK+++++++++","rss.html");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,2,"+++++++++RSSCOMMENT+++++++++","rssComments.html");

  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++ATOMLINK+++++++++","atom.html");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++PINGBACKLINK+++++++++","pingback.html");

  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++TAGLIST+++++++++","<li>no tags</li>");
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"+++++++++ARCHIVELIST+++++++++","<li>no archives</li>");


  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,4,"+++RESOURCES+++","res");

  indexPageWithoutContent = AmmServer_CopyMemoryHandler(indexPage);

  fprintf(stderr,"Injecting Post List ..!\n");
  htmlData = getPostListHTML(configuration,pageNumber);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++POSTS+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }

  htmlData = getPreviousNextPageHTML(configuration,pageNumber);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++PREVNEXT+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }



  fprintf(stderr,"Done with index..\n");

  return indexPage->content;
}



void * menu_callback(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><head><title>Not yet ready</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>Not yet ready</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_index(struct AmmServer_DynamicRequest  * rqst)
{
  struct AmmServer_MemoryHandler * postListPage=0;

  postListPage = AmmServer_CopyMemoryHandler(indexPageWithoutContent);

  unsigned int pageToShow=_GETuint(rqst->instance,rqst,(char*) "page");


  unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  unsigned char * buffer = getPostListHTML(&myblog,pageToShow);
  if (buffer!=0)
  {
      AmmServer_ReplaceVariableInMemoryHandler(postListPage,"+++++++++POSTS+++++++++",buffer);
      free(buffer);
  } else
  {
      char failToFindPost[]=" Could not find posts ";
      AmmServer_ReplaceVariableInMemoryHandler(postListPage,"+++++++++POSTS+++++++++",failToFindPost);
  }

  buffer = getPreviousNextPageHTML(&myblog,pageToShow);
  AmmServer_ReplaceVariableInMemoryHandler(postListPage,"+++++++++PREVNEXT+++++++++",buffer);
  if (buffer!=0) { free(buffer); buffer=0; }


  unsigned int howLongToCopy = postListPage->contentCurrentLength;
  if ( howLongToCopy > rqst->MAXcontentSize ) { howLongToCopy=rqst->MAXcontentSize; }
  strncpy(rqst->content,postListPage->content,howLongToCopy);
  rqst->contentSize=howLongToCopy;

  AmmServer_FreeMemoryHandler(&postListPage);


  return 0;
}


void * post_callback(struct AmmServer_DynamicRequest  * rqst)
{
  struct AmmServer_MemoryHandler * postPage=0;

  postPage = AmmServer_CopyMemoryHandler(indexPageWithoutContent);

  unsigned int pageToShow=_GETuint(rqst->instance,rqst,(char*) "id");


  unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  unsigned char * buffer = (unsigned char*) malloc (sizeof(unsigned char) * totalSize );
  if (buffer!=0)
  {
      appendPost(&myblog , pageToShow , buffer , &currentSize , totalSize);
      AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++POSTS+++++++++",buffer);
      free(buffer);
  } else
  {
      char failToFindPost[]=" Could not find post ";
      AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++POSTS+++++++++",failToFindPost);
  }

  char backButton[]=" <div class=\"leftnav\"><a href=\"javascript: history.go(-1)\" >Back to previous page</a></div> ";
  AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++PREVNEXT+++++++++",backButton);

  unsigned int howLongToCopy = postPage->contentCurrentLength;
  if ( howLongToCopy > rqst->MAXcontentSize ) { howLongToCopy=rqst->MAXcontentSize; }
  strncpy(rqst->content,postPage->content,howLongToCopy);
  rqst->contentSize=howLongToCopy;

  AmmServer_FreeMemoryHandler(&postPage);
  return 0;
}
