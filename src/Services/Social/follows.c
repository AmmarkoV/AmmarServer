#include "follows.h"
#include "notifications.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define PEOPLE_RENDER_BUFFER_SIZE (256*1024)

struct socialFollow
{
  char follower[MAX_USERNAME];
  char followee[MAX_USERNAME];
};

static struct socialFollow follows[MAX_FOLLOWS];
static unsigned int numberOfFollows=0;
static char followsFilename[MAX_FILE_PATH]={0};

/*Guards everything above , same as the post store - requests arrive on a pool of threads. A reader may take
  this lock while already holding the post store's one ( the feed asks "do I follow this author" while it
  renders ) , so nothing here may ever reach back into the post store..*/
static pthread_mutex_t followsLock=PTHREAD_MUTEX_INITIALIZER;


static int findFollowUnlocked(const char * follower,const char * followee)
{
  unsigned int i=0;
  for (i=0; i<numberOfFollows; i++)
  {
    if ( (strcmp(follows[i].follower,follower)==0) && (strcmp(follows[i].followee,followee)==0) ) { return (int) i; }
  }
  return -1;
}


static int saveFollowsUnlocked()
{
  if (followsFilename[0]==0) { return 0; }

  FILE * fp=fopen(followsFilename,"w");
  if (fp==0) { AmmServer_Warning("Could not write follows to %s",followsFilename); return 0; }

  unsigned int i=0;
  for (i=0; i<numberOfFollows; i++) { fprintf(fp,"F\t%s\t%s\n",follows[i].follower,follows[i].followee); }

  fclose(fp);
  return 1;
}


int loadFollows(const char * filename)
{
  if (filename==0) { return 0; }
  snprintf(followsFilename,sizeof(followsFilename),"%s",filename);

  FILE * fp=fopen(filename,"r");
  if (fp==0) { AmmServer_Warning("Nobody follows anybody yet ( no %s )",filename); return 1; }

  char line[512]={0};
  while ( fgets(line,sizeof(line),fp) != 0 )
  {
    unsigned int lineLength=strlen(line);
    while ( (lineLength>0) && ( (line[lineLength-1]=='\n') || (line[lineLength-1]=='\r') ) ) { line[--lineLength]=0; }
    if (lineLength<5) { continue; }

    if (strncmp(line,"F\t",2)!=0) { continue; }

    char * follower=line+2;
    char * separator=strchr(follower,'\t');
    if (separator==0) { continue; }
    *separator=0;
    char * followee=separator+1;

    if (numberOfFollows>=MAX_FOLLOWS) { continue; }
    //A row naming something that could never be an account is a corrupt row , not a follow to silently
    //truncate into one that fits..
    if ( ( ! uadbWeb_isValidUsername(follower) ) || ( ! uadbWeb_isValidUsername(followee) ) ) { continue; }

    snprintf(follows[numberOfFollows].follower,MAX_USERNAME,"%.*s",MAX_USERNAME-1,follower);
    snprintf(follows[numberOfFollows].followee,MAX_USERNAME,"%.*s",MAX_USERNAME-1,followee);
    ++numberOfFollows;
  }

  fclose(fp);
  AmmServer_Success("Loaded %u follows from %s",numberOfFollows,filename);
  return 1;
}


int unloadFollows()
{
  pthread_mutex_lock(&followsLock);
  int result=saveFollowsUnlocked();
  numberOfFollows=0;
  pthread_mutex_unlock(&followsLock);
  return result;
}


int followsIsFollowing(const char * follower,const char * followee)
{
  if ( (follower==0) || (followee==0) ) { return 0; }

  pthread_mutex_lock(&followsLock);
  int result = ( findFollowUnlocked(follower,followee) >= 0 );
  pthread_mutex_unlock(&followsLock);
  return result;
}


int followsToggle(const char * follower,const char * followee)
{
  if ( (follower==0) || (followee==0) )    { return 0; }
  if (strcmp(follower,followee)==0)        { return 0; } //Following yourself is what the feed already does

  pthread_mutex_lock(&followsLock);

  int result=0;
  int nowFollowing=0;
  int existing=findFollowUnlocked(follower,followee);
  if (existing>=0)
  {
    //The last entry takes the freed slot , order does not matter here..
    follows[existing]=follows[numberOfFollows-1];
    --numberOfFollows;
    result=1;
  } else
  if (numberOfFollows<MAX_FOLLOWS)
  {
    snprintf(follows[numberOfFollows].follower,MAX_USERNAME,"%s",follower);
    snprintf(follows[numberOfFollows].followee,MAX_USERNAME,"%s",followee);
    ++numberOfFollows;
    result=1;
    nowFollowing=1;
  }

  if (result) { saveFollowsUnlocked(); }
  pthread_mutex_unlock(&followsLock);

  //Gaining a follower is news ; losing one quietly is not..
  if (nowFollowing) { notificationAdd(followee,follower,NOTIFICATION_FOLLOW,0); }
  return result;
}


unsigned int followsCountFollowing(const char * username)
{
  pthread_mutex_lock(&followsLock);
  unsigned int i=0,count=0;
  for (i=0; i<numberOfFollows; i++) { if (strcmp(follows[i].follower,username)==0) { ++count; } }
  pthread_mutex_unlock(&followsLock);
  return count;
}


unsigned int followsCountFollowers(const char * username)
{
  pthread_mutex_lock(&followsLock);
  unsigned int i=0,count=0;
  for (i=0; i<numberOfFollows; i++) { if (strcmp(follows[i].followee,username)==0) { ++count; } }
  pthread_mutex_unlock(&followsLock);
  return count;
}


void * follow_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char follower[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,follower,sizeof(follower)) ) { socialServeLoginRequired(rqst); return 0; }

  //The directory and a wall both follow from a button , and each wants to land back where it was..
  int cameFromDirectory = _POSTexists(rqst,"dir");
  char backUser[MAX_USERNAME]={0};
  _POSTcpy(rqst,"back",backUser,sizeof(backUser));

  char followee[MAX_USERNAME]={0};
  if ( ( socialPostedCSRFIsValid(rqst) ) &&
       ( _POSTcpy(rqst,"user",followee,sizeof(followee)) ) &&
       ( uadbWeb_isValidUsername(followee) ) &&
       ( uadbWeb_userAccountExists(uadb,followee) ) )
  {
    followsToggle(follower,followee);
  }

  if (cameFromDirectory)
  {
    snprintf(rqst->content,rqst->MAXcontentSize,
             "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
             "<meta http-equiv=\"refresh\" content=\"0; url=people.html\"></head><body></body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  socialRedirectBack(rqst,backUser);
  return 0;
}


void * people_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char viewer[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,viewer,sizeof(viewer)) ) { socialServeLoginRequired(rqst); return 0; }

  char csrfToken[MAX_CSRF_TOKEN]={0};
  if ( ! AmmServer_GenerateCSRFToken(rqst,csrfToken,sizeof(csrfToken)) )
    { snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Could not issue a security token.</body></html>"); rqst->contentSize=strlen(rqst->content); return 0; }

  char * people=(char*) malloc(PEOPLE_RENDER_BUFFER_SIZE);
  if (people==0) { return 0; }
  people[0]=0;

  unsigned int position=0;
  char row[2048]={0};
  char escapedName[MAX_ESCAPED_USERNAME]={0};

  //The account database is the directory - there is no separate list of people to keep in step with it..
  unsigned int i=0;
  for (i=0; i<uadb->userListSize; i++)
  {
    const char * name=uadb->userList[i].username;
    AmmServer_HTMLEscape(name,escapedName,sizeof(escapedName));

    char action[768]={0};
    if (strcmp(name,viewer)==0)
    {
      snprintf(action,sizeof(action),"<span class=\"you\">this is you</span>");
    } else
    {
      int alreadyFollowing=followsIsFollowing(viewer,name);
      snprintf(action,sizeof(action),
               "<form method=\"post\" enctype=\"multipart/form-data\" action=\"follow.html\">"
                "<input type=\"hidden\" name=\"csrf\" value=\"%s\">"
                "<input type=\"hidden\" name=\"user\" value=\"%s\">"
                "<input type=\"hidden\" name=\"dir\" value=\"1\">"
                "<button type=\"submit\" class=\"%s\">%s</button>"
               "</form>",
               csrfToken,escapedName,
               alreadyFollowing ? "follow following" : "follow",
               alreadyFollowing ? "Following" : "Follow");
    }

    snprintf(row,sizeof(row),
             "<div class=\"personrow\">"
              "<div><a href=\"profile.html?u=%s\">%s</a>"
               "<div class=\"personmeta\">%u followers &middot; following %u</div></div>"
              "<div>%s</div>"
             "</div>",
             escapedName,escapedName,
             followsCountFollowers(name),followsCountFollowing(name),
             action);
    socialAppendChunk(people,PEOPLE_RENDER_BUFFER_SIZE,&position,row);
  }

  if (position==0) { socialAppendChunk(people,PEOPLE_RENDER_BUFFER_SIZE,&position,"<div class=\"notice\">Nobody has signed up yet.</div>"); }

  char escapedViewer[MAX_ESCAPED_USERNAME]={0};
  AmmServer_HTMLEscape(viewer,escapedViewer,sizeof(escapedViewer));

  char topbar[1024]={0};
  socialRenderTopbar(topbar,sizeof(topbar),escapedViewer,csrfToken,notificationCountUnseen(viewer));

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>People</title>"
           "<link rel='stylesheet' type='text/css' href='social.css'></head><body>"
           "%s<div class=\"page\"><div class=\"wall\"><h2 class=\"profiletitle\">People</h2>%s</div></div>"
           "</body></html>",
           topbar,people);
  rqst->contentSize=strlen(rqst->content);

  free(people);
  return 0;
}
