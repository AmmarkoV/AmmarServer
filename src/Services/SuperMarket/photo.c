#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "photo.h"
#include "json.h"

//Splice {"error":"..."} into an already-built {"items":[...],...} JSON object , matching go.php's cart['error']
static void spliceError(char * buffer,unsigned int bufferSize,const char * message)
{
  unsigned int len=(unsigned int) strlen(buffer);
  if ( (len==0) || (buffer[len-1]!='}') ) { return; }
  buffer[len-1]=0;

  unsigned int cur=(unsigned int) strlen(buffer);
  snprintf(buffer+cur,bufferSize-cur,",\"error\":");
  jsonAppendEscaped(buffer,bufferSize,message);

  unsigned int end=(unsigned int) strlen(buffer);
  if (end+1<bufferSize) { buffer[end]='}'; buffer[end+1]=0; }
}


void * photoUpload_callback(struct AmmServer_DynamicRequest * rqst,const char * token)
{
  char rawID[64]={0};
  _POSTcpy(rqst,"id",rawID,sizeof(rawID));

  char id[ID_BUF_SIZE]={0};
  const char * err=0;

  if ( ! sanitizeItemID(rawID,id,sizeof(id)) ) { err="Άγνωστο προϊόν"; }

  unsigned int fileSize=0;
  const char * fileData=0;
  if (!err)
  {
    fileData=_FILES(rqst,"f",VALUE,&fileSize);
    if ( (fileData==0) || (fileSize==0) ) { err="Το ανέβασμα απέτυχε (πολύ μεγάλο αρχείο;)"; }
  }

  if (!err)
  {
    char destPath[512]={0};
    photoPathFor(token,id,destPath,sizeof(destPath));

    char tmpIn[512]={0};
    snprintf(tmpIn,sizeof(tmpIn),"%s/upload-%u-%d.tmp",photosDirectory(),(unsigned int) getpid(),rand());
    AmmServer_WriteFileFromMemory(tmpIn,fileData,fileSize);

    //Convert into a scratch output first : AmmServer_ExecuteCommandLine can't report convert's real exit
    //status ( it only reports whether popen() itself started ) , so success has to be judged from a FRESH
    //file this exact attempt produced , never from destPath , which may already hold an older photo.
    char tmpOut[512]={0};
    snprintf(tmpOut,sizeof(tmpOut),"%s/convert-%u-%d.tmp.jpg",photosDirectory(),(unsigned int) getpid(),rand());

    char command[1200]={0};
    snprintf(command,sizeof(command),
             "convert '%s[0]' -auto-orient -strip -resize '1000x1000>' -quality 82 'jpg:%s' >/dev/null 2>&1",
             tmpIn,tmpOut);
    char scratch[16]={0};
    AmmServer_ExecuteCommandLine(command,scratch,sizeof(scratch));
    unlink(tmpIn);

    if ( AmmServer_FileExists(tmpOut) )
    {
      rename(tmpOut,destPath);
    } else
    {
      err="Μη έγκυρη εικόνα";
    }
  }

  struct cart cart;
  if (err)
  {
    withCart(token,&cart,0); //failure : just re-read , no rev bump
  } else
  {
    struct cartActionParams params={0};
    params.action=ACTION_NOOP; //success : bump rev so other phones refetch , nothing else changes
    withCart(token,&cart,&params);
  }

  buildCartJSON(token,&cart,rqst->content,rqst->MAXcontentSize);
  if (err) { spliceError(rqst->content,rqst->MAXcontentSize,err); }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * photoStream_callback(struct AmmServer_DynamicRequest * rqst,const char * token,const char * rawImgID)
{
  char id[ID_BUF_SIZE]={0};
  if ( sanitizeItemID(rawImgID,id,sizeof(id)) )
  {
    char path[512]={0};
    photoPathFor(token,id,path,sizeof(path));
    if (AmmServer_FileExists(path))
    {
      AmmServer_DynamicRequestReturnFile(rqst,path);
      return 0;
    }
  }
  rqst->contentSize=0;
  return 0;
}
