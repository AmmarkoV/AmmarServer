#include "notifications.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define NOTIFICATIONS_RENDER_BUFFER_SIZE (256*1024)
#define MAX_NOTIFICATIONS_RENDERED 50

struct socialNotification
{
  char recipient[MAX_USERNAME];
  char actor[MAX_USERNAME];
  char wall[MAX_USERNAME];
  unsigned int type;
  unsigned int seen;
  unsigned long timestamp;
};

static struct socialNotification notifications[MAX_NOTIFICATIONS];
static unsigned int numberOfNotifications=0;
static char notificationsFilename[MAX_FILE_PATH]={0};

/*Guards everything above. The post store and the follow table both record notifications while holding their
  own locks , so nothing in here may ever call back into either of them - which is exactly why a notification
  carries the wall it happened on rather than looking the post up when it is drawn..*/
static pthread_mutex_t notificationsLock=PTHREAD_MUTEX_INITIALIZER;


static char * nextField(char ** cursor)
{
  char * field=*cursor;
  if (field==0) { return 0; }

  char * tab=strchr(field,'\t');
  if (tab!=0) { *tab=0; *cursor=tab+1; } else { *cursor=0; }
  return field;
}


static int saveNotificationsUnlocked()
{
  if (notificationsFilename[0]==0) { return 0; }

  FILE * fp=fopen(notificationsFilename,"w");
  if (fp==0) { AmmServer_Warning("Could not write notifications to %s",notificationsFilename); return 0; }

  unsigned int i=0;
  for (i=0; i<numberOfNotifications; i++)
  {
    fprintf(fp,"N\t%s\t%s\t%u\t%u\t%lu\t%s\n",
            notifications[i].recipient,notifications[i].actor,notifications[i].type,
            notifications[i].seen,notifications[i].timestamp,notifications[i].wall);
  }

  fclose(fp);
  return 1;
}


int loadNotifications(const char * filename)
{
  if (filename==0) { return 0; }
  snprintf(notificationsFilename,sizeof(notificationsFilename),"%s",filename);

  FILE * fp=fopen(filename,"r");
  if (fp==0) { AmmServer_Warning("No notifications yet ( no %s )",filename); return 1; }

  char line[1024]={0};
  while ( fgets(line,sizeof(line),fp) != 0 )
  {
    unsigned int lineLength=strlen(line);
    while ( (lineLength>0) && ( (line[lineLength-1]=='\n') || (line[lineLength-1]=='\r') ) ) { line[--lineLength]=0; }
    if (lineLength<10) { continue; }

    char * cursor=line;
    if (strcmp(nextField(&cursor),"N")!=0) { continue; }

    char * recipient = nextField(&cursor);
    char * actor     = nextField(&cursor);
    char * type      = nextField(&cursor);
    char * seen      = nextField(&cursor);
    char * timestamp = nextField(&cursor);
    char * wall      = cursor; //May legitimately be empty , which is a follow rather than a post
    if ( (recipient==0) || (actor==0) || (type==0) || (seen==0) || (timestamp==0) || (wall==0) ) { continue; }
    if (numberOfNotifications>=MAX_NOTIFICATIONS)                                               { continue; }
    if ( ( ! uadbWeb_isValidUsername(recipient) ) || ( ! uadbWeb_isValidUsername(actor) ) )      { continue; }

    struct socialNotification * notification=&notifications[numberOfNotifications++];
    memset(notification,0,sizeof(struct socialNotification));
    snprintf(notification->recipient,MAX_USERNAME,"%.*s",MAX_USERNAME-1,recipient);
    snprintf(notification->actor,MAX_USERNAME,"%.*s",MAX_USERNAME-1,actor);
    snprintf(notification->wall,MAX_USERNAME,"%.*s",MAX_USERNAME-1,wall);
    notification->type      = (unsigned int)  atoi(type);
    notification->seen      = (unsigned int)  atoi(seen);
    notification->timestamp = (unsigned long) atol(timestamp);
    if (notification->type>=NOTIFICATION_TYPES) { --numberOfNotifications; }
  }

  fclose(fp);
  AmmServer_Success("Loaded %u notifications from %s",numberOfNotifications,filename);
  return 1;
}


int unloadNotifications()
{
  pthread_mutex_lock(&notificationsLock);
  int result=saveNotificationsUnlocked();
  numberOfNotifications=0;
  pthread_mutex_unlock(&notificationsLock);
  return result;
}


int notificationAdd(const char * recipient,const char * actor,unsigned int type,const char * wall)
{
  if ( (recipient==0) || (actor==0) || (recipient[0]==0) || (actor[0]==0) ) { return 0; }
  if (strcmp(recipient,actor)==0) { return 0; } //Your own doing is not news to you
  if (type>=NOTIFICATION_TYPES)   { return 0; }

  pthread_mutex_lock(&notificationsLock);

  //A full list makes room by dropping its oldest , same as the wall does..
  if (numberOfNotifications>=MAX_NOTIFICATIONS)
  {
    memmove(&notifications[0],&notifications[1],sizeof(struct socialNotification)*(MAX_NOTIFICATIONS-1));
    --numberOfNotifications;
  }

  struct socialNotification * notification=&notifications[numberOfNotifications++];
  memset(notification,0,sizeof(struct socialNotification));
  snprintf(notification->recipient,MAX_USERNAME,"%s",recipient);
  snprintf(notification->actor,MAX_USERNAME,"%s",actor);
  if (wall!=0) { snprintf(notification->wall,MAX_USERNAME,"%s",wall); }
  notification->type=type;
  notification->seen=0;
  notification->timestamp=(unsigned long) time(0);

  saveNotificationsUnlocked();
  pthread_mutex_unlock(&notificationsLock);
  return 1;
}


unsigned int notificationCountUnseen(const char * username)
{
  if (username==0) { return 0; }

  pthread_mutex_lock(&notificationsLock);
  unsigned int i=0,count=0;
  for (i=0; i<numberOfNotifications; i++)
  {
    if ( (!notifications[i].seen) && (strcmp(notifications[i].recipient,username)==0) ) { ++count; }
  }
  pthread_mutex_unlock(&notificationsLock);
  return count;
}


int notificationMarkAllSeen(const char * username)
{
  if (username==0) { return 0; }

  pthread_mutex_lock(&notificationsLock);
  unsigned int i=0,changed=0;
  for (i=0; i<numberOfNotifications; i++)
  {
    if ( (!notifications[i].seen) && (strcmp(notifications[i].recipient,username)==0) ) { notifications[i].seen=1; ++changed; }
  }
  if (changed) { saveNotificationsUnlocked(); }
  pthread_mutex_unlock(&notificationsLock);
  return (int) changed;
}


static const char * notificationSentence(unsigned int type)
{
  switch (type)
  {
    case NOTIFICATION_COMMENT  : return "commented on your post";
    case NOTIFICATION_LIKE     : return "liked your post";
    case NOTIFICATION_WALLPOST : return "wrote on your wall";
    case NOTIFICATION_FOLLOW   : return "started following you";
  };
  return "did something";
}


void * notifications_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char viewer[MAX_USERNAME]={0};
  if ( ! AmmServer_CurrentUsername(rqst,viewer,sizeof(viewer)) ) { socialServeLoginRequired(rqst); return 0; }

  char csrfToken[MAX_CSRF_TOKEN]={0};
  if ( ! AmmServer_GenerateCSRFToken(rqst,csrfToken,sizeof(csrfToken)) )
    { snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Could not issue a security token.</body></html>"); rqst->contentSize=strlen(rqst->content); return 0; }

  char * list=(char*) malloc(NOTIFICATIONS_RENDER_BUFFER_SIZE);
  if (list==0) { return 0; }
  list[0]=0;

  unsigned int position=0,rendered=0;
  char row[1024]={0};
  char escapedActor[MAX_ESCAPED_USERNAME]={0};
  char escapedWall[MAX_ESCAPED_USERNAME]={0};
  char when[64]={0};

  pthread_mutex_lock(&notificationsLock);

  unsigned int i=numberOfNotifications;
  while ( (i>0) && (rendered<MAX_NOTIFICATIONS_RENDERED) )
  {
    struct socialNotification * notification=&notifications[--i];
    if (strcmp(notification->recipient,viewer)!=0) { continue; }
    ++rendered;

    AmmServer_HTMLEscape(notification->actor,escapedActor,sizeof(escapedActor));
    //A post notification leads back to the wall it happened on , a follow to whoever did it..
    AmmServer_HTMLEscape( (notification->wall[0]!=0) ? notification->wall : notification->actor ,
                          escapedWall,sizeof(escapedWall));

    time_t rawTime=(time_t) notification->timestamp;
    struct tm brokenDownTime;
    if ( localtime_r(&rawTime,&brokenDownTime) != 0 ) { strftime(when,sizeof(when),"%d %b %Y %H:%M",&brokenDownTime); }
    else                                             { snprintf(when,sizeof(when),"unknown time"); }

    snprintf(row,sizeof(row),
             "<a class=\"notification %s\" href=\"profile.html?u=%s\">"
              "<b>%s</b> %s<span class=\"when\">%s</span>"
             "</a>",
             notification->seen ? "seen" : "unseen",
             escapedWall,escapedActor,notificationSentence(notification->type),when);
    socialAppendChunk(list,NOTIFICATIONS_RENDER_BUFFER_SIZE,&position,row);
  }

  pthread_mutex_unlock(&notificationsLock);

  if (rendered==0) { socialAppendChunk(list,NOTIFICATIONS_RENDER_BUFFER_SIZE,&position,"<div class=\"notice\">Nothing has happened yet.</div>"); }

  char escapedViewer[MAX_ESCAPED_USERNAME]={0};
  AmmServer_HTMLEscape(viewer,escapedViewer,sizeof(escapedViewer));

  //The badge is drawn before they are marked , so this page still shows what was new when it was opened..
  char topbar[1024]={0};
  socialRenderTopbar(topbar,sizeof(topbar),escapedViewer,csrfToken,notificationCountUnseen(viewer));

  snprintf(rqst->content,rqst->MAXcontentSize,
           "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>Notifications</title>"
           "<link rel='stylesheet' type='text/css' href='social.css'></head><body>"
           "%s<div class=\"page\"><div class=\"wall\"><h2 class=\"profiletitle\">Notifications</h2>%s</div></div>"
           "</body></html>",
           topbar,list);
  rqst->contentSize=strlen(rqst->content);

  free(list);

  //Opening the page is what reads them..
  notificationMarkAllSeen(viewer);
  return 0;
}
