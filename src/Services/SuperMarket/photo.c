#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "photo.h"
#include "json.h"
#include "../../BasicImaging/basicImaging.h"

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


//Determines the REAL type of an uploaded photo by sniffing its magic bytes rather than trusting the
//extension on the filename the browser sent us ( which is trivial to fake ) . Returns "jpg"/"png" or 0 -
//GIFs are deliberately rejected ( BasicImaging has no GIF codec , and ImageMagick used to take their first
//frame ). Mirrors HabChan's detectImageType() but works straight over the in-memory upload.
static const char * sniffImageType(const char * bytes,unsigned int size)
{
  if (bytes==0) { return 0; }

  if ( (size>=3) && ((unsigned char)bytes[0]==0xFF) && ((unsigned char)bytes[1]==0xD8) && ((unsigned char)bytes[2]==0xFF) )
  {
    return "jpg";
  }

  if ( (size>=8) && (memcmp(bytes,"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",8)==0) )
  {
    return "png";
  }

  return 0;
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

    //Sniff the upload's magic bytes : garbage uploads are rejected right here ( ImageMagick's convert used
    //to do this validation implicitly ). GIFs are now rejected too - approved change.
    const char * ext = sniffImageType(fileData,fileSize);
    if (ext==0)
    {
      err="Μη έγκυρη εικόνα";
    } else
    {
      //BasicImaging_Load() decodes by filename extension , so the scratch input needs the REAL extension of
      //what was sniffed ( the old bare .tmp suffix would never decode ).
      char tmpIn[512]={0};
      snprintf(tmpIn,sizeof(tmpIn),"%s/upload-%u-%d.%s",photosDirectory(),(unsigned int) getpid(),rand(),ext);
      AmmServer_WriteFileFromMemory(tmpIn,fileData,fileSize);

      //Decode ( EXIF auto-orient happens inside BasicImaging_Load ) , shrink-only fit to 1000px
      //( "-resize 1000x1000>" ) , re-encode as a quality-82 JPEG into a scratch output : success has to be
      //judged from a FRESH file this exact attempt produced , never from destPath , which may already hold
      //an older photo.
      char tmpOut[512]={0};
      snprintf(tmpOut,sizeof(tmpOut),"%s/convert-%u-%d.tmp.jpg",photosDirectory(),(unsigned int) getpid(),rand());

      struct Image * img = BasicImaging_Load(tmpIn);
      unlink(tmpIn);
      struct Image * thumb = (img!=0) ? BasicImaging_Thumbnail(img,1000) : 0;
      BasicImaging_Free(&img);
      int saved = (thumb!=0) ? BasicImaging_SaveJPEG(thumb,tmpOut,82) : 0;
      BasicImaging_Free(&thumb);

      if ( saved && (rename(tmpOut,destPath)==0) )
      {
        //photo stored ( tmpOut is now destPath )
      } else
      {
        unlink(tmpOut);
        err="Μη έγκυρη εικόνα";
      }
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
