#include "media.h"
#include "../../AmmServerlib/tools/http_tools.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

/*Has to live under the service's web root : the framework refuses to stream a file that resolves outside it
  ( TransmitFileToSocket ) , which is the same reason HabChan keeps its attachments inside its own data root.
  Being under the root also means these are served as ordinary static files , with no handler of our own..*/
#define MEDIA_DIRECTORY "src/Services/Social/res/uploads/"
/*12 random bytes is plenty to make a name unguessable , which is what keeps a picture sent in a private
  message from being findable by anyone who did not receive it..*/
#define MEDIA_NAME_RANDOM_BYTES 12


int initializeMedia()
{
  mkdir(MEDIA_DIRECTORY,0755); //Already there is the normal case , not an error..
  return 1;
}


/*What a file actually is , decided from its leading bytes rather than from whatever name or type the client
  claimed - the extension chosen here is also what decides the content type it gets served back with..*/
static int detectMediaType(const char * bytes,unsigned int size,char * outExtension,unsigned int outExtensionSize)
{
  if ( (bytes==0) || (outExtension==0) ) { return 0; }

  if ( (size>=3) && ((unsigned char)bytes[0]==0xFF) && ((unsigned char)bytes[1]==0xD8) && ((unsigned char)bytes[2]==0xFF) )
    { snprintf(outExtension,outExtensionSize,"jpg"); return 1; }

  if ( (size>=8) && (memcmp(bytes,"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",8)==0) )
    { snprintf(outExtension,outExtensionSize,"png"); return 1; }

  if ( (size>=6) && ( (memcmp(bytes,"GIF87a",6)==0) || (memcmp(bytes,"GIF89a",6)==0) ) )
    { snprintf(outExtension,outExtensionSize,"gif"); return 1; }

  //RIFF....WAVE - the four size bytes in between are not part of the signature..
  if ( (size>=12) && (memcmp(bytes,"RIFF",4)==0) && (memcmp(bytes+8,"WAVE",4)==0) )
    { snprintf(outExtension,outExtensionSize,"wav"); return 1; }

  if ( (size>=4) && (memcmp(bytes,"OggS",4)==0) )
    { snprintf(outExtension,outExtensionSize,"ogg"); return 1; }

  //Either an ID3 tagged mp3 , or a bare MPEG audio frame ( 11 set bits , then the layer/bitrate nibbles )..
  if ( (size>=3) && (memcmp(bytes,"ID3",3)==0) )
    { snprintf(outExtension,outExtensionSize,"mp3"); return 1; }
  if ( (size>=2) && ((unsigned char)bytes[0]==0xFF) && ( ((unsigned char)bytes[1] & 0xE0) == 0xE0 ) )
    { snprintf(outExtension,outExtensionSize,"mp3"); return 1; }

  return 0;
}


int mediaStore(const char * bytes,unsigned int size,char * outStoredName,unsigned int outStoredNameSize)
{
  if ( (bytes==0) || (size==0) || (outStoredName==0) ) { return 0; }
  if (size>MAX_MEDIA_SIZE) { AmmServer_Warning("Refusing a %u byte upload , over the %u byte limit",size,MAX_MEDIA_SIZE); return 0; }

  char extension[8]={0};
  if ( ! detectMediaType(bytes,size,extension,sizeof(extension)) )
    { AmmServer_Warning("Refusing an upload that is not a supported image or sound"); return 0; }

  char token[32]={0};
  if ( ! AmmServer_GenerateSecureToken(token,sizeof(token),MEDIA_NAME_RANDOM_BYTES) ) { return 0; }

  char storedName[MAX_MEDIA_NAME]={0};
  snprintf(storedName,sizeof(storedName),"%s.%s",token,extension);

  char path[MAX_FILE_PATH]={0};
  snprintf(path,sizeof(path),"%s%s",MEDIA_DIRECTORY,storedName);

  if ( ! AmmServer_WriteFileFromMemory(path,bytes,size) )
    { AmmServer_Warning("Could not write an upload to %s",path); return 0; }

  snprintf(outStoredName,outStoredNameSize,"%s",storedName);
  return 1;
}


int mediaStoreFromRequest(struct AmmServer_DynamicRequest * rqst,const char * fieldName,char * outStoredName,unsigned int outStoredNameSize)
{
  unsigned int size=0;
  const char * bytes=_FILES(rqst,fieldName,VALUE,&size);

  //An untouched file input still sends an empty part , which is simply "no attachment" rather than a failure..
  if ( (bytes==0) || (size==0) ) { return 0; }

  return mediaStore(bytes,size,outStoredName,outStoredNameSize);
}


/*A stored name is ours , not the client's , so this only has to confirm nothing else got in : the base64url
  token characters , one dot , and a lowercase extension. No dots in a row and no slash means it can never
  reach out of the upload directory..*/
static int mediaIsValidName(const char * name)
{
  if (name==0) { return 0; }

  unsigned int length=strlen(name);
  if ( (length<3) || (length>=MAX_MEDIA_NAME) ) { return 0; }

  const char * dot=strchr(name,'.');
  if ( (dot==0) || (dot==name) || (strchr(dot+1,'.')!=0) ) { return 0; }

  unsigned int i=0;
  for (i=0; i<length; i++)
  {
    if (name[i]=='.') { continue; }
    if ( ! ( isalnum((unsigned char)name[i]) || (name[i]=='_') || (name[i]=='-') ) ) { return 0; }
  }

  return 1;
}


int mediaDelete(const char * storedName)
{
  if ( ! mediaIsValidName(storedName) ) { return 0; }

  char path[MAX_FILE_PATH]={0};
  snprintf(path,sizeof(path),"%s%s",MEDIA_DIRECTORY,storedName);

  return (remove(path)==0);
}


void mediaRenderTag(const char * storedName,char * output,unsigned int outputSize)
{
  output[0]=0;
  if ( ! mediaIsValidName(storedName) ) { return; }

  const char * extension=strchr(storedName,'.')+1;

  if ( (strcmp(extension,"mp3")==0) || (strcmp(extension,"wav")==0) || (strcmp(extension,"ogg")==0) )
    { snprintf(output,outputSize,"<audio class=\"postmedia\" controls preload=\"none\" src=\"uploads/%s\"></audio>",storedName); }
  else
    { snprintf(output,outputSize,"<a href=\"uploads/%s\"><img class=\"postmedia\" src=\"uploads/%s\"></a>",storedName,storedName); }
}
