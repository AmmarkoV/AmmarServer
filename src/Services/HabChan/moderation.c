#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "state.h"
#include "thread.h"
#include "post.h"
#include "moderation.h"

#include "../../AmmServerlib/AmmServerlib.h"

int deleteReply(const char * boardName , struct thread * t , unsigned int postIndex , const char * password)
{
  if ( (boardName==0) || (t==0) || (password==0) ) { return 0; }
  if ( (postIndex==0) || (postIndex>=t->numberOfReplies) ) { return 0; } //postIndex 0 is the OP , handled by deleteThread()

  struct post * p = &t->replies[postIndex];
  if (p->deleted) { return 0; }
  if ( (strlen(p->password)==0) || (strcmp(p->password,password)!=0) ) { return 0; }

  if ( p->hasFile && (strlen(p->fileCachedName)>0) )
  {
    char imagePath[MAX_FILE_PATH*2]={0};
    snprintf(imagePath,sizeof(imagePath),"%sboard/%s/%s/%s",dataRoot,boardName,t->name,p->fileCachedName);
    unlink(imagePath);
    if (t->numberOfImages>0) { --t->numberOfImages; }
  }

  p->deleted=1;
  p->hasFile=0;
  p->fileOriginalName[0]=0;
  p->fileCachedName[0]=0;
  if (p->message!=0) { free(p->message); }
  p->message=strdup("");
  p->messageSize=0;

  char headerPath[MAX_FILE_PATH*2]={0};
  snprintf(headerPath,sizeof(headerPath),"%sboard/%s/%s/header_%u",dataRoot,boardName,t->name,postIndex);
  savePostHeader(headerPath,p);

  char postPath[MAX_FILE_PATH*2]={0};
  snprintf(postPath,sizeof(postPath),"%sboard/%s/%s/post_%u",dataRoot,boardName,t->name,postIndex);
  savePostContent(postPath,p);

  saveThreadStatus(boardName,t);

  return 1;
}


int deleteThread(const char * boardName , struct thread * t , const char * password)
{
  if ( (boardName==0) || (t==0) || (password==0) ) { return 0; }
  if (t->deleted) { return 0; }
  if ( (strlen(t->password)==0) || (strcmp(t->password,password)!=0) ) { return 0; }

  //Best effort only : remove the OP's own image , the rest of the thread's files are simply left ( orphaned )
  //on disk since deleting them all would need recursing the directory , which is more risk than this is worth.
  if ( (t->numberOfReplies>0) && (t->replies[0].hasFile) && (strlen(t->replies[0].fileCachedName)>0) )
  {
    char imagePath[MAX_FILE_PATH*2]={0};
    snprintf(imagePath,sizeof(imagePath),"%sboard/%s/%s/%s",dataRoot,boardName,t->name,t->replies[0].fileCachedName);
    unlink(imagePath);
  }

  t->deleted=1;
  saveThreadStatus(boardName,t);

  return 1;
}


void * processDeletePost(struct AmmServer_DynamicRequest * rqst)
{
  char boardName[MAX_STRING_SIZE]={0};
  char threadName[MAX_STRING_SIZE]={0};
  char password[MAX_STRING_SIZE]={0};
  char postIndexStr[32]={0};

  _POSTcpy(rqst,"board",boardName,MAX_STRING_SIZE);
  _POSTcpy(rqst,"thread",threadName,MAX_STRING_SIZE);
  _POSTcpy(rqst,"postpassword",password,MAX_STRING_SIZE);
  _POSTcpy(rqst,"postindex",postIndexStr,sizeof(postIndexStr));
  unsigned int postIndex = (unsigned int) atoi(postIndexStr);

  unsigned int success=0;
  const char * message = "Delete failed : wrong password";

  unsigned long boardIndex=0;
  unsigned long encoded=0;
  if ( hashMap_GetULongPayload(boardHashMap,boardName,&boardIndex) &&
       hashMap_GetULongPayload(threadHashMap,threadName,&encoded) &&
       ( (encoded / MAX_THREADS_PER_BOARD) == boardIndex ) )
  {
    struct thread * t = &ourSite.boards[boardIndex].threads[encoded % MAX_THREADS_PER_BOARD];

    if (!t->deleted)
    {
      if (postIndex==0)
      {
        if ( deleteThread(boardName,t,password) ) { success=1; message="Thread deleted"; }
      } else
      {
        if ( deleteReply(boardName,t,postIndex,password) ) { success=1; message="Post deleted"; }
      }
    }
  }

  char redirectURL[MAX_FILE_PATH*2]={0};
  if ( success && (postIndex==0) )
  {
    snprintf(redirectURL,sizeof(redirectURL),"threadIndexView.html?board=%s",boardName);
  } else
  {
    snprintf(redirectURL,sizeof(redirectURL),"threadView.html?board=%s&thread=%s",boardName,threadName);
  }

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><meta http-equiv=\"refresh\" content=\"1; url=%s\"></head><body>%s</body></html>",
           redirectURL,message);
  rqst->contentSize=strlen(rqst->content);

  return 0;
}
