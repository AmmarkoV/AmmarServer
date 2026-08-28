
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "state.h"
#include "thread.h"
#include "post.h"
#include "board.h"
#include "csrf.h"


#include "../../AmmServerlib/AmmServerlib.h"
#include "../../InputParser/InputParser_C.h"

#define THREADS_PER_PAGE 10
#define THREAD_LIST_BUFFER_CAPACITY 140000

static void formatTimestampHTML(struct timestamp * t , char * buf , unsigned int bufSize)
{
  snprintf(buf,bufSize,"%04u-%02u-%02u %02u:%02u:%02u",t->year,t->month,t->day,t->hour,t->minute,t->second);
}


//Builds the image ( or placeholder ) shown for one post. Prefers a thumbnail if one was generated for the
//upload , but the click-through link always goes to the full sized original.
static void buildImageBlock(char * imageBlock , unsigned int imageBlockSize , const char * boardName , const char * threadName , struct post * p , int showPlaceholderIfMissing)
{
  imageBlock[0]=0;

  if ( (!p->hasFile) || (strlen(p->fileCachedName)==0) )
  {
    if (showPlaceholderIfMissing) { snprintf(imageBlock,imageBlockSize,"<img src=\"empty.png\" width=\"10\">"); }
    return;
  }

  char thumbName[MAX_STRING_SIZE]={0};
  deriveThumbnailName(p->fileCachedName,thumbName,sizeof(thumbName));

  char thumbPath[MAX_STRING_SIZE*2]={0};
  snprintf(thumbPath,sizeof(thumbPath),"data/board/%s/%s/%s",boardName,threadName,thumbName);

  const char * displaySrc = AmmServer_FileExists(thumbPath) ? thumbName : p->fileCachedName;

  snprintf(imageBlock,imageBlockSize,
           "<a href=\"board/%s/%s/%s\" target=\"_new\"><img src=\"board/%s/%s/%s\" height=\"200\"></a>",
           boardName,threadName,p->fileCachedName,
           boardName,threadName,displaySrc);
}


//Small inline "delete your own post" form , shown under every post ( postIndex 0 deletes the whole thread )
static void appendDeleteForm(char * buffer , unsigned int bufferCapacity , const char * boardName , const char * threadName , unsigned int postIndex)
{
  char chunk[700]={0};
  snprintf(chunk,sizeof(chunk),
           "<form action=\"deletePost.html\" method=\"post\" style=\"display:inline\">"
           "<input type=\"hidden\" name=\"board\" value=\"%s\">"
           "<input type=\"hidden\" name=\"thread\" value=\"%s\">"
           "<input type=\"hidden\" name=\"postindex\" value=\"%u\">"
           "<input type=\"password\" name=\"postpassword\" size=\"6\" placeholder=\"pwd\">"
           "<input type=\"submit\" value=\"Del\">"
           "</form>",
           boardName,threadName,postIndex);
  strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
}


//Renders the HTML for the very first post of a thread ( the OP ) , used both by the board index ( preview ) and the full thread view
static void appendOPPostHTML(char * buffer , unsigned int bufferCapacity , const char * boardName , struct thread * t)
{
  char chunk[12000]={0}; //Generous : op/title/message can each expand up to 6x once HTML-escaped
  char imageBlock[1200]={0};
  char timeStr[64]={0};
  formatTimestampHTML(&t->creation,timeStr,64);

  struct post * op = (t->numberOfReplies>0) ? &t->replies[0] : 0;
  if (op!=0) { buildImageBlock(imageBlock,sizeof(imageBlock),boardName,t->name,op,0); }

  //t->op , t->title and the post message all came in from a POST request , so they must be HTML-escaped
  //before landing in the page , otherwise a post could inject markup/script or break the surrounding HTML..!
  char escapedOp[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(t->op,escapedOp,sizeof(escapedOp));

  char escapedTitle[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(t->title,escapedTitle,sizeof(escapedTitle));

  char escapedMessage[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape((op!=0 && op->message!=0) ? op->message : "",escapedMessage,sizeof(escapedMessage));

  char subject[sizeof(escapedTitle)+16]={0};
  if (strlen(escapedTitle)>0) { snprintf(subject,sizeof(subject)," <b>%s</b>",escapedTitle); }

  char stickyBadge[32]={0};
  if (t->sticky) { snprintf(stickyBadge,sizeof(stickyBadge),"<b>[Sticky]</b> "); }

  char deleteForm[700]={0};
  appendDeleteForm(deleteForm,sizeof(deleteForm),boardName,t->name,0);

  snprintf(chunk,sizeof(chunk),
           "<div style=\"background-color:#ffffee;\">"
           "<hr><br>"
           "<div>"
           "<table width=\"400\" style=\"background-color:#f0e0d6;\">"
           "<tr><td colspan=2>%s%s%s %s No.%s <a href=\"threadView.html?board=%s&thread=%s\">[Reply]</a> \xe2\x96\xb6 %s</td></tr>"
           "<tr><td>%s</td> <td> %s </td></tr>"
           "</table>"
           "</div><br><br>",
           stickyBadge , escapedOp , subject , timeStr , t->name ,
           boardName , t->name , deleteForm ,
           imageBlock , escapedMessage );

  strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
}


//Renders the HTML for a single reply post , used both by the board index ( preview ) and the full thread view
static void appendReplyPostHTML(char * buffer , unsigned int bufferCapacity , const char * boardName , const char * threadName , struct post * p , unsigned int postIndex)
{
  char chunk[12000]={0}; //Generous : op/message can each expand up to 6x once HTML-escaped

  if (p->deleted)
  {
    snprintf(chunk,sizeof(chunk),
             "<div style=\"background-color:#f0e0d6;\"><table>"
             "<tr><td height=30><i>Post No.%s.%u deleted</i></td></tr>"
             "</table></div><br>",
             threadName,postIndex);
    strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
    return;
  }

  char imageBlock[1200]={0};
  buildImageBlock(imageBlock,sizeof(imageBlock),boardName,threadName,p,1);

  char timeStr[64]={0};
  formatTimestampHTML(&p->creation,timeStr,64);

  //p->op and the post message both came in from a POST request , so they must be HTML-escaped before landing
  //in the page , otherwise a reply could inject markup/script or break the surrounding HTML..!
  char escapedOp[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape(p->op,escapedOp,sizeof(escapedOp));

  char escapedMessage[MAX_STRING_SIZE*6]={0};
  AmmServer_HTMLEscape((p->message!=0) ? p->message : "",escapedMessage,sizeof(escapedMessage));

  char deleteForm[700]={0};
  appendDeleteForm(deleteForm,sizeof(deleteForm),boardName,threadName,postIndex);

  snprintf(chunk,sizeof(chunk),
           "<div style=\"background-color:#f0e0d6;\">"
           "<table>"
           "<tr><td colspan=2 height=30>%s %s No.%s.%u \xe2\x96\xb6 %s</td></tr>"
           "<tr><td>%s</td> <td> %s </td></tr>"
           "</table>"
           "</div><br>",
           escapedOp , timeStr , threadName , postIndex , deleteForm ,
           imageBlock , escapedMessage );

  strncat(buffer,chunk,bufferCapacity - strlen(buffer) - 1);
}


struct threadSortEntry
{
  unsigned int slot;
  unsigned char sticky;
  long long bumpKey;
};

static long long timestampSortKey(struct timestamp * t)
{
  return ((long long)t->year)*10000000000LL + ((long long)t->month)*100000000LL + ((long long)t->day)*1000000LL +
         ((long long)t->hour)*10000LL + ((long long)t->minute)*100LL + (long long)t->second;
}

//Sticky threads always come first ; within each group , most recently bumped ( replied to ) comes first
static int compareThreadSortEntries(const void * a , const void * b)
{
  const struct threadSortEntry * ea = (const struct threadSortEntry *) a;
  const struct threadSortEntry * eb = (const struct threadSortEntry *) b;

  if (ea->sticky != eb->sticky) { return (eb->sticky>ea->sticky) ? 1 : -1; }
  if (eb->bumpKey > ea->bumpKey) { return 1;  }
  if (eb->bumpKey < ea->bumpKey) { return -1; }
  return 0;
}


//Renders one page ( THREADS_PER_PAGE threads ) of a board's thread listing , sorted sticky-first then by
//bump order ( most recently replied to first ) , skipping deleted threads entirely.
char * mallocHTMLListOfThreadsOfBoard(const char * boardName , unsigned int page , unsigned int * htmlLength , unsigned int * outTotalThreads)
{
    unsigned int bufferCapacity = THREAD_LIST_BUFFER_CAPACITY;
    char * buffer=(char*) malloc(sizeof(char) * bufferCapacity);
    if (buffer==0) { return 0; }
    buffer[0]=0;

    if (outTotalThreads!=0) { *outTotalThreads=0; }

    unsigned long boardIndex = 0;
    if ( hashMap_GetULongPayload(boardHashMap,boardName,&boardIndex) )
    {
        struct board * b = &ourSite.boards[boardIndex];

        struct threadSortEntry * order = (struct threadSortEntry *) malloc(sizeof(struct threadSortEntry) * (b->currentThreads+1));
        if (order==0) { return buffer; }

        unsigned int liveCount=0;
        unsigned int i=0;
        for (i=0; i<b->currentThreads; i++)
        {
          if (b->threads[i].deleted) { continue; }
          order[liveCount].slot   = i;
          order[liveCount].sticky = b->threads[i].sticky;
          order[liveCount].bumpKey= timestampSortKey(&b->threads[i].lastReply);
          ++liveCount;
        }

        qsort(order,liveCount,sizeof(struct threadSortEntry),compareThreadSortEntries);

        if (outTotalThreads!=0) { *outTotalThreads=liveCount; }

        unsigned int startIdx = page*THREADS_PER_PAGE;
        unsigned int endIdx   = startIdx+THREADS_PER_PAGE;
        if (endIdx>liveCount) { endIdx=liveCount; }

        for (i=startIdx; i<endIdx; i++)
        {
               struct thread * t = &b->threads[order[i].slot];

               appendOPPostHTML(buffer,bufferCapacity,boardName,t);

               unsigned int postID=0;
               unsigned int shown=0;
               for (postID=1; (postID<t->numberOfReplies) && (shown<3); postID++)
               {
                 appendReplyPostHTML(buffer,bufferCapacity,boardName,t->name,&t->replies[postID],postID);
                 ++shown;
               }

               strncat(buffer,"</div>",bufferCapacity - strlen(buffer) - 1);
        }

        free(order);
    } else
    {
     AmmServer_Error("Could not find board %s \n",boardName);
    }

  if (htmlLength!=0) { *htmlLength = strlen(buffer); }
  return buffer;
}


//Appends "Page X / Y   « Prev | Next »" navigation , omitting whichever link would go out of range
static void appendPageNavHTML(char * buffer , unsigned int bufferCapacity , const char * boardName , unsigned int page , unsigned int totalThreads)
{
  unsigned int totalPages = (totalThreads+THREADS_PER_PAGE-1)/THREADS_PER_PAGE;
  if (totalPages==0) { totalPages=1; }

  char prevLink[300]={0};
  if (page>0) { snprintf(prevLink,sizeof(prevLink),"<a href=\"threadIndexView.html?board=%s&page=%u\">&laquo; Prev</a>",boardName,page-1); }

  char nextLink[300]={0};
  if (page+1<totalPages) { snprintf(nextLink,sizeof(nextLink),"<a href=\"threadIndexView.html?board=%s&page=%u\">Next &raquo;</a>",boardName,page+1); }

  char nav[900]={0};
  snprintf(nav,sizeof(nav),"<center>Page %u / %u &nbsp; %s &nbsp; %s</center><br>",page+1,totalPages,prevLink,nextLink);

  strncat(buffer,nav,bufferCapacity - strlen(buffer) - 1);
}


void * prepareThreadIndexView(struct AmmServer_DynamicRequest  * rqst)
{
 fprintf(stderr,"prepareThreadIndexView  \n");

         char boardID[257]={0};
         if ( _GETcpy(rqst,"board",boardID,256) )
             {
                fprintf(stderr,"board: %s \n",boardID);
                if ( hashMap_ContainsKey(boardHashMap,boardID) )
                {
                  struct AmmServer_MemoryHandler * threadIndexPageWithContents = AmmServer_CopyMemoryHandler(threadIndexPage);

                  unsigned int page = _GETuint(rqst,"page");

                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!BOARDNAME!",boardID);
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!THREADID!","new");

                  char csrfToken[33]={0};
                  generateCSRFToken(csrfToken,sizeof(csrfToken));
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!CSRFTOKEN!",csrfToken);

                  char captchaID[16]={0};
                  snprintf(captchaID,sizeof(captchaID),"%u",rand());
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,2,"!CAPTCHAID!",captchaID);

                  char * channelList = mallocChannelListHTML(boardID);
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,2,"!list channels here!",(channelList!=0)?channelList:"");
                  if (channelList!=0) { free(channelList); }
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!Say Which Board we are in here!",boardID);
                  AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"!HabChan Dynamic Title here!","HabChan @ AmmarServer");



                  unsigned int threadsHTMLLength=0;
                  unsigned int totalThreads=0;
                  char * threadsHTML = mallocHTMLListOfThreadsOfBoard(boardID,page,&threadsHTMLLength,&totalThreads);
                  if (threadsHTML!=0)
                   {
                    appendPageNavHTML(threadsHTML,THREAD_LIST_BUFFER_CAPACITY,boardID,page,totalThreads);
                    AmmServer_ReplaceAllVarsInMemoryHandler(threadIndexPageWithContents,1,"<!--THREAD_CONTENT-->",threadsHTML);
                    free(threadsHTML);
                   }


                   unsigned long copyLength = threadIndexPageWithContents->contentCurrentLength;
                   if (copyLength >= rqst->MAXcontentSize) { copyLength = rqst->MAXcontentSize-1; }
                   memcpy (rqst->content , threadIndexPageWithContents ->content , copyLength );
                   rqst->contentSize=copyLength ;
                   AmmServer_FreeMemoryHandler(&threadIndexPageWithContents);
                   return 0;
                } else
                {
                  strcat(rqst->content,"No BOARD  , denied!!!  <BR> ");
                }
             }


   strcpy(rqst->content,"<html>ERROR</html>");
   rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * prepareThreadView(struct AmmServer_DynamicRequest  * rqst)
{
   char boardID[257]={0};
   char threadID[257]={0};

   if ( (_GETcpy(rqst,"board",boardID,256)) && (_GETcpy(rqst,"thread",threadID,256)) )
   {
     unsigned long boardIndex=0;
     if ( hashMap_GetULongPayload(boardHashMap,boardID,&boardIndex) )
     {
       unsigned long encoded=0;
       if ( hashMap_GetULongPayload(threadHashMap,threadID,&encoded) )
       {
         unsigned long threadBoardIndex = encoded / MAX_THREADS_PER_BOARD;
         unsigned long threadSlot       = encoded % MAX_THREADS_PER_BOARD;

         if (threadBoardIndex==boardIndex)
         {
           struct thread * t = &ourSite.boards[boardIndex].threads[threadSlot];

           if (!t->deleted)
           {
           struct AmmServer_MemoryHandler * threadViewPageWithContents = AmmServer_CopyMemoryHandler(threadIndexPage);

           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,1,"!BOARDNAME!",boardID);
           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,1,"!THREADID!",threadID);

           char csrfToken[33]={0};
           generateCSRFToken(csrfToken,sizeof(csrfToken));
           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,1,"!CSRFTOKEN!",csrfToken);

           char captchaID[16]={0};
           snprintf(captchaID,sizeof(captchaID),"%u",rand());
           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,2,"!CAPTCHAID!",captchaID);

           char * channelList = mallocChannelListHTML(boardID);
           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,2,"!list channels here!",(channelList!=0)?channelList:"");
           if (channelList!=0) { free(channelList); }
           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,1,"!Say Which Board we are in here!",boardID);

           //t->title lands inside <title> , an HTML "raw text" element : a raw "</title>" in it would let a post
           //break out of the title and inject markup into <head> , so this needs escaping same as everywhere else
           char escapedPageTitle[MAX_STRING_SIZE*6]={0};
           AmmServer_HTMLEscape(t->title,escapedPageTitle,sizeof(escapedPageTitle));
           AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,1,"!HabChan Dynamic Title here!",(strlen(escapedPageTitle)>0)?escapedPageTitle:"HabChan @ AmmarServer");

           unsigned int threadHTMLCapacity = THREAD_LIST_BUFFER_CAPACITY;
           char * threadHTML = (char *) malloc(sizeof(char) * threadHTMLCapacity);
           if (threadHTML!=0)
           {
             threadHTML[0]=0;
             appendOPPostHTML(threadHTML,threadHTMLCapacity,boardID,t);

             unsigned int postID=0;
             for (postID=1; postID<t->numberOfReplies; postID++)
             {
               appendReplyPostHTML(threadHTML,threadHTMLCapacity,boardID,t->name,&t->replies[postID],postID);
             }
             strncat(threadHTML,"</div>",threadHTMLCapacity - strlen(threadHTML) - 1);

             AmmServer_ReplaceAllVarsInMemoryHandler(threadViewPageWithContents,1,"<!--THREAD_CONTENT-->",threadHTML);
             free(threadHTML);
           }

           unsigned long copyLength = threadViewPageWithContents->contentCurrentLength;
           if (copyLength >= rqst->MAXcontentSize) { copyLength = rqst->MAXcontentSize-1; }
           memcpy (rqst->content , threadViewPageWithContents->content , copyLength );
           rqst->contentSize = copyLength;
           AmmServer_FreeMemoryHandler(&threadViewPageWithContents);
           return 0;
           }
         }
       }
     }
   }

   snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Thread not found</body></html>" );
   rqst->contentSize=strlen(rqst->content);
   return 0;
}



int loadThread(const char * threadName , struct board * ourBoard , struct thread * ourThread)
{
   if (ourBoard==0)  { fprintf(stderr,"Cannot load thread without an allocated board\n");  return 0; }
   if (ourThread==0) { fprintf(stderr,"Cannot load thread without an allocated thread\n"); return 0; }
   fprintf(stderr,"Loading Thread `%s` to board `%s` \n",threadName,ourBoard->name);

   //ourBoard->threads[] comes from a plain malloc() , not calloc() , so fields with no line in an older
   //status.ini ( deleted , password , ... ) must be explicitly zeroed here rather than left as garbage..!
   memset(ourThread,0,sizeof(struct thread));
   snprintf(ourThread->name,MAX_STRING_SIZE,"%s",threadName);

   char filename[LINE_MAX_LENGTH]={0};
   snprintf(filename,LINE_MAX_LENGTH,"data/board/%s/%s/status.ini",ourBoard->name,threadName);
   char line [LINE_MAX_LENGTH]={0};
   //Try and open filename
   FILE * fp = fopen(filename,"r");
   if (fp == 0 ) { fprintf(stderr,"Cannot open loadBoardSettings file %s \n",filename); return 0; }

    //Allocate a token parser
    struct InputParserC * ipc=0;
    ipc = InputParser_Create(LINE_MAX_LENGTH,5);
    if (ipc==0) { fprintf(stderr,"Cannot allocate memory for new loadBoardSettings parser\n"); return 0; }

    while (!feof(fp))
       {
         //We get a new line out of the file
         unsigned int readOpResult = (fgets(line,LINE_MAX_LENGTH,fp)!=0);
         if ( readOpResult != 0 )
           {
             //We tokenize it
             unsigned int words_count = InputParser_SeperateWords(ipc,line,0);
             if ( words_count > 0 )
              {
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"OP")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->op,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"PASSWORD")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->password,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"DELETED")==1)
                {
                   ourThread->deleted =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"TITLE")==1)
                {
                   InputParser_GetWord(ipc,1,ourThread->title,MAX_STRING_SIZE);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"LASTREPLY")==1)
                {
                   ourThread->lastReply.year   =  InputParser_GetWordInt(ipc,1);
                   ourThread->lastReply.month  =  InputParser_GetWordInt(ipc,2);
                   ourThread->lastReply.day    =  InputParser_GetWordInt(ipc,3);
                   ourThread->lastReply.hour   =  InputParser_GetWordInt(ipc,4);
                   ourThread->lastReply.minute =  InputParser_GetWordInt(ipc,5);
                   ourThread->lastReply.second =  InputParser_GetWordInt(ipc,6);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"NUMBEROFREPLIES")==1)
                {
                   ourThread->numberOfReplies =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"NUMBEROFIMAGES")==1)
                {
                   ourThread->numberOfImages =  InputParser_GetWordInt(ipc,1);
                } else
                if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"STICKY")==1)
                {
                   ourThread->sticky =  InputParser_GetWordInt(ipc,1);
                }
              }
          }
       }

    InputParser_Destroy(ipc);
    fclose(fp);

    ourThread->creation = ourThread->lastReply;

    return loadPosts(ourBoard,ourThread);
}


int saveThreadStatus(const char * boardName , struct thread * ourThread)
{
   if ( (boardName==0) || (ourThread==0) ) { return 0; }

   char filename[LINE_MAX_LENGTH]={0};
   snprintf(filename,LINE_MAX_LENGTH,"data/board/%s/%s/status.ini",boardName,ourThread->name);

   FILE * fp = fopen(filename,"w");
   if (fp == 0 ) { fprintf(stderr,"Cannot open %s for writing\n",filename); return 0; }

   fprintf(fp,"op(%s)\n",ourThread->op);
   fprintf(fp,"password(%s)\n",ourThread->password);
   fprintf(fp,"deleted(%u)\n",ourThread->deleted);
   fprintf(fp,"title(%s)\n",ourThread->title);
   fprintf(fp,"lastReply(%u,%u,%u,%u,%u,%u)\n",
           ourThread->lastReply.year , ourThread->lastReply.month , ourThread->lastReply.day ,
           ourThread->lastReply.hour , ourThread->lastReply.minute , ourThread->lastReply.second);
   fprintf(fp,"numberOfReplies(%u)\n",ourThread->numberOfReplies);
   fprintf(fp,"numberOfImages(%u)\n",ourThread->numberOfImages);
   fprintf(fp,"sticky(%u)\n",ourThread->sticky);

   fclose(fp);
   return 1;
}


int addThreadToBoard( const char * boardName , const char * threadName )
{
  fprintf(stderr,"Adding Thread `%s` to board `%s` \n",threadName,boardName);

  unsigned long boardID=0;
  if ( ! hashMap_GetULongPayload(boardHashMap,boardName,&boardID) )
  {
    fprintf(stderr,"Could not find board name `%s` , Cannot create a thread in non existing board\n", boardName);
    return 0;
  }

  struct board * ourBoard = &ourSite.boards[boardID];
  if ( ourBoard->currentThreads >= ourBoard->maxThreads )
  {
    fprintf(stderr,"Board `%s` is full , cannot add thread `%s`\n",boardName,threadName);
    return 0;
  }

  //The slot inside ourBoard->threads[] this thread will permanently live at
  unsigned int slot = ourBoard->currentThreads;
  if ( loadThread(threadName , ourBoard , &ourBoard->threads[slot]) )
  {
    //Payload encodes both which board and which slot , so a later lookup by thread name alone
    //( from postReceiver / threadView ) can find it again , regardless of how threadHashMap gets sorted internally
    hashMap_AddULong(threadHashMap,threadName,(unsigned long)boardID * MAX_THREADS_PER_BOARD + slot);
    ++ourBoard->currentThreads;
    return 1;
  }

 fprintf(stderr,"Failed to add thread %s to board %s\n", threadName,boardName);
 return 0;
}


int createThread(
                   const char * boardName ,
                   const char * op ,
                   const char * title ,
                   const char * password ,
                   struct post * opPost ,
                   const char * fileBytes ,
                   unsigned int fileBytesSize ,
                   char * outThreadName ,
                   unsigned int outThreadNameSize
                 )
{
  unsigned long boardID=0;
  if ( ! hashMap_GetULongPayload(boardHashMap,boardName,&boardID) )
  {
    fprintf(stderr,"createThread : board `%s` does not exist\n",boardName);
    return 0;
  }

  struct board * ourBoard = &ourSite.boards[boardID];
  if ( ourBoard->currentThreads >= ourBoard->maxThreads )
  {
    fprintf(stderr,"createThread : board `%s` is full\n",boardName);
    return 0;
  }

  char threadName[MAX_STRING_SIZE]={0};
  snprintf(threadName,MAX_STRING_SIZE,"%09u",ourBoard->threadUID);

  char dirPath[MAX_STRING_SIZE*2]={0};
  snprintf(dirPath,sizeof(dirPath),"data/board/%s/%s",boardName,threadName);
  if ( mkdir(dirPath,0755) != 0 )
  {
    fprintf(stderr,"createThread : cannot create directory `%s` (%s)\n",dirPath,strerror(errno));
    return 0;
  }

  unsigned int slot = ourBoard->currentThreads;
  struct thread * t = &ourBoard->threads[slot];
  memset(t,0,sizeof(struct thread));
  snprintf(t->name,MAX_STRING_SIZE,"%s",threadName);
  snprintf(t->op,MAX_STRING_SIZE,"%s",(op!=0)?op:"Anonymous");
  snprintf(t->title,MAX_STRING_SIZE,"%s",(title!=0)?title:"");
  snprintf(t->password,MAX_STRING_SIZE,"%s",(password!=0)?password:"");
  t->sticky=0;
  t->repliable=1;
  fillTimestampNow(&t->creation);
  t->maxNumberOfReplies=MAX_POSTS_PER_THREAD;
  t->numberOfReplies=0;
  t->numberOfImages=0;
  t->replies=(struct post *) malloc(sizeof(struct post) * MAX_POSTS_PER_THREAD);
  if (t->replies==0)
  {
    fprintf(stderr,"createThread : cannot allocate replies for thread `%s`\n",threadName);
    return 0;
  }
  memset(t->replies,0,sizeof(struct post) * MAX_POSTS_PER_THREAD);

  if ( ! addPostToThread(boardName,t,opPost,fileBytes,fileBytesSize) )
  {
    fprintf(stderr,"createThread : failed to store first post for thread `%s`\n",threadName);
    free(t->replies);
    t->replies=0;
    return 0;
  }

  hashMap_AddULong(threadHashMap,threadName,(unsigned long)boardID * MAX_THREADS_PER_BOARD + slot);
  ++ourBoard->threadUID;
  ++ourBoard->currentThreads;

  if ( (outThreadName!=0) && (outThreadNameSize>0) ) { snprintf(outThreadName,outThreadNameSize,"%s",threadName); }
  return 1;
}
