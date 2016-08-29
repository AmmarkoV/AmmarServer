#include "database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../AmmServerlib/InputParser/InputParser_C.h"

struct website myblog={0};

int isThisLastPostPage(struct website * configuration,unsigned int pageNum)
{
  unsigned int totalPages = (unsigned int ) configuration->post.currentPosts/configuration->postsPerPage;
  if (pageNum>=totalPages) { return 1;}
  return 0;
}

int loadPostInfo(struct website * configuration,unsigned int postNum)
{
 char filename[FILENAME_MAX]={0};
 snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/posts/info%u.html",postNum);
 fprintf(stderr," Loading post info %u (%s) .. \n",postNum,filename);

 ssize_t read;


 FILE * fp = fopen(filename,"r");
 if (fp!=0)
  {
   struct InputParserC * ipc = InputParser_Create(512,4);
   InputParser_SetDelimeter(ipc,1,'(');
   InputParser_SetDelimeter(ipc,2,',');
   InputParser_SetDelimeter(ipc,3,')');

   struct tagItemList tags;
   char * line = NULL;
   size_t len = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
       //printf("Retrieved line of length %zu :\n", read);
       //printf("%s", line);
       InputParser_SeperateWords(ipc,line,0);

       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"TITLE")  )   { InputParser_GetWord(ipc,1,configuration->post.item[postNum].title  ,MAX_STR);   } else
       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"DATE")   )   { InputParser_GetWord(ipc,1,configuration->post.item[postNum].dateStr,MAX_STR);   } else
       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"AUTHOR") )   { InputParser_GetWord(ipc,1,configuration->post.item[postNum].author ,MAX_STR);   }
       //if ( InputParser_WordCompareNoCaseAuto(ipc,0,"TAGS") )     { InputParser_GetWord(ipc,0,configuration->post.item[postNum].tags ,MAX_STR); } else
       //if ( InputParser_WordCompareNoCaseAuto(ipc,0,"COMMENTS") ) { InputParser_GetWord(ipc,0,line,MAX_STR); }
    }

    fprintf(stderr," Post Info %u --------------\n",postNum);
    fprintf(stderr,"   Title : %s \n",configuration->post.item[postNum].title);
    fprintf(stderr,"   Date : %s \n",configuration->post.item[postNum].dateStr);
    fprintf(stderr,"   Author : %s \n",configuration->post.item[postNum].author);

    fclose(fp);
    if (line) { free(line); }

    InputParser_Destroy(ipc);
    return 1;
  }
 return 0;
}


int loadPosts(struct website * configuration)
{
  fprintf(stderr," Loading posts .. \n");

  configuration->post.currentPosts=0;

  char filename[FILENAME_MAX]={0};
  snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/posts/post%u.html",configuration->post.currentPosts);
  while (AmmServer_FileExists(filename))
  {

   struct AmmServer_MemoryHandler *  tmp = AmmServer_ReadFileToMemoryHandler(filename);
   if (tmp!=0)
   {
    fprintf(stderr," Loading post %u (%s) .. \n",configuration->post.currentPosts,filename);
    configuration->post.item[configuration->post.currentPosts].content.data = tmp->content;
    //If we didnt use this we shold -> AmmServer_FreeMemoryHandler(&tmp); tmp->content=0;

    loadPostInfo(configuration,configuration->post.currentPosts);
    //-------------
    ++configuration->post.currentPosts;
    snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/posts/post%u.html",configuration->post.currentPosts);
   } else
   {
     break;
   }

  }
  return 1;
}








int loadWidgetInfo(struct website * configuration,unsigned int postNum)
{
 char filename[FILENAME_MAX]={0};
 snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/widgets/info%u.html",postNum);
 fprintf(stderr," Loading widget info %u (%s) .. \n",postNum,filename);

 ssize_t read;
 FILE * fp = fopen(filename,"r");
 if (fp!=0)
  {
   struct InputParserC * ipc = InputParser_Create(512,4);
   InputParser_SetDelimeter(ipc,1,'(');
   InputParser_SetDelimeter(ipc,2,',');
   InputParser_SetDelimeter(ipc,3,')');

   struct tagItemList tags;
   char * line = NULL;
   size_t len = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
       InputParser_SeperateWords(ipc,line,0);

       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"TITLE")  )   { InputParser_GetWord(ipc,1,configuration->widget.item[postNum].label  ,MAX_STR);   }
    }

    fprintf(stderr," Widget Title %u --------------\n",postNum);
    fprintf(stderr,"   Title : %s \n",configuration->widget.item[postNum].label);

    fclose(fp);
    if (line) { free(line); }

    InputParser_Destroy(ipc);
    return 1;
  }
 return 0;
}



int loadWidgets(struct website * configuration)
{
  fprintf(stderr," Loading widgets .. \n");

  char tmpPath[512]={0};
  struct AmmServer_MemoryHandler *  tmp=0;
  configuration->widget.currentItems=0;

  unsigned int loadedWidgets=0;
  for (loadedWidgets=0; loadedWidgets<3; loadedWidgets++)
  {
   //-------------------------------
   snprintf(tmpPath,512,"src/Services/MyBlog/res/widgets/widget%u.html",loadedWidgets);
   tmp = AmmServer_ReadFileToMemoryHandler(tmpPath);
   if (tmp!=0)
   {
    snprintf(configuration->widget.item[configuration->widget.currentItems].link , MAX_STR , "widget%u.html",loadedWidgets );
    configuration->widget.item[configuration->widget.currentItems].content.data=tmp->content;
    configuration->widget.item[configuration->widget.currentItems].content.totalDataLength = tmp->contentSize;
    configuration->widget.item[configuration->widget.currentItems].content.currentDataLength  = tmp->contentCurrentLength;

    loadWidgetInfo(configuration,configuration->widget.currentItems);
    ++configuration->widget.currentItems;
   }
  //-------------------------------
  }

  return 0;
}

















int loadPageInfo(struct website * configuration,unsigned int postNum)
{
 char filename[FILENAME_MAX]={0};
 snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/pages/info%u.html",postNum);
 fprintf(stderr," Loading widget info %u (%s) .. \n",postNum,filename);

 ssize_t read;
 FILE * fp = fopen(filename,"r");
 if (fp!=0)
  {
   struct InputParserC * ipc = InputParser_Create(512,4);
   InputParser_SetDelimeter(ipc,1,'(');
   InputParser_SetDelimeter(ipc,2,',');
   InputParser_SetDelimeter(ipc,3,')');

   struct tagItemList tags;
   char * line = NULL;
   size_t len = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
       InputParser_SeperateWords(ipc,line,0);

       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"TITLE")  )   { InputParser_GetWord(ipc,1,configuration->pages.item[postNum].title  ,MAX_STR);   }
    }

    fprintf(stderr," Page Title %u --------------\n",postNum);
    fprintf(stderr,"   Title : %s \n",configuration->pages.item[postNum].title);

    fclose(fp);
    if (line) { free(line); }

    InputParser_Destroy(ipc);
    return 1;
  }
 return 0;
}


int loadPages(struct website * configuration)
{
  fprintf(stderr," Loading pages .. \n");

  char tmpPath[512]={0};
  struct AmmServer_MemoryHandler *  tmp=0;
  configuration->pages.currentItems=0;

  unsigned int loadedPages=0;
  for (loadedPages=0; loadedPages<10; loadedPages++)
  {
   //-------------------------------
   snprintf(tmpPath,512,"src/Services/MyBlog/res/pages/page%u.html",loadedPages);
   tmp = AmmServer_ReadFileToMemoryHandler(tmpPath);
   if (tmp!=0)
   {
    configuration->pages.item[configuration->pages.currentItems].content.data=tmp->content;
    configuration->pages.item[configuration->pages.currentItems].content.totalDataLength = tmp->contentSize;
    configuration->pages.item[configuration->pages.currentItems].content.currentDataLength  = tmp->contentCurrentLength;

    loadPageInfo(configuration,configuration->pages.currentItems);
    ++configuration->pages.currentItems;
   } else
   {
     return 0;
   }
  //-------------------------------
  }

  return 0;
}




