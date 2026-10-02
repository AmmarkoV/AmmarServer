#ifndef MEDIA_H_INCLUDED
#define MEDIA_H_INCLUDED

#include "session.h"

/*The server refuses a POST header larger than 4MB ( DEFAULT_MAX_HTTP_POST_REQUEST_HEADER ) , so an upload has
  to stay comfortably under that with room to spare for the rest of the form..*/
#define MAX_MEDIA_SIZE (3*1024*1024)
/*A random 12 byte name in base64url is 16 characters , plus a ".jpeg" sized extension..*/
#define MAX_MEDIA_NAME 64

int initializeMedia();

/*Works out what was actually uploaded from its leading bytes ( never from the name the client claimed ) ,
  stores it under an unguessable name of its own , and hands that name back to be kept with the post or
  message it belongs to.
  @retval 1=Stored ( outStoredName holds the name ) , 0=Nothing uploaded / not a supported image or sound*/
int mediaStore(const char * bytes,unsigned int size,char * outStoredName,unsigned int outStoredNameSize);

/*Reads an uploaded file off a request and stores it - the whole upload path a callback needs..
  @retval 1=Stored , 0=No file was attached , or it was rejected*/
int mediaStoreFromRequest(struct AmmServer_DynamicRequest * rqst,const char * fieldName,char * outStoredName,unsigned int outStoredNameSize);

/*Removes a stored file - used when the post or message carrying it is deleted , so uploads don't outlive
  what they belonged to.
  @retval 1=Removed , 0=Nothing to remove / not a name we could have written*/
int mediaDelete(const char * storedName);

/*Renders the tag that plays or shows a stored file - an <img> for a picture , an <audio> for a sound. Uploads
  are served as ordinary static files out of the web root , so these point straight at uploads/<name>..*/
void mediaRenderTag(const char * storedName,char * output,unsigned int outputSize);

#endif // MEDIA_H_INCLUDED
