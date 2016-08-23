#include "database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../AmmServerlib/InputParser/InputParser_C.h"

struct website myblog={0};


int loadPostInfo(struct website * configuration,unsigned int postNum )
{
 char filename[FILENAME_MAX]={0};
 snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/posts/info%u.html",postNum);

 ssize_t read;


 FILE * fp = fopen(filename,"r");
 if (fp!=0)
  {
   struct InputParserC * ipc = InputParser_Create(512,5);

   struct tagItemList tags;
   char * line = NULL;
   size_t len = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
       printf("Retrieved line of length %zu :\n", read);
       printf("%s", line);

       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"TITLE")  )   { InputParser_GetWord(ipc,1,configuration->post.item[postNum].title  ,MAX_STR);  printf("=> TITLE \n");  } else
       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"DATE")   )   { InputParser_GetWord(ipc,1,configuration->post.item[postNum].dateStr,MAX_STR);  printf("=> DATE \n");   } else
       if ( InputParser_WordCompareNoCaseAuto(ipc,0,"AUTHOR") )   { InputParser_GetWord(ipc,1,configuration->post.item[postNum].author ,MAX_STR);  printf("=> AUTHOR \n"); } else
        {

        }
       //if ( InputParser_WordCompareNoCaseAuto(ipc,0,"TAGS") )     { InputParser_GetWord(ipc,0,configuration->post.item[postNum].tags ,MAX_STR); } else
       //if ( InputParser_WordCompareNoCaseAuto(ipc,0,"COMMENTS") ) { InputParser_GetWord(ipc,0,line,MAX_STR); }
    }

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
  FILE *fp = 0;

  unsigned int number=1;

  snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/posts/post%u.html",number);
  while (AmmServer_FileExists(filename))
  {

   struct AmmServer_MemoryHandler *  tmp = AmmServer_ReadFileToMemoryHandler(filename);
   if (tmp!=0)
   {
    fprintf(stderr," Loading post %u (%s) .. \n",number,filename);
    configuration->post.item[configuration->post.currentPosts].content.data = tmp->content;
    //If we didnt use this we shold -> AmmServer_FreeMemoryHandler(&tmp); tmp->content=0;

    loadPostInfo(configuration,configuration->post.currentPosts);
    //-------------
    ++configuration->post.currentPosts;
   }

    ++number;
    snprintf(filename,FILENAME_MAX,"src/Services/MyBlog/res/posts/post%u.html",number);
  }




  return 0;
}



