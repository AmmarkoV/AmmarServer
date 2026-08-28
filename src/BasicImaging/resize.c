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
*        Three implementations share that precomputed table :
*          - scalar : plain integer fixed-point math ( 7-bit fixed point weights ) , always available,
*                     on every architecture. Also the correctness reference : the SIMD paths are
*                     verified BYTE-IDENTICAL to it in testBasicImaging.c.
*          - SSE3   : the same arithmetic, with the horizontal blend done by a SSSE3 pshufb window
*                     gather ( 4 destination pixels of all 3 channels per batch ) and the vertical
*                     blend done by SSE2 8-wide batches. The trick that makes this fast is a rolling
*                     row cache : each x-blended source row is reused when the next destination row
*                     samples it again ( consecutive rows share one tap almost everywhere ) , so the
*                     expensive horizontal pass runs once per source row instead of once per row-tap.
*          - AVX2   : identical horizontal pass, vertical blend widened to 256-bit loads.
*        BasicImaging_Resize() picks one at runtime by asking the CPU what it supports ( cached after
*        the first call ) - real callers always just call BasicImaging_Resize() ; which path actually
*        runs never changes the function signature or the result. See resize_internal.h for the
*        benchmark-only hook that forces a specific path.
*
*        Only 3/4-channel images take the SIMD paths ( RGB photos and RGBA icons - what cameras,
*        browsers and PNGs actually produce ; grayscale falls back to the scalar reference, still
*        correct ) and only when the horizontal scale keeps each 4-pixel gather window within 32
*        source bytes - heavy downscales like a 9.6x thumbnail have taps too far apart for the
*        window trick and use the scalar path, which is already close to optimal for that case
*        anyway ( every source row is used there, so the rolling cache has nothing to amortize ).
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
  unsigned short * weight; //0..128 : weight of the high tap ( low tap's weight is implicitly 128-this )
  unsigned char  * rel0;   //low  tap's byte offset within its 4-pixel gather window : (low[d]-low[base])*channels+c
  unsigned char  * rel1;   //high tap's byte offset within the same window - only the SIMD window gather reads these
};

//Precomputes , for every destination coordinate along one axis, which two source pixels to blend and
//by how much. Uses the standard "pixel center" mapping ( srcCoord = (dst+0.5)*scale-0.5 ) so a resize
//to the same size is the identity and there's no half-pixel drift at the edges. rel0/rel1 are
//best-effort extras : if they can't be allocated the table is still perfectly usable by the scalar
//path ( and the SIMD paths will notice and fall back ).
static int buildAxisTaps(struct AxisTaps * taps,unsigned int dstCount,unsigned int srcCount,unsigned int channels)
{
  taps->low=0; taps->high=0; taps->weight=0; taps->rel0=0; taps->rel1=0;
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

  //+16 bytes slack so the window gather's 16-byte load at the last full batch never reads past the end
  taps->rel0 = (unsigned char *) malloc((size_t)dstCount*channels + 16);
  taps->rel1 = (unsigned char *) malloc((size_t)dstCount*channels + 16);
  if ( (taps->rel0==0) || (taps->rel1==0) )
  {
    free(taps->rel0); free(taps->rel1);
    taps->rel0=0; taps->rel1=0; //optional extra - scalar path doesn't need them
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

    if ( (taps->rel0!=0) && (taps->rel1!=0) )
    {
      //Window base = the batch-aligned tap : 4-pixel batches always start at a multiple of 4.
      unsigned int base = d & ~3u;
      unsigned int c;
      for (c=0; c<channels; c++)
      {
        taps->rel0[d*channels+c] = (unsigned char)( (taps->low[d] -taps->low[base])*channels + c );
        taps->rel1[d*channels+c] = (unsigned char)( (taps->high[d]-taps->low[base])*channels + c );
      }
    }
  }

  return 1;
}

static void freeAxisTaps(struct AxisTaps * taps)
{
  free(taps->low); free(taps->high); free(taps->weight); free(taps->rel0); free(taps->rel1);
  taps->low=0; taps->high=0; taps->weight=0; taps->rel0=0; taps->rel1=0;
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

        unsigned int top = p00*wX0 + p10*wX1; //<=255*128=32640 , fits comfortably in unsigned int
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

//Shared by both SIMD paths : Y-blends ( and rounds/narrows to bytes ) exactly 8 x-blended, 16-bit-lane
//pixel values. AVX2's 16-wide batch just calls this twice, once per 128-bit half - see the comment in
//yBlendRowAVX2() for why splitting it this way sidesteps AVX2's cross-lane pack/unpack semantics
//instead of fighting them.
//
//Values are always <=255*128=32640 here ( 7-bit weights ) so they fit SIGNED 16-bit lanes, which is
//exactly what _mm_madd_epi16 needs - that's the whole reason the fixed point is 7-bit and not 8.8.
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

/* Horizontal blend of ONE whole source row into an interleaved unsigned-short row :
 * tmpRow[px*3+c] = srcRow[low*3+c]*(128-w) + srcRow[high*3+c]*w , exact, no rounding yet ( the only
 * rounding happens in the final vertical blend, so the SIMD result stays byte-identical to scalar ).
 *
 * The gather : 4 destination pixels of all 3 channels at a time. The taps of those 4 pixels lie within
 * (span*3) bytes of srcRow+low[base]*3 and xWindowOK() has guaranteed span*3+2<32 , so two 16-byte
 * window loads cover every byte the batch can need. pshufb then picks each needed byte by its precomputed
 * rel offset ; a cmpgt/andnot/and/or select picks between the two windows for offsets >=16 ( pshufb
 * can't look past the 16 bytes of its own input register ). One pshufb result carries r,g,b of all 4
 * pixels, so a single gather serves every channel - that fusion is what makes this ~3x faster than the
 * scalar x-blend instead of break-even.
 */
__attribute__((target("ssse3")))
static void xBlendRow3SSSE3(const unsigned char * srcRow,const struct AxisTaps * xTaps,unsigned int srcWidth,unsigned int newWidth,unsigned short * tmpRow)
{
  const unsigned char * rowEnd = srcRow + (size_t)srcWidth*3;
  __m128i mask15 = _mm_set1_epi8(15);
  __m128i zero   = _mm_setzero_si128();
  __m128i one    = _mm_set1_epi16((short)FIXED_ONE);
  __m128i wIdxA  = _mm_setr_epi8(0,1,0,1,0,1,2,3,2,3,2,3,4,5,4,5);          //{w0,w0,w0,w1,w1,w1,w2,w2}
  __m128i wIdxB  = _mm_setr_epi8(4,5,6,7,6,7,6,7,8,9,10,11,12,13,14,15);  //{w2,w3,w3,w3,0,0,0,0}

  unsigned int d=0;
  for (; (d+4<=newWidth) && ( (size_t)(rowEnd - (srcRow + (size_t)xTaps->low[d]*3)) >= 32 ); d+=4)
  {
    const unsigned char * win = srcRow + (size_t)xTaps->low[d]*3;
    __m128i W0 = _mm_loadu_si128((const __m128i*)(win));
    __m128i W1 = _mm_loadu_si128((const __m128i*)(win+16));

    //Gather the two taps of all 4 pixels : rel0/rel1 are byte offsets 0..31 within the two windows.
    __m128i idx0 = _mm_loadu_si128((const __m128i*)(xTaps->rel0 + (size_t)d*3));
    __m128i idx1 = _mm_loadu_si128((const __m128i*)(xTaps->rel1 + (size_t)d*3));

    __m128i m0  = _mm_cmpgt_epi8(idx0,mask15); //0xFF where the byte lives in W1
    __m128i g00 = _mm_shuffle_epi8(W0,idx0);
    __m128i g01 = _mm_shuffle_epi8(W1,idx0);
    __m128i p00 = _mm_or_si128(_mm_andnot_si128(m0,g00),_mm_and_si128(m0,g01));

    __m128i m1  = _mm_cmpgt_epi8(idx1,mask15);
    __m128i g10 = _mm_shuffle_epi8(W0,idx1);
    __m128i g11 = _mm_shuffle_epi8(W1,idx1);
    __m128i p10 = _mm_or_si128(_mm_andnot_si128(m1,g10),_mm_and_si128(m1,g11));

    //Widen bytes -> 16-bit lanes : p00lo = {r0,g0,b0,r1,g1,b1,r2,g2} , p00hi = {b2,r3,g3,b3,*,*,*,*}
    __m128i p00lo = _mm_unpacklo_epi8(p00,zero);
    __m128i p00hi = _mm_unpackhi_epi8(p00,zero);
    __m128i p10lo = _mm_unpacklo_epi8(p10,zero);
    __m128i p10hi = _mm_unpackhi_epi8(p10,zero);

    //Replicate the 4 per-pixel weights across the 3 channels each one applies to ( lane pattern above ).
    __m128i w    = _mm_loadl_epi64((const __m128i*)(xTaps->weight + d));
    __m128i wA   = _mm_shuffle_epi8(w,wIdxA);
    __m128i wB   = _mm_shuffle_epi8(w,wIdxB);
    __m128i wA0  = _mm_sub_epi16(one,wA);
    __m128i wB0  = _mm_sub_epi16(one,wB);

    __m128i topLo = _mm_add_epi16(_mm_mullo_epi16(p00lo,wA0),_mm_mullo_epi16(p10lo,wA)); //{r0,g0,b0,r1,g1,b1,r2,g2}
    __m128i topHi = _mm_add_epi16(_mm_mullo_epi16(p00hi,wB0),_mm_mullo_epi16(p10hi,wB)); //{b2,r3,g3,b3,0,0,0,0}

    //Stored interleaved they land EXACTLY at tmpRow[(d+i)*3+c] - contiguous, no scatter.
    _mm_storeu_si128((__m128i*)(tmpRow + (size_t)d*3),topLo);
    _mm_storel_epi64((__m128i*)(tmpRow + (size_t)d*3+8),topHi);
  }

  //Tail : the last 0..3 pixels of the row ( and the whole row when the source is too small for a
  //32-byte window ) , plain scalar - identical arithmetic, a handful of pixels at most.
  for (; d<newWidth; d++)
  {
    const unsigned char * p0 = srcRow + (size_t)xTaps->low[d]*3;
    const unsigned char * p1 = srcRow + (size_t)xTaps->high[d]*3;
    unsigned int w1 = xTaps->weight[d];
    unsigned int w0 = FIXED_ONE-w1;
    unsigned short * o = tmpRow + (size_t)d*3;
    o[0]=(unsigned short)(p0[0]*w0 + p1[0]*w1);
    o[1]=(unsigned short)(p0[1]*w0 + p1[1]*w1);
    o[2]=(unsigned short)(p0[2]*w0 + p1[2]*w1);
  }
}

/* The same window gather for 4-channel ( RGBA ) rows - the case PNG icons hit. 16 bytes is exactly
 * 4 RGBA pixels, so unlike the 3-channel version the widened lanes split at clean pixel boundaries
 * and both stores are full 16-byte stores.
 */
__attribute__((target("ssse3")))
static void xBlendRow4SSSE3(const unsigned char * srcRow,const struct AxisTaps * xTaps,unsigned int srcWidth,unsigned int newWidth,unsigned short * tmpRow)
{
  const unsigned char * rowEnd = srcRow + (size_t)srcWidth*4;
  __m128i mask15 = _mm_set1_epi8(15);
  __m128i zero   = _mm_setzero_si128();
  __m128i one    = _mm_set1_epi16((short)FIXED_ONE);
  __m128i wIdxA  = _mm_setr_epi8(0,1,0,1,0,1,0,1,2,3,2,3,2,3,2,3); //{w0,w0,w0,w0,w1,w1,w1,w1}
  __m128i wIdxB  = _mm_setr_epi8(4,5,4,5,4,5,4,5,6,7,6,7,6,7,6,7); //{w2,w2,w2,w2,w3,w3,w3,w3}

  unsigned int d=0;
  for (; (d+4<=newWidth) && ( (size_t)(rowEnd - (srcRow + (size_t)xTaps->low[d]*4)) >= 32 ); d+=4)
  {
    const unsigned char * win = srcRow + (size_t)xTaps->low[d]*4;
    __m128i W0 = _mm_loadu_si128((const __m128i*)(win));
    __m128i W1 = _mm_loadu_si128((const __m128i*)(win+16));

    __m128i idx0 = _mm_loadu_si128((const __m128i*)(xTaps->rel0 + (size_t)d*4));
    __m128i idx1 = _mm_loadu_si128((const __m128i*)(xTaps->rel1 + (size_t)d*4));

    __m128i m0  = _mm_cmpgt_epi8(idx0,mask15);
    __m128i g00 = _mm_shuffle_epi8(W0,idx0);
    __m128i g01 = _mm_shuffle_epi8(W1,idx0);
    __m128i p00 = _mm_or_si128(_mm_andnot_si128(m0,g00),_mm_and_si128(m0,g01));

    __m128i m1  = _mm_cmpgt_epi8(idx1,mask15);
    __m128i g10 = _mm_shuffle_epi8(W0,idx1);
    __m128i g11 = _mm_shuffle_epi8(W1,idx1);
    __m128i p10 = _mm_or_si128(_mm_andnot_si128(m1,g10),_mm_and_si128(m1,g11));

    __m128i p00lo = _mm_unpacklo_epi8(p00,zero); //{r0,g0,b0,a0,r1,g1,b1,a1}
    __m128i p00hi = _mm_unpackhi_epi8(p00,zero); //{r2,g2,b2,a2,r3,g3,b3,a3}
    __m128i p10lo = _mm_unpacklo_epi8(p10,zero);
    __m128i p10hi = _mm_unpackhi_epi8(p10,zero);

    __m128i w    = _mm_loadl_epi64((const __m128i*)(xTaps->weight + d));
    __m128i wA   = _mm_shuffle_epi8(w,wIdxA);
    __m128i wB   = _mm_shuffle_epi8(w,wIdxB);
    __m128i wA0  = _mm_sub_epi16(one,wA);
    __m128i wB0  = _mm_sub_epi16(one,wB);

    __m128i topLo = _mm_add_epi16(_mm_mullo_epi16(p00lo,wA0),_mm_mullo_epi16(p10lo,wA)); //pixels 0..1
    __m128i topHi = _mm_add_epi16(_mm_mullo_epi16(p00hi,wB0),_mm_mullo_epi16(p10hi,wB)); //pixels 2..3

    _mm_storeu_si128((__m128i*)(tmpRow + (size_t)d*4),topLo);
    _mm_storeu_si128((__m128i*)(tmpRow + (size_t)d*4+8),topHi);
  }

  for (; d<newWidth; d++)
  {
    const unsigned char * p0 = srcRow + (size_t)xTaps->low[d]*4;
    const unsigned char * p1 = srcRow + (size_t)xTaps->high[d]*4;
    unsigned int w1 = xTaps->weight[d];
    unsigned int w0 = FIXED_ONE-w1;
    unsigned short * o = tmpRow + (size_t)d*4;
    o[0]=(unsigned short)(p0[0]*w0 + p1[0]*w1);
    o[1]=(unsigned short)(p0[1]*w0 + p1[1]*w1);
    o[2]=(unsigned short)(p0[2]*w0 + p1[2]*w1);
    o[3]=(unsigned short)(p0[3]*w0 + p1[3]*w1);
  }
}

/* Vertical blend of one whole x-blended row pair into the destination row : out[i] = rowA[i]*wY0 +
 * rowB[i]*wY1 , rounded and narrowed to bytes. Fully contiguous on both sides - no gather, no scatter,
 * which is what a SIMD loop needs to actually win.
 */
__attribute__((target("sse2")))
static void yBlendRowSSE2(const unsigned short * rowA,const unsigned short * rowB,unsigned int wY0,unsigned int wY1,unsigned char * dstRow,unsigned int count)
{
  unsigned int i=0;
  for (; i+8<=count; i+=8)
  {
    __m128i top = _mm_loadu_si128((const __m128i*)(rowA+i));
    __m128i bot = _mm_loadu_si128((const __m128i*)(rowB+i));
    yBlendPack8SSE2(top,bot,wY0,wY1,dstRow+i);
  }
  for (; i<count; i++)
  {
    dstRow[i] = (unsigned char)( ( (unsigned int)rowA[i]*wY0 + (unsigned int)rowB[i]*wY1 + FIXED_ROUND ) >> FIXED_SHIFT );
  }
}

/* True 256-bit version of yBlendPack8SSE2 : blends 16 x-blended pixel values in one go. AVX2's
 * madd/pack/unpack instructions all operate per 128-bit half, but with the pairs arranged as
 * {top0,bot0,top1,bot1,...} in each half the halves never need to talk to each other : after the
 * pack chain, pixels 0..7 sit in the low half of packed16 and 8..15 in the high half, so swapping
 * the halves once lets the final packus_epi16 land all 16 output bytes contiguously in the low
 * 16 bytes - one 16-byte store, no cross-lane data loss ( packus_epi16(a,a) would duplicate each
 * half instead and overrun the output, so the swap is load-bearing ).
 */
__attribute__((target("avx2")))
static inline void yBlendPack16AVX2(__m256i top,__m256i bot,unsigned int wY0,unsigned int wY1,unsigned char * out16)
{
  __m256i topbotLo = _mm256_unpacklo_epi16(top,bot); //per half : {top0,bot0,top1,bot1,...}
  __m256i topbotHi = _mm256_unpackhi_epi16(top,bot);
  __m256i wYv   = _mm256_set1_epi32( ((int)wY1<<16) | (int)wY0 );
  __m256i round = _mm256_set1_epi32( (int)FIXED_ROUND );

  __m256i blLo = _mm256_srli_epi32( _mm256_add_epi32(_mm256_madd_epi16(topbotLo,wYv),round), FIXED_SHIFT ); //pixels 0..3,8..11
  __m256i blHi = _mm256_srli_epi32( _mm256_add_epi32(_mm256_madd_epi16(topbotHi,wYv),round), FIXED_SHIFT ); //pixels 4..7,12..15

  __m256i packed16 = _mm256_packs_epi32(blLo,blHi); //16x16-bit lanes : low half = pixels 0..7 , high half = 8..15
  __m256i swapped  = _mm256_permute4x64_epi64(packed16, _MM_SHUFFLE(1,0,3,2)); //pixels 8..15 now in the low half
  __m256i packed8  = _mm256_packus_epi16(packed16,swapped); //low 16 bytes = pixels 0..15 in order

  _mm_storeu_si128((__m128i*)out16,_mm256_castsi256_si128(packed8));
}

__attribute__((target("avx2")))
static void yBlendRowAVX2(const unsigned short * rowA,const unsigned short * rowB,unsigned int wY0,unsigned int wY1,unsigned char * dstRow,unsigned int count)
{
  unsigned int i=0;
  for (; i+16<=count; i+=16)
  {
    __m256i top = _mm256_loadu_si256((const __m256i*)(rowA+i));
    __m256i bot = _mm256_loadu_si256((const __m256i*)(rowB+i));
    yBlendPack16AVX2(top,bot,wY0,wY1,dstRow+i);
  }
  _mm256_zeroupper(); //avoid the SSE/AVX transition penalty on whatever runs after us
  for (; i<count; i++)
  {
    dstRow[i] = (unsigned char)( ( (unsigned int)rowA[i]*wY0 + (unsigned int)rowB[i]*wY1 + FIXED_ROUND ) >> FIXED_SHIFT );
  }
}

/* Is the 4-pixel window gather usable along this axis ? Every full batch's tap span must stay within
 * the two 16-byte windows ( max rel offset <32 ) and the rel tables must exist. Callers that get a 0
 * here fall back to the scalar reference - same result, no crash.
 */
static int xWindowOK(const struct AxisTaps * xTaps,unsigned int dstCount,unsigned int channels)
{
  unsigned int d;
  if ( (xTaps->rel0==0) || (xTaps->rel1==0) ) { return 0; }
  for (d=0; d+4<=dstCount; d+=4)
  {
    unsigned int span = xTaps->low[d+3]-xTaps->low[d]+1;
    if ( span*channels + (channels-1) >= 32 ) { return 0; }
  }
  return 1;
}

typedef void (*XBlendRowFn)(const unsigned char *,const struct AxisTaps *,unsigned int,unsigned int,unsigned short *);
typedef void (*YBlendRowFn)(const unsigned short *,const unsigned short *,unsigned int,unsigned int,unsigned char *,unsigned int);

/* The SIMD single-pass driver : for each destination row, x-blend the two source rows it samples into
 * interleaved unsigned-short temp rows ( tmpA/tmpB ) and y-blend the pair into the output row. The
 * rolling cache : consecutive destination rows usually share one source-row tap ( low[dy]==high[dy-1]
 * for downscale/mild resize, low[dy]==low[dy-1] for upscale ) , and when they do the x-blend of that
 * row is reused instead of recomputed - the horizontal pass then costs ~one row per destination row
 * instead of two, which is where most of the speedup over scalar comes from.
 */
static struct Image * resizeRolling(const struct Image * img,unsigned int newWidth,unsigned int newHeight,
                                    struct AxisTaps * xTaps,struct AxisTaps * yTaps,
                                    XBlendRowFn xBlend,YBlendRowFn yBlend)
{
  unsigned int channels = img->channels;

  struct Image * out = BasicImaging_New(newWidth,newHeight,channels);
  if (out==0) { return 0; }

  unsigned int rowPixels = newWidth*channels; //interleaved channels per pixel

  unsigned short * tmpA = (unsigned short *) malloc( (size_t)rowPixels*sizeof(unsigned short) );
  unsigned short * tmpB = (unsigned short *) malloc( (size_t)rowPixels*sizeof(unsigned short) );
  if ( (tmpA==0) || (tmpB==0) )
  {
    free(tmpA); free(tmpB);
    BasicImaging_Free(&out);
    return resizeScalar(img,newWidth,newHeight,xTaps,yTaps); //graceful fallback, never a crash
  }

  const unsigned char * srcPixels = img->pixels;
  unsigned int srcStride = img->width*channels;
  unsigned int srcWidth = img->width;
  unsigned char * dstPixels = out->pixels;

  xBlend(srcPixels + (size_t)yTaps->low[0] *srcStride, xTaps, srcWidth, newWidth, tmpA);
  xBlend(srcPixels + (size_t)yTaps->high[0]*srcStride, xTaps, srcWidth, newWidth, tmpB);

  unsigned int dy;
  for (dy=0; dy<newHeight; dy++)
  {
    if (dy>0)
    {
      unsigned int lo  = yTaps->low[dy];
      unsigned int hi  = yTaps->high[dy];
      unsigned int plo = yTaps->low[dy-1];
      unsigned int phi = yTaps->high[dy-1];

      if ( (lo==plo) && (hi==phi) ) { /* same row pair as last time : reuse both temp rows as-is */ } else
      if (lo==phi) { unsigned short * t=tmpA; tmpA=tmpB; tmpB=t; /* tmpB already holds row lo */
                     xBlend(srcPixels+(size_t)hi*srcStride, xTaps, srcWidth, newWidth, tmpB); } else
      if (lo==plo) { xBlend(srcPixels+(size_t)hi*srcStride, xTaps, srcWidth, newWidth, tmpB); } else
      if (hi==phi) { xBlend(srcPixels+(size_t)lo*srcStride, xTaps, srcWidth, newWidth, tmpA); } else
      { xBlend(srcPixels+(size_t)lo*srcStride, xTaps, srcWidth, newWidth, tmpA);
        xBlend(srcPixels+(size_t)hi*srcStride, xTaps, srcWidth, newWidth, tmpB); }
    }

    unsigned int wY1 = yTaps->weight[dy];
    unsigned int wY0 = FIXED_ONE-wY1;
    yBlend(tmpA,tmpB,wY0,wY1,dstPixels+(size_t)dy*rowPixels,rowPixels);
  }

  free(tmpA); free(tmpB);
  return out;
}

static struct Image * resizeSSE3(const struct Image * img,unsigned int newWidth,unsigned int newHeight,
                                  struct AxisTaps * xTaps,struct AxisTaps * yTaps)
{
  //The SIMD kernels only know how to handle 3/4-channel rows whose 4-pixel gather windows fit in
  //32 source bytes ; everything else silently takes the scalar reference ( same bytes, just not
  //SIMD-accelerated ).
  if ( ((img->channels!=3) && (img->channels!=4)) || !xWindowOK(xTaps,newWidth,img->channels) )
  {
    return resizeScalar(img,newWidth,newHeight,xTaps,yTaps);
  }
  return resizeRolling(img,newWidth,newHeight,xTaps,yTaps,
                       (img->channels==3) ? xBlendRow3SSSE3 : xBlendRow4SSSE3,
                       yBlendRowSSE2);
}

static struct Image * resizeAVX2(const struct Image * img,unsigned int newWidth,unsigned int newHeight,
                                  struct AxisTaps * xTaps,struct AxisTaps * yTaps)
{
  if ( ((img->channels!=3) && (img->channels!=4)) || !xWindowOK(xTaps,newWidth,img->channels) )
  {
    return resizeScalar(img,newWidth,newHeight,xTaps,yTaps);
  }
  return resizeRolling(img,newWidth,newHeight,xTaps,yTaps,
                       (img->channels==3) ? xBlendRow3SSSE3 : xBlendRow4SSSE3,
                       yBlendRowAVX2);
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
    if (__builtin_cpu_supports("ssse3")) { best = BASICIMAGING_RESIZE_SSE3; }
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
    if ( (forcedPath==BASICIMAGING_RESIZE_SSE3) && (best==BASICIMAGING_RESIZE_SCALAR) ) { return best; }
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
    case BASICIMAGING_RESIZE_SSE3:   return "SSE3";
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
  if ( !buildAxisTaps(&xTaps,newWidth,srcWidth,channels) || !buildAxisTaps(&yTaps,newHeight,srcHeight,channels) )
  {
    freeAxisTaps(&xTaps); freeAxisTaps(&yTaps);
    return 0;
  }

  struct Image * out = 0;
  enum BasicImaging_ResizePath path = BasicImaging_Resize_ActivePath();

  #if defined(__x86_64__) || defined(__i386__)
    if (path==BASICIMAGING_RESIZE_AVX2)      { out = resizeAVX2(img,newWidth,newHeight,&xTaps,&yTaps); } else
    if (path==BASICIMAGING_RESIZE_SSE3)      { out = resizeSSE3(img,newWidth,newHeight,&xTaps,&yTaps); } else
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
