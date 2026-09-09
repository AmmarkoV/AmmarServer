#include "posts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/*Only the newest posts are ever drawn on a page , which is what bounds the size of a rendered wall..*/
#define MAX_POSTS_RENDERED 50

static struct socialPost posts[MAX_POSTS];
static unsigned int numberOfPosts=0;
static unsigned int nextPostID=1;
static char postsFilename[MAX_FILE_PATH]={0};

/*The server answers requests from a pool of threads , so every reader and writer of the store above goes
  through this one lock..*/
static pthread_mutex_t postsLock=PTHREAD_MUTEX_INITIALIZER;


/*Post text is stored raw and escaped at render time , so the only thing that has to be taken out of it here is
  whatever would break the one-record-per-line file format ( and any other control character while we are at
  it ). Leading/trailing whitespace goes too , so a form submitted with only spaces counts as empty..*/
static int sanitizeText(const char * input,char * output,unsigned int outputSize)
{
  if ( (input==0) || (output==0) || (outputSize==0) ) { return 0; }

  unsigned int i=0;
  while ( (input[i]==' ') || (input[i]=='\t') || (input[i]=='\n') || (input[i]=='\r') ) { ++i; }

  unsigned int o=0;
  for (; (input[i]!=0) && (o+1<outputSize); i++)
  {
    if ( (unsigned char) input[i] < 32 ) { output[o]=' '; } else { output[o]=input[i]; }
    ++o;
  }

  while ( (o>0) && (output[o-1]==' ') ) { --o; }
  output[o]=0;

  return (o>0);
}


/*Splits off the next tab separated field , leaving cursor on whatever follows it ( or 0 when the line ran
  out ) - the last field of a record is simply read off the cursor , so post text may contain spaces..*/
static char * nextField(char ** cursor)
{
  char * field = *cursor;
  if (field==0) { return 0; }

  char * tab = strchr(field,'\t');
  if (tab!=0) { *tab=0; *cursor=tab+1; } else { *cursor=0; }

  return field;
}


static struct socialPost * findPostUnlocked(unsigned int postID)
{
  unsigned int i=0;
  for (i=0; i<numberOfPosts; i++)
    { if (posts[i].id==postID) { return &posts[i]; } }
  return 0;
}


/*The whole store is small enough ( a few hundred KB at most ) that rewriting it after every change is simpler ,
  and leaves no append-only log to replay , compact or get out of step with memory..*/
static int savePostsUnlocked()
{
  if (postsFilename[0]==0) { return 0; }

  FILE * fp=fopen(postsFilename,"w");
  if (fp==0) { AmmServer_Warning("Could not write posts to %s",postsFilename); return 0; }

  unsigned int i=0,z=0;
  for (i=0; i<numberOfPosts; i++)
  {
    fprintf(fp,"P\t%u\t%lu\t%s\t%s\n",posts[i].id,posts[i].timestamp,posts[i].author,posts[i].text);
    fprintf(fp,"W\t%s\n",posts[i].wall);

    for (z=0; z<posts[i].numberOfComments; z++)
      { fprintf(fp,"C\t%lu\t%s\t%s\n",posts[i].comments[z].timestamp,posts[i].comments[z].author,posts[i].comments[z].text); }

    for (z=0; z<posts[i].numberOfLikes; z++)
      { fprintf(fp,"L\t%s\n",posts[i].likedBy[z]); }
  }

  fclose(fp);
  return 1;
}


int loadPosts(const char * filename)
{
  if (filename==0) { return 0; }
  snprintf(postsFilename,sizeof(postsFilename),"%s",filename);

  FILE * fp=fopen(filename,"r");
  if (fp==0) { AmmServer_Warning("No post database at %s yet , starting with an empty wall",filename); return 1; }

  /*Comments and likes belong to the last post read , which is how the file stays free of id lookups..*/
  struct socialPost * currentPost=0;
  char line[2048]={0};

  while ( fgets(line,sizeof(line),fp) != 0 )
  {
    unsigned int lineLength=strlen(line);
    while ( (lineLength>0) && ( (line[lineLength-1]=='\n') || (line[lineLength-1]=='\r') ) ) { line[--lineLength]=0; }
    if (lineLength<3) { continue; }

    char * cursor=line;
    char * kind=nextField(&cursor);
    if ( (kind==0) || (cursor==0) ) { continue; }

    if (strcmp(kind,"P")==0)
    {
      currentPost=0;

      char * idString        = nextField(&cursor);
      char * timestampString = nextField(&cursor);
      char * author          = nextField(&cursor);
      char * text            = cursor;
      if ( (idString==0) || (timestampString==0) || (author==0) || (text==0) ) { continue; }
      if (numberOfPosts>=MAX_POSTS)                                           { continue; }

      currentPost=&posts[numberOfPosts++];
      memset(currentPost,0,sizeof(struct socialPost));
      currentPost->id        = (unsigned int)  atoi(idString);
      currentPost->timestamp = (unsigned long) atol(timestampString);
      snprintf(currentPost->author,MAX_USERNAME,"%s",author);
      snprintf(currentPost->wall,MAX_USERNAME,"%s",author);
      snprintf(currentPost->text,MAX_POST_TEXT,"%s",text);

      if (currentPost->id>=nextPostID) { nextPostID=currentPost->id+1; }
    } else
    if ( (strcmp(kind,"C")==0) && (currentPost!=0) )
    {
      char * timestampString = nextField(&cursor);
      char * author          = nextField(&cursor);
      char * text            = cursor;
      if ( (timestampString==0) || (author==0) || (text==0) )     { continue; }
      if (currentPost->numberOfComments>=MAX_COMMENTS_PER_POST)   { continue; }

      struct socialComment * comment=&currentPost->comments[currentPost->numberOfComments++];
      comment->timestamp=(unsigned long) atol(timestampString);
      snprintf(comment->author,MAX_USERNAME,"%s",author);
      snprintf(comment->text,MAX_POST_TEXT,"%s",text);
    } else
    if ( (strcmp(kind,"W")==0) && (currentPost!=0) )
    {
      snprintf(currentPost->wall,MAX_USERNAME,"%s",cursor);
    } else
    if ( (strcmp(kind,"L")==0) && (currentPost!=0) )
    {
      if (currentPost->numberOfLikes>=MAX_LIKES_PER_POST) { continue; }
      snprintf(currentPost->likedBy[currentPost->numberOfLikes++],MAX_USERNAME,"%s",cursor);
    }
  }

  fclose(fp);
  AmmServer_Success("Loaded %u posts from %s",numberOfPosts,filename);
  return 1;
}


int unloadPosts()
{
  pthread_mutex_lock(&postsLock);
  int result=savePostsUnlocked();
  numberOfPosts=0;
  pthread_mutex_unlock(&postsLock);
  return result;
}


int addPost(const char * author,const char * wall,const char * text)
{
  if ( (author==0) || (author[0]==0) ) { return 0; }

  char cleanText[MAX_POST_TEXT]={0};
  if ( ! sanitizeText(text,cleanText,sizeof(cleanText)) ) { return 0; }

  pthread_mutex_lock(&postsLock);

  //A full wall makes room by dropping its oldest post..
  if (numberOfPosts>=MAX_POSTS)
  {
    memmove(&posts[0],&posts[1],sizeof(struct socialPost)*(MAX_POSTS-1));
    --numberOfPosts;
  }

  struct socialPost * post=&posts[numberOfPosts++];
  memset(post,0,sizeof(struct socialPost));
  post->id=nextPostID++;
  post->timestamp=(unsigned long) time(0);
  snprintf(post->author,MAX_USERNAME,"%s",author);
  snprintf(post->wall,MAX_USERNAME,"%s", ( (wall!=0) && (wall[0]!=0) ) ? wall : author );
  snprintf(post->text,MAX_POST_TEXT,"%s",cleanText);

  savePostsUnlocked();
  pthread_mutex_unlock(&postsLock);
  return 1;
}


int addComment(unsigned int postID,const char * author,const char * text)
{
  if ( (author==0) || (author[0]==0) ) { return 0; }

  char cleanText[MAX_POST_TEXT]={0};
  if ( ! sanitizeText(text,cleanText,sizeof(cleanText)) ) { return 0; }

  pthread_mutex_lock(&postsLock);

  int result=0;
  struct socialPost * post=findPostUnlocked(postID);
  if ( (post!=0) && (post->numberOfComments<MAX_COMMENTS_PER_POST) )
  {
    struct socialComment * comment=&post->comments[post->numberOfComments++];
    memset(comment,0,sizeof(struct socialComment));
    comment->timestamp=(unsigned long) time(0);
    snprintf(comment->author,MAX_USERNAME,"%s",author);
    snprintf(comment->text,MAX_POST_TEXT,"%s",cleanText);

    savePostsUnlocked();
    result=1;
  }

  pthread_mutex_unlock(&postsLock);
  return result;
}


int toggleLike(unsigned int postID,const char * username)
{
  if ( (username==0) || (username[0]==0) ) { return 0; }

  pthread_mutex_lock(&postsLock);

  int result=0;
  struct socialPost * post=findPostUnlocked(postID);
  if (post!=0)
  {
    unsigned int i=0;
    for (i=0; i<post->numberOfLikes; i++)
    {
      if (strcmp(post->likedBy[i],username)==0)
      {
        //Already liked , so this is an unlike - the last entry takes the freed slot , order doesn't matter here..
        snprintf(post->likedBy[i],MAX_USERNAME,"%s",post->likedBy[post->numberOfLikes-1]);
        --post->numberOfLikes;
        savePostsUnlocked();
        pthread_mutex_unlock(&postsLock);
        return 1;
      }
    }

    if (post->numberOfLikes<MAX_LIKES_PER_POST)
    {
      snprintf(post->likedBy[post->numberOfLikes++],MAX_USERNAME,"%s",username);
      savePostsUnlocked();
      result=1;
    }
  }

  pthread_mutex_unlock(&postsLock);
  return result;
}


static int viewerHasLikedUnlocked(struct socialPost * post,const char * viewer)
{
  unsigned int i=0;
  for (i=0; i<post->numberOfLikes; i++)
    { if (strcmp(post->likedBy[i],viewer)==0) { return 1; } }
  return 0;
}


static void formatTimestamp(unsigned long timestamp,char * output,unsigned int outputSize)
{
  time_t rawTime=(time_t) timestamp;
  struct tm brokenDownTime;
  if ( localtime_r(&rawTime,&brokenDownTime) == 0 ) { snprintf(output,outputSize,"unknown time"); return; }
  strftime(output,outputSize,"%d %b %Y %H:%M",&brokenDownTime);
}


unsigned int renderPosts(
                          char * buffer,
                          unsigned int bufferSize,
                          const char * onlyOnWall,
                          const char * viewer,
                          const char * csrfToken,
                          const char * backUser
                        )
{
  if ( (buffer==0) || (bufferSize==0) ) { return 0; }
  buffer[0]=0;

  if (backUser==0) { backUser=""; }

  //An empty multipart field makes the shared POST parser drop every field after it , so the back field is
  //emitted only when there actually is somewhere to go back to ( the feed has none - it is the default )..
  char backField[MAX_ESCAPED_USERNAME+64]={0};
  if (backUser[0]!=0)
    { snprintf(backField,sizeof(backField),"<input type=\"hidden\" name=\"back\" value=\"%s\">",backUser); }

  unsigned int position=0;
  unsigned int rendered=0;
  char chunk[4096]={0};
  char escapedText[MAX_POST_TEXT*6]={0};
  char escapedAuthor[MAX_ESCAPED_USERNAME]={0};
  char when[64]={0};

  pthread_mutex_lock(&postsLock);

  unsigned int i=numberOfPosts;
  while ( (i>0) && (rendered<MAX_POSTS_RENDERED) )
  {
    struct socialPost * post=&posts[--i];
    if ( (onlyOnWall!=0) && (strcmp(post->wall,onlyOnWall)!=0) ) { continue; }
    ++rendered;

    //A post on somebody else's wall says so in its header , so the home feed shows who wrote to whom..
    char wallMarker[MAX_ESCAPED_USERNAME+64]={0};
    if (strcmp(post->wall,post->author)!=0)
    {
      char escapedWall[MAX_ESCAPED_USERNAME]={0};
      AmmServer_HTMLEscape(post->wall,escapedWall,sizeof(escapedWall));
      snprintf(wallMarker,sizeof(wallMarker)," &rarr; <a href=\"profile.html?u=%s\">%s</a>",escapedWall,escapedWall);
    }

    AmmServer_HTMLEscape(post->author,escapedAuthor,sizeof(escapedAuthor));
    AmmServer_HTMLEscape(post->text,escapedText,sizeof(escapedText));
    formatTimestamp(post->timestamp,when,sizeof(when));

    snprintf(chunk,sizeof(chunk),
             "<div class=\"post\">"
              "<div class=\"postheader\"><a href=\"profile.html?u=%s\">%s</a>%s<span class=\"when\">%s</span></div>"
              "<div class=\"posttext\">%s</div>"
              "<div class=\"postactions\">"
               "<form method=\"post\" enctype=\"multipart/form-data\" action=\"like.html\">"
                "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
                "<input type=\"hidden\" name=\"post\" value=\"%u\">"
                "%s"
                "<button type=\"submit\" class=\"%s\">&hearts; %u</button>"
               "</form>"
              "</div>"
              "<div class=\"comments\">",
             escapedAuthor,escapedAuthor,wallMarker,when,escapedText,
             csrfToken,post->id,backField,
             viewerHasLikedUnlocked(post,viewer) ? "like liked" : "like",
             post->numberOfLikes);
    socialAppendChunk(buffer,bufferSize,&position,chunk);

    unsigned int z=0;
    for (z=0; z<post->numberOfComments; z++)
    {
      AmmServer_HTMLEscape(post->comments[z].author,escapedAuthor,sizeof(escapedAuthor));
      AmmServer_HTMLEscape(post->comments[z].text,escapedText,sizeof(escapedText));
      formatTimestamp(post->comments[z].timestamp,when,sizeof(when));

      snprintf(chunk,sizeof(chunk),
               "<div class=\"comment\"><a href=\"profile.html?u=%s\">%s</a> %s<span class=\"when\">%s</span></div>",
               escapedAuthor,escapedAuthor,escapedText,when);
      socialAppendChunk(buffer,bufferSize,&position,chunk);
    }

    snprintf(chunk,sizeof(chunk),
              "</div>"
              "<form class=\"commentform\" method=\"post\" enctype=\"multipart/form-data\" action=\"comment.html\">"
               "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
               "<input type=\"hidden\" name=\"post\" value=\"%u\">"
               "%s"
               "<input type=\"text\" name=\"text\" maxlength=\"280\" placeholder=\"Write a comment..\">"
               "<button type=\"submit\">Reply</button>"
              "</form>"
             "</div>",
             csrfToken,post->id,backField);
    socialAppendChunk(buffer,bufferSize,&position,chunk);
  }

  pthread_mutex_unlock(&postsLock);

  if (rendered==0) { socialAppendChunk(buffer,bufferSize,&position,"<div class=\"notice\">Nothing here yet..</div>"); }

  return rendered;
}
