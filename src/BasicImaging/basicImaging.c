#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> //strcasecmp

#include "basicImaging.h"
#include "codecs/codecs.h"
#include "codecs/jpgInput.h"
#include "codecs/pngInput.h"
#include "codecs/pzpInput.h"

int BasicImaging_HasJPEG() { return USE_JPG_FILES; }
int BasicImaging_HasPNG()  { return USE_PNG_FILES; }
int BasicImaging_HasPZP()  { return USE_PZP_FILES; }


static int hasSuffixCaseInsensitive(const char * s,const char * suffix)
{
  if ( (s==0) || (suffix==0) ) { return 0; }
  size_t ls=strlen(s), lf=strlen(suffix);
  if ( (lf==0) || (lf>ls) ) { return 0; }
  return (strcasecmp(s+ls-lf,suffix)==0);
}


struct Image * BasicImaging_Load(const char * filename)
{
  if (filename==0) { return 0; }

  struct Image * img = readImage(filename,NO_CODEC,0);
  if ( (img!=0) && (img->pixels==0) )
  {
    //Never hand back a half-populated image
    destroyImage(img);
    return 0;
  }
  return img;
}


struct Image * BasicImaging_New(unsigned int width,unsigned int height,unsigned int channels)
{
  if ( (width==0) || (height==0) || (channels==0) ) { return 0; }

  struct Image * img = createImage(width,height,channels,8);
  if (img!=0) { img->image_size = width*height*channels; } //createImage()/populateImage() never set this themselves
  return img;
}


void BasicImaging_Free(struct Image ** img)
{
  if ( (img==0) || (*img==0) ) { return; }
  destroyImage(*img);
  *img=0;
}


int BasicImaging_SaveJPEG(struct Image * img,const char * filename,int quality)
{
  if ( (img==0) || (img->pixels==0) || (filename==0) ) { return 0; }

  #if USE_JPG_FILES
    if (quality<1)   { quality=1;   }
    if (quality>100) { quality=100; }
    return WriteJPEGInternal(filename,img,0,0,quality);
  #else
    fprintf(stderr,"BasicImaging: JPEG support is not compiled in ( libjpeg/jpeglib.h was not found at build time ) , cannot save %s\n",filename);
    return 0;
  #endif
}


int BasicImaging_SavePNG(struct Image * img,const char * filename)
{
  if ( (img==0) || (img->pixels==0) || (filename==0) ) { return 0; }

  #if USE_PNG_FILES
    return WritePNG(filename,img);
  #else
    fprintf(stderr,"BasicImaging: PNG support is not compiled in ( libpng/png.h was not found at build time ) , cannot save %s\n",filename);
    return 0;
  #endif
}


int BasicImaging_SavePZP(struct Image * img,const char * filename)
{
  if ( (img==0) || (img->pixels==0) || (filename==0) ) { return 0; }

  #if USE_PZP_FILES
    return WritePZP(filename,img);
  #else
    fprintf(stderr,"BasicImaging: PZP support is not compiled in ( libzstd/liblz4 or their headers were not found at build time ) , cannot save %s\n",filename);
    return 0;
  #endif
}


int BasicImaging_Save(struct Image * img,const char * filename,int jpegQuality)
{
  if (filename==0) { return 0; }

  if ( hasSuffixCaseInsensitive(filename,".jpg") || hasSuffixCaseInsensitive(filename,".jpeg") )
  {
    return BasicImaging_SaveJPEG(img,filename,jpegQuality);
  }
  if ( hasSuffixCaseInsensitive(filename,".png") )
  {
    return BasicImaging_SavePNG(img,filename);
  }
  if ( hasSuffixCaseInsensitive(filename,".pzp") )
  {
    return BasicImaging_SavePZP(img,filename);
  }

  fprintf(stderr,"BasicImaging: cannot infer an output codec from filename `%s` ( expected .jpg/.jpeg/.png/.pzp )\n",filename);
  return 0;
}


static int copyFileBytes(const char * srcPath,const char * dstPath)
{
  if ( (srcPath==0) || (dstPath==0) ) { return 0; }

  FILE * in = fopen(srcPath,"rb");
  if (in==0) { return 0; }

  FILE * out = fopen(dstPath,"wb");
  if (out==0) { fclose(in); return 0; }

  unsigned char buffer[65536];
  size_t n;
  int ok=1;
  while ( (n=fread(buffer,1,sizeof(buffer),in)) > 0 )
  {
    if (fwrite(buffer,1,n,out)!=n) { ok=0; break; }
  }

  fclose(in);
  fclose(out);
  return ok;
}


int BasicImaging_ThumbnailFile(const char * inputFilename,const char * outputFilename,unsigned int maxDimension,int jpegQuality)
{
  if ( (inputFilename==0) || (outputFilename==0) ) { return 0; }

  struct Image * img = BasicImaging_Load(inputFilename);
  if (img==0)
  {
    copyFileBytes(inputFilename,outputFilename);
    return 0;
  }

  struct Image * thumb = BasicImaging_Thumbnail(img,maxDimension);
  BasicImaging_Free(&img);
  if (thumb==0)
  {
    copyFileBytes(inputFilename,outputFilename);
    return 0;
  }

  int saved = BasicImaging_Save(thumb,outputFilename,jpegQuality);
  BasicImaging_Free(&thumb);

  if (!saved)
  {
    copyFileBytes(inputFilename,outputFilename);
    return 0;
  }

  return 1;
}
