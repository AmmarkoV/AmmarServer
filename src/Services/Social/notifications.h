#ifndef NOTIFICATIONS_H_INCLUDED
#define NOTIFICATIONS_H_INCLUDED

#include "session.h"

#define MAX_NOTIFICATIONS 2048

enum socialNotificationType
{
  NOTIFICATION_COMMENT = 0,
  NOTIFICATION_LIKE,
  NOTIFICATION_WALLPOST,
  NOTIFICATION_FOLLOW,
  NOTIFICATION_TYPES
};

int loadNotifications(const char * filename);
int unloadNotifications();

/*Records that `actor` did something to `recipient`. `wall` is where it happened , so the notification can be
  followed back without looking the post up again later ( which is what keeps this module from ever having to
  reach back into the post store , and the two locks in a fixed order ) - empty for a follow , which leads to
  the actor's own wall instead. Doing something to yourself notifies nobody.*/
int notificationAdd(const char * recipient,const char * actor,unsigned int type,const char * wall);

unsigned int notificationCountUnseen(const char * username);
int notificationMarkAllSeen(const char * username);

void * notifications_callback(struct AmmServer_DynamicRequest  * rqst);

#endif // NOTIFICATIONS_H_INCLUDED
