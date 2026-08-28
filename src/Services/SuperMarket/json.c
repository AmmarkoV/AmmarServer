#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "json.h"

void jsonAppendEscaped(char * out,unsigned int outSize,const char * str)
{
  unsigned int oi=(unsigned int) strlen(out);
  unsigned int ii=0;
  if (oi+1>=outSize) { return; }
  out[oi++]='"';
  while ( (str[ii]!=0) && (oi+2<outSize) )
  {
    unsigned char c=(unsigned char) str[ii];
    switch (c)
    {
      case '"' : if (oi+2>=outSize) { goto done; } out[oi++]='\\'; out[oi++]='"';  break;
      case '\\': if (oi+2>=outSize) { goto done; } out[oi++]='\\'; out[oi++]='\\'; break;
      case '\n': if (oi+2>=outSize) { goto done; } out[oi++]='\\'; out[oi++]='n';  break;
      case '\r': if (oi+2>=outSize) { goto done; } out[oi++]='\\'; out[oi++]='r';  break;
      case '\t': if (oi+2>=outSize) { goto done; } out[oi++]='\\'; out[oi++]='t';  break;
      default:
        if (c<0x20)
        {
          if (oi+6>=outSize) { goto done; }
          oi+=(unsigned int) snprintf(out+oi,outSize-oi,"\\u%04x",c);
        } else
        {
          out[oi++]=(char)c; //UTF-8 continuation/ASCII bytes pass through unescaped ( raw UTF-8 body , charset=utf-8 )
        }
        break;
    }
    ++ii;
  }
  done:
  if (oi+1<outSize) { out[oi++]='"'; }
  out[oi]=0;
}

static unsigned int photoMTime(const char * path)
{
  struct stat st;
  if (stat(path,&st)!=0) { return 0; }
  return (unsigned int) st.st_mtime;
}

void buildCartJSON(const char * token,struct cart * cart,char * out,unsigned int outSize)
{
  if (outSize==0) { return; }
  out[0]=0;

  snprintf(out,outSize,"{\"items\":[");

  unsigned int i=0;
  for (i=0; i<cart->numberOfItems; i++)
  {
    struct item * it=&cart->items[i];
    char photoPath[512]={0};
    photoPathFor(token,it->id,photoPath,sizeof(photoPath));
    unsigned int mtime=photoMTime(photoPath);

    unsigned int len=(unsigned int) strlen(out);
    if (i>0) { snprintf(out+len,outSize-len,","); len=(unsigned int) strlen(out); }
    snprintf(out+len,outSize-len,"{\"id\":");
    jsonAppendEscaped(out,outSize,it->id);
    len=(unsigned int) strlen(out);
    snprintf(out+len,outSize-len,",\"n\":");
    jsonAppendEscaped(out,outSize,it->name);
    len=(unsigned int) strlen(out);
    snprintf(out+len,outSize-len,",\"q\":%u,\"c\":%u,\"img\":%u}",it->qty,it->checked?1:0,mtime);
  }

  unsigned int len=(unsigned int) strlen(out);
  snprintf(out+len,outSize-len,"],\"rev\":%u",cart->rev);

  if (cart->title[0]!=0)
  {
    len=(unsigned int) strlen(out);
    snprintf(out+len,outSize-len,",\"name\":");
    jsonAppendEscaped(out,outSize,cart->title);
  }

  char coverPath[512]={0};
  photoPathFor(token,"cover",coverPath,sizeof(coverPath));
  len=(unsigned int) strlen(out);
  snprintf(out+len,outSize-len,",\"cimg\":%u}",photoMTime(coverPath));
}
