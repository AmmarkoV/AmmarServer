/** @file resize.c
* @brief BasicImaging_Resize() - the one call almost every Service actually needs : a fast, safe,
*        general-purpose image resize.
*
*        Algorithm : separable bilinear interpolation. Each destination pixel is a blend of the 4
*        nearest source pixels ( 2 taps horizontally x 2 taps vertically ). "Separable" here means the
*        horizontal and vertical sampling positions/weights are precomputed ONCE per destination column
*        / row ( O(newWidth+newHeight) ) rather than being recomputed from scratch for every one of the
*        newWidth*newHeight destination pixels.
*
*        Three implementations of the actual per-pixel blend share that precomputed table :
*          - scalar   : plain integer fixed-point math ( 8.8 fixed point weights ) , always available,
*                       on every architecture.
*          - SSE2     : the same arithmetic, batched 8 destination pixels of one channel at a time.
*          - AVX2     : batched 16 destination pixels of one channel at a time.
*        BasicImaging_Resize() picks one at runtime by asking the CPU what it supports ( cached after
*        the first call ) - real callers always just call BasicImaging_Resize() ; which path actually
*        runs never changes the function signature or the result ( the SIMD paths are verified
*        byte-identical to the scalar reference in testBasicImaging.c ). See resize_internal.h for the
*        benchmark-only hook that forces a specific path.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basicImaging.h"
#include "resize_internal.h"
#include "codecs/codecs.h"

/* 7-bit fixed point : weights run 0..128 inclusive ( NOT 0..256 - see the comment on yBlendPack8SSE2()
 * below for why the SIMD paths need this narrower range ). Still 128 distinct fractional steps per
 * axis, far finer than bilinear interpolation needs to look smooth - this costs no visible quality. */
#define FIXED_BITS 7u
#define FIXED_ONE (1u<<FIXED_BITS)             //128
#define FIXED_SHIFT (2u*FIXED_BITS)            //14 : how far to shift the two-axis blended value down
#define FIXED_ROUND (1u<<(FIXED_SHIFT-1))      //8192 : rounds to nearest instead of truncating

/* ===================================================================
   Shared precomputed sampling table ( structure-of-arrays , so the SIMD
   paths can load 8/16 consecutive weights with one instruction )
   =================================================================== */
struct AxisTaps
{
  unsigned int   * low;    //source index of the "left"/"top" tap , one per destination coordinate
  unsigned int   * high;   //source index of the "right"/"bottom" tap
  unsigned short * weight; //0..256 : weight of the high tap ( low tap's weight is implicitly 256-this )
};

//Precomputes , for every destination coordinate along one axis, which two source pixels to blend and
//by how much. Uses the standard "pixel center" mapping ( srcCoord = (dst+0.5)*scale-0.5 ) so a resize
//to the same size is the identity and there's no half-pixel drift at the edges.
static int buildAxisTaps(struct AxisTaps * taps,unsigned int dstCount,unsigned int srcCount)
{
  taps->low=0; taps->high=0; taps->weight=0;
  if ( (dstCount==0) || (srcCount==0) ) { return 0; }

  taps->low    = (unsigned int *)   malloc(sizeof(unsigned int)*dstCount);
  taps->high   = (unsigned int *)   malloc(sizeof(unsigned int)*dstCount);
  taps->weight = (unsigned short *) malloc(sizeof(unsigned short)*dstCount);
  if ( (taps->low==0) || (taps->high==0) || (taps->weight==0) )
  {
    free(taps->low); free(taps->high); free(taps->weight);
    taps->low=0; taps->high=0; taps->weight=0;
    return 0;
  }

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

    taps->low[d]=low;
    taps->high[d]=high;
    taps->weight[d]=(unsigned short)(frac*(double)FIXED_ONE + 0.5);
  }

  return 1;
}

static void freeAxisTaps(struct AxisTaps * taps)
{
  free(taps->low); free(taps->high); free(taps->weight);
  taps->low=0; taps->high=0; taps->weight=0;
}


/* ===================================================================
   Scalar reference implementation - always available, every architecture.
   =================================================================== */
static struct Image * resizeScalar(const struct Image * img,unsigned int newWidth,unsigned int newHeight,
                                    struct AxisTaps * xTaps,struct AxisTaps * yTaps)
{
  unsigned int channels = (img->channels>0) ? img->channels : 1;
  unsigned int srcStride = img->width*channels;
  unsigned int dstStride = newWidth*channels;

  struct Image * out = BasicImaging_New(newWidth,newHeight,channels);
  if (out==0) { return 0; }

  const unsigned char * srcPixels = img->pixels;
  unsigned char * dstPixels = out->pixels;

  unsigned int dy;
  for (dy=0; dy<newHeight; dy++)
  {
    const unsigned char * rowTop = srcPixels + (size_t)yTaps->low[dy]  * srcStride;
    const unsigned char * rowBot = srcPixels + (size_t)yTaps->high[dy] * srcStride;
    const unsigned int wY1 = yTaps->weight[dy];
    const unsigned int wY0 = FIXED_ONE - wY1;

    unsigned char * dstRow = dstPixels + (size_t)dy*dstStride;

    unsigned int dx;
    for (dx=0; dx<newWidth; dx++)
    {
      const unsigned int sx0 = xTaps->low[dx]  * channels;
      const unsigned int sx1 = xTaps->high[dx] * channels;
      const unsigned int wX1 = xTaps->weight[dx];
      const unsigned int wX0 = FIXED_ONE - wX1;

      unsigned char * dstPixel = dstRow + (size_t)dx*channels;

      unsigned int c;
      for (c=0; c<channels; c++)
      {
        unsigned int p00 = rowTop[sx0+c];
        unsigned int p10 = rowTop[sx1+c];
        unsigned int p01 = rowBot[sx0+c];
        unsigned int p11 = rowBot[sx1+c];

        unsigned int top = p00*wX0 + p10*wX1; //<=255*256=65280 , fits comfortably in unsigned int
        unsigned int bot = p01*wX0 + p11*wX1;

        unsigned int blended = (top*wY0 + bot*wY1 + FIXED_ROUND) >> FIXED_SHIFT;

        dstPixel[c] = (unsigned char) blended;
      }
    }
  }

  return out;
}


/* ===================================================================
   x86 SIMD paths. Gracefully compiled out on any other architecture - the
   scalar path above is always correct and always available regardless.
   =================================================================== */
#if defined(__x86_64__) || defined(__i386__)

#include <immintrin.h>

/* One destination row , one channel , batched 8 ( SSE2 ) or 16 ( AVX2 ) destination pixels at a time :
 * gather ( scalar - each tap is a single, always in-bounds byte read ; xTaps->low/high are guaranteed
 * < source width by buildAxisTaps ) , blend ( vectorized ) , scatter ( scalar ). The gather/scatter
 * steps stay scalar on purpose - the pixel each destination sample needs isn't at a fixed stride from
 * its neighbours ( the scale factor is an arbitrary ratio ) - but the actual multiply/add/round/shift
 * arithmetic, which is what the scalar path spends most of its time on, batches cleanly.
 */

//Shared by both SSE2 and AVX2 : Y-blends ( and rounds/narrows to bytes ) exactly 8 already X-blended,
//16-bit-lane pixel values. AVX2's 16-wide batch just calls this twice, once per 128-bit half - see the
//comment above resizeChannelBatchAVX2() for why splitting it this way sidesteps AVX2's cross-lane
//pack/unpack semantics instead of fighting them.
__attribute__((target("sse2")))
static inline void yBlendPack8SSE2(__m128i top,__m128i bot,unsigned int wY0,unsigned int wY1,unsigned char * out8)
{
  //Interleave so madd_epi16 computes , per output lane , top[i]*wY0 + bot[i]*wY1 in one instruction :
  //unpacklo/hi_epi16(top,bot) gives {top0,bot0,top1,bot1,...} ; multiplying that against a register
  //holding {wY0,wY1,wY0,wY1,...} and pairwise-summing is exactly what _mm_madd_epi16 does.
  __m128i topbotLo = _mm_unpacklo_epi16(top,bot); //pixels 0..3
  __m128i topbotHi = _mm_unpackhi_epi16(top,bot); //pixels 4..7
  __m128i wYv   = _mm_set1_epi32( ((int)wY1<<16) | (int)wY0 );
  __m128i round = _mm_set1_epi32( (int)FIXED_ROUND );

  __m128i blLo = _mm_srli_epi32( _mm_add_epi32(_mm_madd_epi16(topbotLo,wYv),round), FIXED_SHIFT ); //pixels 0..3 , 32-bit
  __m128i blHi = _mm_srli_epi32( _mm_add_epi32(_mm_madd_epi16(topbotHi,wYv),round), FIXED_SHIFT ); //pixels 4..7 , 32-bit

  //Narrow 32->16->8 bit. Values are always 0..255 here so signed saturation on the way to 16-bit never
  //clips anything real ; packus_epi16 does the final unsigned 8-bit clamp.
  __m128i packed16 = _mm_packs_epi32(blLo,blHi); //8x16-bit lanes , pixels 0..7 in order
  __m128i packed8  = _mm_packus_epi16(packed16,packed16); //low 8 bytes = pixels 0..7

  _mm_storel_epi64((__m128i*)out8,packed8);
}

__attribute__((target("sse2")))
static void resizeChannelBatchSSE2(const unsigned char * rowTop,const unsigned char * rowBot,
                                    const struct AxisTaps * xTaps,unsigned int channels,unsigned int channel,
                                    unsigned int dxStart,unsigned int count, //count<=8
                                    unsigned int wY0,unsigned int wY1,
                                    unsigned char * dstRow)
{
  unsigned short p00a[8]={0},p10a[8]={0},p01a[8]={0},p11a[8]={0},wxa[8]={0};
  unsigned int i;
  for (i=0; i<count; i++)
  {
    unsigned int dx  = dxStart+i;
    unsigned int sx0 = xTaps->low[dx]*channels+channel;
    unsigned int sx1 = xTaps->high[dx]*channels+channel;
    p00a[i]=rowTop[sx0]; p10a[i]=rowTop[sx1];
    p01a[i]=rowBot[sx0]; p11a[i]=rowBot[sx1];
    wxa[i]=xTaps->weight[dx];
  }

  __m128i p00 = _mm_loadu_si128((const __m128i*)p00a);
  __m128i p10 = _mm_loadu_si128((const __m128i*)p10a);
  __m128i p01 = _mm_loadu_si128((const __m128i*)p01a);
  __m128i p11 = _mm_loadu_si128((const __m128i*)p11a);
  __m128i wx1 = _mm_loadu_si128((const __m128i*)wxa);
  __m128i wx0 = _mm_sub_epi16(_mm_set1_epi16((short)FIXED_ONE),wx1);

  __m128i top = _mm_add_epi16(_mm_mullo_epi16(p00,wx0),_mm_mullo_epi16(p10,wx1)); //<=65280 , fits u16 bit pattern
  __m128i bot = _mm_add_epi16(_mm_mullo_epi16(p01,wx0),_mm_mullo_epi16(p11,wx1));

  unsigned char out[8];
  yBlendPack8SSE2(top,bot,wY0,wY1,out);

  for (i=0; i<count; i++) { dstRow[(dxStart+i)*channels+channel] = out[i]; }
}

__attribute__((target("avx2")))
static void resizeChannelBatchAVX2(const unsigned char * rowTop,const unsigned char * rowBot,
                                    const struct AxisTaps * xTaps,unsigned int channels,unsigned int channel,
                                    unsigned int dxStart,unsigned int count, //count<=16
                                    unsigned int wY0,unsigned int wY1,
                                    unsigned char * dstRow)
{
  unsigned short p00a[16]={0},p10a[16]={0},p01a[16]={0},p11a[16]={0},wxa[16]={0};
  unsigned int i;
  for (i=0; i<count; i++)
  {
    unsigned int dx  = dxStart+i;
    unsigned int sx0 = xTaps->low[dx]*channels+channel;
    unsigned int sx1 = xTaps->high[dx]*channels+channel;
    p00a[i]=rowTop[sx0]; p10a[i]=rowTop[sx1];
    p01a[i]=rowBot[sx0]; p11a[i]=rowBot[sx1];
    wxa[i]=xTaps->weight[dx];
  }

  __m256i p00 = _mm256_loadu_si256((const __m256i*)p00a);
  __m256i p10 = _mm256_loadu_si256((const __m256i*)p10a);
  __m256i p01 = _mm256_loadu_si256((const __m256i*)p01a);
  __m256i p11 = _mm256_loadu_si256((const __m256i*)p11a);
  __m256i wx1 = _mm256_loadu_si256((const __m256i*)wxa);
  __m256i wx0 = _mm256_sub_epi16(_mm256_set1_epi16((short)FIXED_ONE),wx1);

  //X-blend across all 16 lanes in one instruction each - this is the genuine 2x-over-SSE2 width win.
  __m256i top = _mm256_add_epi16(_mm256_mullo_epi16(p00,wx0),_mm256_mullo_epi16(p10,wx1));
  __m256i bot = _mm256_add_epi16(_mm256_mullo_epi16(p01,wx0),_mm256_mullo_epi16(p11,wx1));

  //AVX2's pack/unpack instructions operate independently on each 128-bit lane ( they don't cross the
  //two halves of the register ) , so trying to Y-blend+pack all 16 lanes in one go would need extra
  //shuffles just to put the bytes back in the right order. Splitting into its two natural 128-bit
  //halves - low = pixels 0..7 , high = pixels 8..15 , exactly how _mm256_loadu_si256 laid them out
  //from p00a/etc above - and reusing yBlendPack8SSE2 on each half sidesteps that entirely.
  __m128i topLo = _mm256_castsi256_si128(top),    topHi = _mm256_extracti128_si256(top,1);
  __m128i botLo = _mm256_castsi256_si128(bot),    botHi = _mm256_extracti128_si256(bot,1);

  unsigned char out[16];
  yBlendPack8SSE2(topLo,botLo,wY0,wY1,out+0);
  yBlendPack8SSE2(topHi,botHi,wY0,wY1,out+8);

  for (i=0; i<count; i++) { dstRow[(dxStart+i)*channels+channel] = out[i]; }

  _mm256_zeroupper(); //avoid the SSE/AVX transition penalty on whatever runs after us
}

__attribute__((target("sse2")))
static struct Image * resizeSSE2(const struct Image * img,unsigned int newWidth,unsigned int newHeight,
                                  struct AxisTaps * xTaps,struct AxisTaps * yTaps)
{
  unsigned int channels = (img->channels>0) ? img->channels : 1;
  unsigned int srcStride = img->width*channels;
  unsigned int dstStride = newWidth*channels;

  struct Image * out = BasicImaging_New(newWidth,newHeight,channels);
  if (out==0) { return 0; }

  const unsigned char * srcPixels = img->pixels;
  unsigned char * dstPixels = out->pixels;

  unsigned int dy;
  for (dy=0; dy<newHeight; dy++)
  {
    const unsigned char * rowTop = srcPixels + (size_t)yTaps->low[dy]  * srcStride;
    const unsigned char * rowBot = srcPixels + (size_t)yTaps->high[dy] * srcStride;
    const unsigned int wY1 = yTaps->weight[dy];
    const unsigned int wY0 = FIXED_ONE - wY1;
    unsigned char * dstRow = dstPixels + (size_t)dy*dstStride;

    unsigned int c;
    for (c=0; c<channels; c++)
    {
      unsigned int dx=0;
      for (; dx+8<=newWidth; dx+=8)
      {
        resizeChannelBatchSSE2(rowTop,rowBot,xTaps,channels,c,dx,8,wY0,wY1,dstRow);
      }
      if (dx<newWidth)
      {
        resizeChannelBatchSSE2(rowTop,rowBot,xTaps,channels,c,dx,newWidth-dx,wY0,wY1,dstRow);
      }
    }
  }

  return out;
}

__attribute__((target("avx2")))
static struct Image * resizeAVX2(const struct Image * img,unsigned int newWidth,unsigned int newHeight,
                                  struct AxisTaps * xTaps,struct AxisTaps * yTaps)
{
  unsigned int channels = (img->channels>0) ? img->channels : 1;
  unsigned int srcStride = img->width*channels;
  unsigned int dstStride = newWidth*channels;

  struct Image * out = BasicImaging_New(newWidth,newHeight,channels);
  if (out==0) { return 0; }

  const unsigned char * srcPixels = img->pixels;
  unsigned char * dstPixels = out->pixels;

  unsigned int dy;
  for (dy=0; dy<newHeight; dy++)
  {
    const unsigned char * rowTop = srcPixels + (size_t)yTaps->low[dy]  * srcStride;
    const unsigned char * rowBot = srcPixels + (size_t)yTaps->high[dy] * srcStride;
    const unsigned int wY1 = yTaps->weight[dy];
    const unsigned int wY0 = FIXED_ONE - wY1;
    unsigned char * dstRow = dstPixels + (size_t)dy*dstStride;

    unsigned int c;
    for (c=0; c<channels; c++)
    {
      unsigned int dx=0;
      for (; dx+16<=newWidth; dx+=16)
      {
        resizeChannelBatchAVX2(rowTop,rowBot,xTaps,channels,c,dx,16,wY0,wY1,dstRow);
      }
      if (dx<newWidth)
      {
        resizeChannelBatchAVX2(rowTop,rowBot,xTaps,channels,c,dx,newWidth-dx,wY0,wY1,dstRow);
      }
    }
  }

  return out;
}

#endif // defined(__x86_64__) || defined(__i386__)


/* ===================================================================
   Runtime dispatch - detected once ( CPU features don't change mid-process ) , cached , overridable
   only through resize_internal.h's benchmark-only hook.
   =================================================================== */
static enum BasicImaging_ResizePath forcedPath = BASICIMAGING_RESIZE_AUTO;

//-1 = not yet detected , otherwise a BasicImaging_ResizePath value ( never AUTO once set )
static int detectedPath = -1;

static enum BasicImaging_ResizePath detectBestAvailablePath(void)
{
  if (detectedPath>=0) { return (enum BasicImaging_ResizePath) detectedPath; }

  enum BasicImaging_ResizePath best = BASICIMAGING_RESIZE_SCALAR;
  #if defined(__x86_64__) || defined(__i386__)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("sse2")) { best = BASICIMAGING_RESIZE_SSE2; }
    if (__builtin_cpu_supports("avx2")) { best = BASICIMAGING_RESIZE_AVX2; }
  #endif

  detectedPath = (int) best;
  return best;
}

void BasicImaging_Resize_ForcePath(enum BasicImaging_ResizePath path)
{
  forcedPath = path;
}

enum BasicImaging_ResizePath BasicImaging_Resize_ActivePath(void)
{
  enum BasicImaging_ResizePath best = detectBestAvailablePath();

  if (forcedPath==BASICIMAGING_RESIZE_AUTO) { return best; }

  //Never let a forced request exceed what this build/CPU can actually run - falls back instead of
  //crashing, same safety contract as everything else in BasicImaging.
  #if defined(__x86_64__) || defined(__i386__)
    if ( (forcedPath==BASICIMAGING_RESIZE_AVX2) && (best!=BASICIMAGING_RESIZE_AVX2) ) { return best; }
    if ( (forcedPath==BASICIMAGING_RESIZE_SSE2) && (best==BASICIMAGING_RESIZE_SCALAR) ) { return best; }
    return forcedPath;
  #else
    return BASICIMAGING_RESIZE_SCALAR;
  #endif
}

const char * BasicImaging_Resize_PathName(enum BasicImaging_ResizePath path)
{
  switch (path)
  {
    case BASICIMAGING_RESIZE_SCALAR: return "scalar";
    case BASICIMAGING_RESIZE_SSE2:   return "SSE2";
    case BASICIMAGING_RESIZE_AVX2:   return "AVX2";
    case BASICIMAGING_RESIZE_AUTO:
    default: return "auto";
  }
}


struct Image * BasicImaging_Resize(const struct Image * img,unsigned int newWidth,unsigned int newHeight)
{
  if ( (img==0) || (img->pixels==0) || (img->width==0) || (img->height==0) ) { return 0; }
  if ( (newWidth==0) || (newHeight==0) ) { return 0; }

  unsigned int srcWidth = img->width;
  unsigned int srcHeight = img->height;
  unsigned int channels = (img->channels>0) ? img->channels : 1;

  //Overflow guard : image_size / row strides are unsigned int , so refuse anything that wouldn't fit
  //rather than silently wrapping and under-allocating.
  unsigned long long dstByteCount = (unsigned long long)newWidth * (unsigned long long)newHeight * (unsigned long long)channels;
  if ( (dstByteCount==0) || (dstByteCount > 0xFFFFFFFFULL) ) { return 0; }

  //Fast path : no resampling needed at all , just hand back a plain pixel copy
  if ( (newWidth==srcWidth) && (newHeight==srcHeight) )
  {
    return copyImage((struct Image*)img); //copyImage() is codecs' own exact-copy helper
  }

  struct AxisTaps xTaps={0},yTaps={0};
  if ( !buildAxisTaps(&xTaps,newWidth,srcWidth) || !buildAxisTaps(&yTaps,newHeight,srcHeight) )
  {
    freeAxisTaps(&xTaps); freeAxisTaps(&yTaps);
    return 0;
  }

  struct Image * out = 0;
  enum BasicImaging_ResizePath path = BasicImaging_Resize_ActivePath();

  #if defined(__x86_64__) || defined(__i386__)
    if (path==BASICIMAGING_RESIZE_AVX2)      { out = resizeAVX2(img,newWidth,newHeight,&xTaps,&yTaps); } else
    if (path==BASICIMAGING_RESIZE_SSE2)      { out = resizeSSE2(img,newWidth,newHeight,&xTaps,&yTaps); } else
  #endif
                                              { out = resizeScalar(img,newWidth,newHeight,&xTaps,&yTaps); }

  freeAxisTaps(&xTaps);
  freeAxisTaps(&yTaps);
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
