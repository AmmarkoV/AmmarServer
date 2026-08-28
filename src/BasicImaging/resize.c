/** @file resize.c
* @brief BasicImaging_Resize() - the one call almost every Service actually needs : a fast, safe,
*        general-purpose image resize.
*
*        Algorithm : separable bilinear interpolation. Each destination pixel is a blend of the 4
*        nearest source pixels ( 2 taps horizontally x 2 taps vertically ). "Separable" here means the
*        horizontal and vertical sampling positions/weights are precomputed ONCE per destination column
*        / row ( O(newWidth+newHeight) ) rather than being recomputed from scratch for every one of the
*        newWidth*newHeight destination pixels - that table is then reused across every row ( for the
*        x table ) and is O(1) to look up per pixel.
*
*        The per-pixel blend itself is plain integer fixed-point math ( weights scaled 0..256 , i.e. an
*        8.8 fixed point ) : no floating point, no division, only multiply/add/shift in the inner loop.
*        That keeps the hot path ( the only part that's O(dstW*dstH*channels) ) cheap and branch-free
*        per channel, which is what actually matters for a function that runs on every uploaded photo.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basicImaging.h"
#include "codecs/codecs.h"

#define FIXED_ONE 256u //8.8 fixed point : weights run 0..256 inclusive:1

struct resizeAxisTap
{
  unsigned int lowIndex;
  unsigned int highIndex;
  unsigned int highWeight; //0..256 ; lowWeight is implicitly (256-highWeight)
};


//Precomputes , for every destination coordinate along one axis, which two source pixels to blend and
//by how much. Uses the standard "pixel center" mapping ( srcCoord = (dst+0.5)*scale-0.5 ) so a resize
//to the same size is the identity and there's no half-pixel drift at the edges.
static struct resizeAxisTap * buildAxisTaps(unsigned int dstCount,unsigned int srcCount)
{
  if ( (dstCount==0) || (srcCount==0) ) { return 0; }

  struct resizeAxisTap * taps = (struct resizeAxisTap *) malloc(sizeof(struct resizeAxisTap)*dstCount);
  if (taps==0) { return 0; }

  double scale = (double)srcCount/(double)dstCount;

  unsigned int d;
  for (d=0; d<dstCount; d++)
  {
    double srcCoord = ( (double)d + 0.5 )*scale - 0.5;
    if (srcCoord<0.0) { srcCoord=0.0; }

    unsigned int low = (unsigned int) srcCoord;
    if (low>=srcCount) { low=srcCount-1; }
    unsigned int high = low+1;
    if (high>=srcCount) { high=srcCount-1; }

    double frac = srcCoord - (double)low;
    if (frac<0.0) { frac=0.0; }
    if (frac>1.0) { frac=1.0; }

    taps[d].lowIndex=low;
    taps[d].highIndex=high;
    taps[d].highWeight=(unsigned int)(frac*(double)FIXED_ONE + 0.5);
  }

  return taps;
}


struct Image * BasicImaging_Resize(const struct Image * img,unsigned int newWidth,unsigned int newHeight)
{
  if ( (img==0) || (img->pixels==0) || (img->width==0) || (img->height==0) ) { return 0; }
  if ( (newWidth==0) || (newHeight==0) ) { return 0; }

  unsigned int channels = (img->channels>0) ? img->channels : 1;
  unsigned int srcWidth = img->width;
  unsigned int srcHeight = img->height;

  //Overflow guard : image_size / row strides below are unsigned int , so refuse anything that
  //wouldn't fit rather than silently wrapping and under-allocating.
  unsigned long long dstPixelCount = (unsigned long long)newWidth * (unsigned long long)newHeight;
  unsigned long long dstByteCount  = dstPixelCount * (unsigned long long)channels;
  if ( (dstByteCount==0) || (dstByteCount > 0xFFFFFFFFULL) ) { return 0; }

  //Fast path : no resampling needed at all , just hand back a plain pixel copy
  if ( (newWidth==srcWidth) && (newHeight==srcHeight) )
  {
    return copyImage((struct Image*)img); //copyImage() is codecs' own exact-copy helper
  }

  struct resizeAxisTap * xTaps = buildAxisTaps(newWidth,srcWidth);
  struct resizeAxisTap * yTaps = buildAxisTaps(newHeight,srcHeight);
  if ( (xTaps==0) || (yTaps==0) ) { free(xTaps); free(yTaps); return 0; }

  struct Image * out = BasicImaging_New(newWidth,newHeight,channels);
  if (out==0) { free(xTaps); free(yTaps); return 0; }

  const unsigned int srcStride = srcWidth*channels;
  const unsigned int dstStride = newWidth*channels;
  const unsigned char * srcPixels = img->pixels;
  unsigned char * dstPixels = out->pixels;

  unsigned int dy;
  for (dy=0; dy<newHeight; dy++)
  {
    const struct resizeAxisTap * yTap = &yTaps[dy];
    const unsigned char * rowTop = srcPixels + (size_t)yTap->lowIndex  * srcStride;
    const unsigned char * rowBot = srcPixels + (size_t)yTap->highIndex * srcStride;
    const unsigned int wY1 = yTap->highWeight;
    const unsigned int wY0 = FIXED_ONE - wY1;

    unsigned char * dstRow = dstPixels + (size_t)dy*dstStride;

    unsigned int dx;
    for (dx=0; dx<newWidth; dx++)
    {
      const struct resizeAxisTap * xTap = &xTaps[dx];
      const unsigned int sx0 = xTap->lowIndex  * channels;
      const unsigned int sx1 = xTap->highIndex * channels;
      const unsigned int wX1 = xTap->highWeight;
      const unsigned int wX0 = FIXED_ONE - wX1;

      unsigned char * dstPixel = dstRow + (size_t)dx*channels;

      unsigned int c;
      for (c=0; c<channels; c++)
      {
        unsigned int p00 = rowTop[sx0+c];
        unsigned int p10 = rowTop[sx1+c];
        unsigned int p01 = rowBot[sx0+c];
        unsigned int p11 = rowBot[sx1+c];

        unsigned int top = p00*wX0 + p10*wX1; //0 .. 255*256 = 65280 , fits comfortably in unsigned int
        unsigned int bot = p01*wX0 + p11*wX1;

        //top/bot are each already weighted by FIXED_ONE on the X axis ; blend them on the Y axis and
        //divide back down by FIXED_ONE*FIXED_ONE in one shift , rounding to nearest instead of truncating.
        unsigned int blended = (top*wY0 + bot*wY1 + (FIXED_ONE*FIXED_ONE/2)) >> 16;

        dstPixel[c] = (unsigned char) blended;
      }
    }
  }

  free(xTaps);
  free(yTaps);
  return out;
}


struct Image * BasicImaging_Thumbnail(const struct Image * img,unsigned int maxDimension)
{
  if ( (img==0) || (img->pixels==0) || (img->width==0) || (img->height==0) || (maxDimension==0) ) { return 0; }

  unsigned int w = img->width;
  unsigned int h = img->height;

  if ( (w<=maxDimension) && (h<=maxDimension) )
  {
    //Already fits : never upscale. A plain copy means the caller can always BasicImaging_Free() the
    //result the same way , whether or not resizing actually happened.
    return copyImage((struct Image*)img);
  }

  double scale = (w>h) ? ( (double)maxDimension/(double)w ) : ( (double)maxDimension/(double)h );

  unsigned int newW = (unsigned int)( (double)w*scale + 0.5 );
  unsigned int newH = (unsigned int)( (double)h*scale + 0.5 );
  if (newW<1) { newW=1; }
  if (newH<1) { newH=1; }

  return BasicImaging_Resize(img,newW,newH);
}
