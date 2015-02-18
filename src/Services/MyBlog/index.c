#include "index.h"
#include "database.h"


unsigned int indexLength=0;
unsigned char * indexContent=0;


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
