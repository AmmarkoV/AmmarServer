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


static unsigned int exifGet16(const unsigned char * p,int littleEndian)
{
  if (littleEndian) { return (unsigned int)p[0] | ((unsigned int)p[1]<<8); }
  return ((unsigned int)p[0]<<8) | (unsigned int)p[1];
}

static unsigned long exifGet32(const unsigned char * p,int littleEndian)
{
  if (littleEndian)
  {
    return (unsigned long)p[0] | ((unsigned long)p[1]<<8) | ((unsigned long)p[2]<<16) | ((unsigned long)p[3]<<24);
  }
  return ((unsigned long)p[0]<<24) | ((unsigned long)p[1]<<16) | ((unsigned long)p[2]<<8) | (unsigned long)p[3];
}

/* Walk the IFD0 entries of a TIFF block ( the bytes right after an APP1 "Exif\0\0" payload
 * header ) looking for tag 0x0112 Orientation ( type SHORT , count 1 ). Fully bounds-checked :
 * any malformed/truncated data yields 0 , never a crash.
 */
static int parseEXIFTIFFOrientation(const unsigned char * tiff,unsigned long size)
{
  if (size<8) { return 0; }

  int littleEndian = 0;
  if ( (tiff[0]=='I') && (tiff[1]=='I') ) { littleEndian = 1; }
  else if ( (tiff[0]=='M') && (tiff[1]=='M') ) { littleEndian = 0; }
  else { return 0; } //not a TIFF byte-order marker

  if ( exifGet16(tiff+2,littleEndian)!=42 ) { return 0; } //TIFF magic

  unsigned long ifd0Offset = exifGet32(tiff+4,littleEndian);
  if (ifd0Offset > size-2) { return 0; } //IFD0 outside the block ( also catches size<2 )

  unsigned long entryCount = exifGet16(tiff+ifd0Offset,littleEndian);
  unsigned long pos = ifd0Offset+2;
  for (unsigned long i=0; i<entryCount; i++)
  {
    if (pos > size-12) { return 0; } //truncated entry
    unsigned int tag   = exifGet16(tiff+pos,littleEndian);
    unsigned int type  = exifGet16(tiff+pos+2,littleEndian);
    unsigned long count = exifGet32(tiff+pos+4,littleEndian);
    unsigned long value = exifGet32(tiff+pos+8,littleEndian);
    if ( (tag==0x0112) && (type==3/*SHORT*/) && (count==1) )
    {
      if ( (value>=1) && (value<=8) ) { return (int) value; }
      return 0;
    }
    pos += 12;
  }
  return 0;
}

/* Open the file and return its EXIF Orientation tag ( 1-8 ) or 0 when the file isn't a JPEG ,
 * has no Exif APP1 segment , or is malformed. The JPEG segment walk stops at SOS/EOI since the
 * image data itself can contain bytes that merely look like markers. Never crashes.
 */
static int readEXIFOrientation(const char * filename)
{
  if (filename==0) { return 0; }

  FILE * file = fopen(filename,"rb");
  if (file==0) { return 0; }

  int orientation = 0;
  if ( fseek(file,0,SEEK_END)==0 )
  {
    long fileSizeL = ftell(file);
    if ( (fileSizeL>=4) && (fseek(file,0,SEEK_SET)==0) )
    {
      unsigned long fileSize = (unsigned long) fileSizeL;
      unsigned char * data = (unsigned char *) malloc(fileSize);
      if (data!=0)
      {
        if ( fread(data,1,fileSize,file)==fileSize )
        {
          if ( (data[0]==0xFF) && (data[1]==0xD8) ) //JPEG SOI
          {
            unsigned long offset = 2;
            while (offset < fileSize)
            {
              if (data[offset]!=0xFF) { break; } //garbage between segments : malformed , give up
              while ( (offset<fileSize) && (data[offset]==0xFF) ) { offset++; } //fill bytes / padding
              if (offset>=fileSize) { break; }

              unsigned char marker = data[offset++];
              if (marker==0xD9) { break; } //EOI - no EXIF
              if (marker==0xDA) { break; } //SOS - image data begins , EXIF must precede it
              if ( (marker==0x01) || ( (marker>=0xD0) && (marker<=0xD7) ) ) { continue; } //TEM / RSTn : no length field

              if (offset > fileSize-2) { break; } //truncated segment header
              unsigned long segmentLen = ((unsigned long)data[offset]<<8) | data[offset+1];
              if (segmentLen<2) { break; } //malformed length
              if (segmentLen > fileSize-offset) { break; } //truncated segment body

              if (marker==0xE1) //APP1
              {
                unsigned long payloadStart = offset+2;
                unsigned long payloadLen = segmentLen-2;
                if ( (payloadLen>=8) &&
                     (data[payloadStart+0]=='E') && (data[payloadStart+1]=='x') &&
                     (data[payloadStart+2]=='i') && (data[payloadStart+3]=='f') &&
                     (data[payloadStart+4]==0)    && (data[payloadStart+5]==0) )
                {
                  orientation = parseEXIFTIFFOrientation(data+payloadStart+6,payloadLen-6);
                  break;
                }
              }
              offset += segmentLen;
            }
          }
        }
        free(data);
      }
    }
  }
  fclose(file);
  return orientation;
}

/* Permute pixels per the standard EXIF Orientation table ( see the migration notes ). The new
 * image is freshly allocated ; the caller keeps ownership of the original and frees it if this
 * succeeds. Returns 0 ( caller keeps the unrotated image ) on bad input / allocation failure.
 */
static struct Image * rotateImageForOrientation(struct Image * img,int orientation)
{
  if ( (img==0) || (img->pixels==0) || (orientation<2) || (orientation>8) ) { return 0; }
  if ( (img->width==0) || (img->height==0) || (img->channels==0) ) { return 0; }

  int swapDimensions = (orientation>=5);
  unsigned int newWidth  = swapDimensions ? img->height : img->width;
  unsigned int newHeight = swapDimensions ? img->width  : img->height;

  struct Image * out = BasicImaging_New(newWidth,newHeight,img->channels);
  if (out==0) { return 0; }

  unsigned int bytesPerPixel = img->channels; //8 bits per channel throughout this library
  for (unsigned int y=0; y<newHeight; y++)
  {
    for (unsigned int x=0; x<newWidth; x++)
    {
      unsigned int srcX=0, srcY=0;
      switch (orientation)
      {
        case 2: srcX = img->width-1-x;   srcY = y;              break; //flip horizontal
        case 3: srcX = img->width-1-x;   srcY = img->height-1-y; break; //rotate 180
        case 4: srcX = x;                srcY = img->height-1-y; break; //flip vertical
        case 5: srcX = y;                srcY = x;              break; //transpose
        case 6: srcX = y;                srcY = img->height-1-x; break; //rotate 90 CW
        case 7: srcX = img->width-1-y;   srcY = img->height-1-x; break; //transverse
        case 8: srcX = img->width-1-y;   srcY = x;              break; //rotate 270 CW
      }
      memcpy(out->pixels + ( (unsigned long)y*newWidth + x )*bytesPerPixel,
             img->pixels + ( (unsigned long)srcY*img->width + srcX )*bytesPerPixel,
             bytesPerPixel);
    }
  }
  return out;
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

  //Auto-orient per the EXIF Orientation tag ( JPEGs only ; other formats have no EXIF and the
  //parser above just returns 0 ). On rotation-allocation failure the unrotated decode is kept.
  if (img!=0)
  {
    int orientation = readEXIFOrientation(filename);
    if (orientation>=2)
    {
      struct Image * rotated = rotateImageForOrientation(img,orientation);
      if (rotated!=0)
      {
        destroyImage(img);
        img = rotated;
      }
    }
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


int BasicImaging_SaveJPEGToMemory(struct Image * img,unsigned char * target,unsigned long targetMaxSize,unsigned long * bytesWritten,unsigned int quality)
{
  if ( (img==0) || (img->pixels==0) || (target==0) || (bytesWritten==0) || (targetMaxSize==0) ) { return 0; }

  #if USE_JPG_FILES
    if (quality<1)   { quality=1;   }
    if (quality>100) { quality=100; }
    *bytesWritten = targetMaxSize; //WriteJPEGInternal consumes the buffer size down to the bytes actually used
    return WriteJPEGInternal(0,img,(char*)target,bytesWritten,(int)quality);
  #else
    fprintf(stderr,"BasicImaging: JPEG support is not compiled in ( libjpeg/jpeglib.h was not found at build time ) , cannot encode to memory\n");
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


/* Channel-generic crop : copy a fully-inside region of src into a freshly allocated image , one
 * row-memcpy at a time. Deliberately NOT codecs' createImageBitBlt() ( that one is hardcoded to 3
 * channels and spams stderr on every call ).
 */
static struct Image * cropImageRegion(const struct Image * src,unsigned int x,unsigned int y,unsigned int w,unsigned int h)
{
  if ( (src==0) || (src->pixels==0) || (src->channels==0) ) { return 0; }
  if ( (x>src->width) || (y>src->height) ) { return 0; }
  if ( (w==0) || (h==0) ) { return 0; }
  if ( (w>src->width-x) || (h>src->height-y) ) { return 0; } //must fit fully inside

  struct Image * out = BasicImaging_New(w,h,src->channels);
  if (out==0) { return 0; }

  unsigned int bytesPerPixel = src->channels; //8 bits per channel throughout this library
  unsigned long rowBytes = (unsigned long)w * bytesPerPixel;
  for (unsigned int row=0; row<h; row++)
  {
    memcpy(out->pixels + (unsigned long)row * rowBytes,
           src->pixels + ( (unsigned long)(y+row) * src->width + x ) * bytesPerPixel,
           rowBytes);
  }
  return out;
}


struct Image * BasicImaging_CoverResize(const struct Image * img,unsigned int newWidth,unsigned int newHeight)
{
  if ( (img==0) || (img->pixels==0) || (img->width==0) || (img->height==0) || (newWidth==0) || (newHeight==0) ) { return 0; }

  //Uniform scale that makes the source COVER the target box ( fills it , never distorts , upscales
  //when the source is smaller ) - this reproduces ImageMagick's
  //"-resize WxH^ -gravity Center -extent WxH"
  double scaleX = (double)newWidth  / (double)img->width;
  double scaleY = (double)newHeight / (double)img->height;
  double scale  = (scaleX>scaleY) ? scaleX : scaleY;

  //Ceil the fractional intermediate dimensions so the crop below never comes up short
  unsigned int scaledW = (unsigned int)( (double)img->width  * scale + 0.999999 );
  unsigned int scaledH = (unsigned int)( (double)img->height * scale + 0.999999 );

  struct Image * resized = BasicImaging_Resize(img,scaledW,scaledH); //gets the SIMD paths
  if (resized==0) { return 0; }

  struct Image * out = cropImageRegion(resized,(resized->width-newWidth)/2,(resized->height-newHeight)/2,newWidth,newHeight);
  BasicImaging_Free(&resized);
  return out;
}
