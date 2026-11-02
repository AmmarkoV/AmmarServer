#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "index.h"
#include "database.h"

#define MYBLOG_HTML_INITIAL_CAPACITY 4096


struct AmmServer_MemoryHandler * indexPageWithoutContent=0;
struct AmmServer_MemoryHandler * indexPage=0;


/*Growable HTML buffer used by every index/page generator below.
  The old code added snprintf()'s return value ( the would-be length ) to a
  fixed size and then used totalSize-currentSize as the next size : once a
  fragment was truncated that subtraction underflowed and the following write
  went past the allocation.  Appending through this buffer grows it instead.*/
struct MyBlogHTMLBuffer
{
  char * data;
  size_t length;
  size_t capacity;
  int failed;
};

static int myblog_html_buffer_init(struct MyBlogHTMLBuffer * buffer,size_t initialCapacity)
{
  if (buffer==0) { return 0; }
  buffer->data=0;
  buffer->length=0;
  buffer->capacity=0;
  buffer->failed=0;

  if (initialCapacity<256) { initialCapacity=256; }
  buffer->data=(char*) malloc(initialCapacity);
  if (buffer->data==0) { buffer->failed=1; return 0; }
  buffer->data[0]=0;
  buffer->capacity=initialCapacity;
  return 1;
}

static void myblog_html_buffer_append(struct MyBlogHTMLBuffer * buffer,const char * format,...)
{
  if ( (buffer==0) || (buffer->failed) || (format==0) ) { return; }

  va_list args,argsCopy;
  va_start(args,format);
  va_copy(argsCopy,args);
  int needed=vsnprintf(0,0,format,argsCopy);
  va_end(argsCopy);
  if (needed<0)
   {
     buffer->failed=1;
     va_end(args);
     return;
   }

  size_t required=buffer->length+(size_t)needed+1;
  if (required>buffer->capacity)
   {
     size_t newCapacity=buffer->capacity;
     while (newCapacity<required)
      {
        if (newCapacity>(((size_t)-1)/2)) { newCapacity=required; break; }
        newCapacity*=2;
      }

     char * newData=(char*) realloc(buffer->data,newCapacity);
     if (newData==0)
      {
        buffer->failed=1;
        va_end(args);
        return;
      }
     buffer->data=newData;
     buffer->capacity=newCapacity;
   }

  vsnprintf(buffer->data+buffer->length,(size_t)needed+1,format,args);
  va_end(args);
  buffer->length+=(size_t)needed;
}

/*Hand ownership of the generated string to the caller ( who free()s it ).
  Returns 0 if any append failed.*/
static char * myblog_html_buffer_take(struct MyBlogHTMLBuffer * buffer)
{
  if (buffer==0) { return 0; }

  if (buffer->failed)
   {
     if (buffer->data!=0) { free(buffer->data); }
     buffer->data=0;
     buffer->length=0;
     buffer->capacity=0;
     return 0;
   }

  char * result=buffer->data;
  buffer->data=0;
  buffer->length=0;
  buffer->capacity=0;
  return result;
}


char * getPreviousNextPageHTML(struct website * configuration,unsigned int currentpage)
{
  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  if (currentpage>0)
  {
    myblog_html_buffer_append(&buffer,
      "<div class=\"leftnav\"><a href=\"index.html?page=%u\" >Older Entries</a></div>"
      ,currentpage-1);
  }

  if ( !isThisLastPostPage(configuration,currentpage) )
  {
    myblog_html_buffer_append(&buffer,
      "<div class=\"rightnav\"><a href=\"index.html?page=%u\" >Newer Entries</a></div>"
      ,currentpage+1);
  }

  return myblog_html_buffer_take(&buffer);
}


char * getLeftBlogRollHTML(struct website * configuration)
{
  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->linksLeft.currentItems; i++)
  {
    myblog_html_buffer_append(&buffer,
                              "<li><a href=\"%s\">%s</a></li>\n" ,configuration->linksLeft.item[i].link , configuration->linksLeft.item[i].label);
  }

  return myblog_html_buffer_take(&buffer);
}

char * getRightBlogRollHTML(struct website * configuration )
{
  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->linksRight.currentItems; i++)
  {
    myblog_html_buffer_append(&buffer,
                              "<li><a href=\"%s\">%s</a></li>\n" ,configuration->linksRight.item[i].link , configuration->linksRight.item[i].label);
  }

  return myblog_html_buffer_take(&buffer);
}

char * getFooterLinksHTML(struct website * configuration )
{
  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  myblog_html_buffer_append(&buffer,"<a href=\"index.html\">Home</a> &nbsp;&nbsp;|	&nbsp;&nbsp;\n" );

  unsigned int i=0;
  for (i=0; i<configuration->pages.currentItems; i++)
  {
    myblog_html_buffer_append(&buffer,
                              "<a href=\"page.html?id=%u\" title=\"%s\">%s</a>&nbsp; | &nbsp;&nbsp;\n" ,
                              i ,
                              configuration->pages.item[i].title,
                              configuration->pages.item[i].title
                             );
  }

  myblog_html_buffer_append(&buffer," <a href=\"rss.html\" title=\"RSS\">Posts RSS</a> &nbsp;|&nbsp;\n" );
  myblog_html_buffer_append(&buffer," <a href=\"rssComments.html\" title=\"Comments RSS\">Comments RSS</a> &nbsp;|&nbsp;\n" );

  return myblog_html_buffer_take(&buffer);
}

char * getMenuListHTML(struct website * configuration)
{
  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  unsigned int i=0;
  for (i=0; i<configuration->pages.currentItems; i++)
  {
    myblog_html_buffer_append(&buffer,
                              "<li class=\"page_item page-item-%u\"><a href=\"page.html?id=%u\">%s</a></li>" ,i,
                              i ,
                              configuration->pages.item[i].title
                             );
  }

  myblog_html_buffer_append(&buffer,"<!--getMenuListHTML done-->");

  return myblog_html_buffer_take(&buffer);
}

char * getWidgetListHTML(struct website * configuration)
{
  fprintf(stderr,"widget list consists of %u items \n",configuration->widget.currentItems);

  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  fprintf(stderr," Populating : ");

  unsigned int i=0;
  for (i=0; i<configuration->widget.currentItems; i++)
  {
    myblog_html_buffer_append(&buffer,
                      "\n<li id=\"text-%u\" class=\"widget widget_text\">\
                       \n<h2 class=\"widgettitle\">%s</h2>\
                       \n<div class=\"textwidget\">%s</div>\
		               \n</li>" , i , configuration->widget.item[i].label , configuration->widget.item[i].content.data);

    fprintf(stderr," %zu , ",buffer.length);
  }
  fprintf(stderr," done\n ");

  return myblog_html_buffer_take(&buffer);
}

int appendPage(struct website * configuration , int pageNum , struct MyBlogHTMLBuffer * buffer)
{
 if ( (configuration==0) || (buffer==0) ) { return 0; }
 if ( (pageNum<0) || ((unsigned int)pageNum>=configuration->pages.currentItems) ) { return 0; }

 unsigned int i=(unsigned int) pageNum;
 myblog_html_buffer_append(buffer,"\n<div class=\"post-%u post type-post status-publish format-standard hentry category-post ", i);

     //Print Tag CSS Classes
     unsigned int z=0;
     for (z=0; z<configuration->pages.item[i].tags.currentTags; z++)
     {
       myblog_html_buffer_append(buffer,"tag-%s ",configuration->pages.item[i].tags.item[z].tag );
     }

      myblog_html_buffer_append(buffer,"\" id=\"post-%u\">\n\
	                  <div class=\"posttitle\">\
		                 <h2 class=\"pagetitle\">\
                          <a href=\"post.html?id=%u\" rel=\"bookmark\" title=\"%s\">%s</a></h2>\
		                   <small>Posted: %s by <strong>%s</strong> in <a href=\"post.html?id=%u\" title=\"View all posts in Post\" rel=\"category\">Post</a><br>\
			               " , i , i ,
	   configuration->pages.item[i].title ,
	   configuration->pages.item[i].title ,
	   configuration->pages.item[i].dateStr ,
	   configuration->pages.item[i].author ,
       i );


     myblog_html_buffer_append(buffer,"Tags: ");
     for (z=0; z<configuration->pages.item[i].tags.currentTags; z++)
     {
       myblog_html_buffer_append(buffer,"<a href=\"tag.html?id=%s\" rel=\"tag\">%s</a> ", configuration->pages.item[i].tags.item[z].tag , configuration->pages.item[i].tags.item[z].tag);
     }

    myblog_html_buffer_append(buffer,"</small></div>\
	                  <div class=\"postcomments\"><a href=\"post.html?id=%u#respond\" title=\"Comment on %s..\">0</a></div>\
                      <div class=\"entry\">%s</div>\
	                 \n</div><!-- end of post -->\n"
            , i , configuration->pages.item[i].title  , configuration->pages.item[i].content.data );


 return 1;
}



int appendPost(struct website * configuration , int postNum , struct MyBlogHTMLBuffer * buffer)
{
 if ( (configuration==0) || (buffer==0) ) { return 0; }
 if ( (postNum<0) || ((unsigned int)postNum>=configuration->post.currentPosts) ) { return 0; }

 unsigned int i=(unsigned int) postNum;
 myblog_html_buffer_append(buffer,"\n<div class=\"post-%u post type-post status-publish format-standard hentry category-post ", i);

     //Print Tag CSS Classes
     unsigned int z=0;
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       myblog_html_buffer_append(buffer,"tag-%s ",configuration->post.item[i].tags.item[z].tag );
     }

      myblog_html_buffer_append(buffer,"\" id=\"post-%u\">\n\
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


     myblog_html_buffer_append(buffer,"Tags: ");
     for (z=0; z<configuration->post.item[i].tags.currentTags; z++)
     {
       myblog_html_buffer_append(buffer,"<a href=\"tag.html?id=%s\" rel=\"tag\">%s</a> ", configuration->post.item[i].tags.item[z].tag , configuration->post.item[i].tags.item[z].tag);
     }

    myblog_html_buffer_append(buffer,"</small></div>\
	                  <div class=\"postcomments\"><a href=\"post.html?id=%u#respond\" title=\"Comment on %s..\">0</a></div>\
                      <div class=\"entry\">%s</div>\
	                 \n</div><!-- end of post -->\n"
            , i , configuration->post.item[i].title  , configuration->post.item[i].content.data );


 return 1;
}

char * getPostListHTML(struct website * configuration,int pageNum)
{
  struct MyBlogHTMLBuffer buffer;
  if (!myblog_html_buffer_init(&buffer,MYBLOG_HTML_INITIAL_CAPACITY)) { return 0; }

  if (configuration->post.currentPosts>0)
  {
   unsigned int totalPages = 1 + (unsigned int ) configuration->post.currentPosts/configuration->postsPerPage;
   fprintf(stderr,"\n\n\n\n\n\nPage %u/%u will give us pages from ",pageNum,totalPages);


   if (pageNum>totalPages) { pageNum=totalPages;}

   unsigned int endPost   = pageNum*configuration->postsPerPage;
   if (endPost>=configuration->post.currentPosts) { endPost=configuration->post.currentPosts-1; }
   unsigned int startPost = endPost - configuration->postsPerPage;
   if (startPost>endPost) { startPost=0; }

   fprintf(stderr," %u to %u ( total %u ) \n\n\n\n\n\n",startPost,endPost,configuration->post.currentPosts);


   unsigned int i=0;
   for (i=endPost; i>startPost; i--)
   {
    appendPost(configuration , i , &buffer);
   }
  }

 return myblog_html_buffer_take(&buffer);
}




int strlimcpy(char * output , unsigned int outputLimit , const char * source )
{
  if ( (output==0) || (source==0) || (outputLimit==0) ) { return 0; }

  unsigned int sourceLength = strlen(source);
  if ( sourceLength >= outputLimit )
  {
    fprintf(stderr,"strlimcpy - Source Does not fit in output\n");
    return 0;
  }

  memcpy(output,source,sourceLength);
  output[sourceLength]=0;
  return 1;
}




int setupMyBlog(struct website * configuration)
{
  strlimcpy( configuration->blogTitle , MAX_STR  , "AmmarkoV's Personal Website");
  strlimcpy( configuration->siteName  , MAX_STR  , "AmmarkoV's Website");
  //strlimcpy( configuration->siteDescription  , MAX_STR  , "I would love to change the world , but they won`t give me the source code");
  strlimcpy( configuration->siteDescription  , MAX_STR  , "powered by AmmarServer&trade;");



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

  configuration->pages.currentItems=0;
  loadPages(configuration);

  loadWidgets(configuration);
  loadPosts(configuration);
  return 1;
}


int destroy_index_prototype()
{
  AmmServer_FreeMemoryHandler(&indexPage);
  AmmServer_FreeMemoryHandler(&indexPageWithoutContent);
  return 0;
}

char * prepare_index_prototype(char * filename , struct website * configuration ,unsigned int pageNumber)
{
  fprintf(stderr,"Generating Index File ..!\n");

  fprintf(stderr,"Loading Setup..!\n");
  setupMyBlog(configuration);

  fprintf(stderr,"Reading master index file..!\n");
  indexPage=AmmServer_ReadFileToMemoryHandler(filename);

  fprintf(stderr,"Injecting Menu List..!\n");
  char * htmlData = 0;
  htmlData = getMenuListHTML(configuration);
  AmmServer_ReplaceVariableInMemoryHandler(indexPage,"+++++++++MENULIST+++++++++",htmlData);
  if (htmlData!=0) { free(htmlData); htmlData=0; }


  fprintf(stderr,"Injecting Widget List..!\n");
  htmlData = getWidgetListHTML(configuration);
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



void * rss_callback(struct AmmServer_DynamicRequest  * rqst)
{
 strncpy(rqst->content," ",rqst->MAXcontentSize);

  rqst->contentSize=strlen(rqst->content);
  return 0;
}





//This function prepares the content of  stats context , ( stats.content )
void * prepare_index(struct AmmServer_DynamicRequest  * rqst)
{
  struct AmmServer_MemoryHandler * postListPage=0;

  postListPage = AmmServer_CopyMemoryHandler(indexPageWithoutContent);

  unsigned int havePageToShow=_GETexists(rqst,(char*) "page");
  unsigned int pageToShow=_GETuint(rqst,(char*) "page");
  unsigned int haveLastPage=_GETexists(rqst,(char*) "lastPage");
  unsigned int lastPage=_GETuint(rqst,(char*) "lastPage");

  if (!havePageToShow) { lastPage=1; }
  if (lastPage)
    {
      fprintf(stderr,"Presenting last page.. \n");
      pageToShow = myblog.post.currentPosts/myblog.postsPerPage;
    }


  //unsigned int totalSize=CONTENT_BUFFER,currentSize=0;
  char * buffer = getPostListHTML(&myblog,pageToShow);
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

  unsigned int havePageToShow=_GETexists(rqst,(char*) "id");
  unsigned int pageToShow=_GETuint(rqst,(char*) "id");

  if (havePageToShow)
  {
   postPage = AmmServer_CopyMemoryHandler(indexPageWithoutContent);
   struct MyBlogHTMLBuffer html;
   if ( myblog_html_buffer_init(&html,MYBLOG_HTML_INITIAL_CAPACITY) &&
        appendPost(&myblog , pageToShow , &html) )
    {
      char * buffer = myblog_html_buffer_take(&html);
      if (buffer!=0)
       {
         AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++POSTS+++++++++",buffer);
         free(buffer);
       } else
       {
         char failToFindPost[]=" Could not find post ";
         AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++POSTS+++++++++",failToFindPost);
       }
    } else
    {
      free(myblog_html_buffer_take(&html));
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
   } else
   {
    AmmServer_Warning("Post requested without post-id , serving index page..");
    return prepare_index(rqst);
   }

 return 0;
}


void * page_callback(struct AmmServer_DynamicRequest  * rqst)
{
  struct AmmServer_MemoryHandler * postPage=0;

  unsigned int havePageToShow=_GETexists(rqst,(char*) "id" );
  unsigned int pageToShow=_GETuint(rqst,(char*) "id");


  if (havePageToShow)
  {
   postPage = AmmServer_CopyMemoryHandler(indexPageWithoutContent);
   struct MyBlogHTMLBuffer html;
   if ( myblog_html_buffer_init(&html,MYBLOG_HTML_INITIAL_CAPACITY) &&
        appendPage(&myblog , pageToShow , &html) )
   {
      char * buffer = myblog_html_buffer_take(&html);
      if (buffer!=0)
       {
         AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++POSTS+++++++++",buffer);
         free(buffer);
       } else
       {
         char failToFindPost[]=" Could not find post ";
         AmmServer_ReplaceVariableInMemoryHandler(postPage,"+++++++++POSTS+++++++++",failToFindPost);
       }
   } else
   {
      free(myblog_html_buffer_take(&html));
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
  } else
  {
    AmmServer_Warning("Page requested without page-id , serving index page..");
    return prepare_index(rqst);
  }

  return 0;
}
