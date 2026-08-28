#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "basicImaging.h"
#include "resize_internal.h"

static unsigned int failures=0;

#define CHECK(cond,msg) do { if (!(cond)) { fprintf(stderr,"FAIL: %s\n",msg); ++failures; } else { fprintf(stderr,"OK: %s\n",msg); } } while(0)


static struct Image * makeGradient(unsigned int w,unsigned int h,unsigned int channels)
{
  struct Image * img = BasicImaging_New(w,h,channels);
  if (img==0) { return 0; }

  unsigned int y,x,c;
  for (y=0; y<h; y++)
  {
    for (x=0; x<w; x++)
    {
      unsigned char * pixel = img->pixels + (size_t)(y*w+x)*channels;
      for (c=0; c<channels; c++)
      {
        pixel[c] = (unsigned char) ( (x*255)/(w>1?w-1:1) );
      }
    }
  }
  return img;
}


static void testResizeBasics()
{
  struct Image * src = makeGradient(8,8,3);
  CHECK(src!=0,"BasicImaging_New(8,8,3) allocates");
  if (src==0) { return; }

  struct Image * down = BasicImaging_Resize(src,4,4);
  CHECK(down!=0,"downscale 8x8 -> 4x4 succeeds");
  if (down!=0)
  {
    CHECK( (down->width==4) && (down->height==4) && (down->channels==3) , "downscaled dimensions correct" );
    BasicImaging_Free(&down);
    CHECK(down==0,"BasicImaging_Free nulls caller pointer");
  }

  struct Image * up = BasicImaging_Resize(src,16,16);
  CHECK(up!=0,"upscale 8x8 -> 16x16 succeeds");
  if (up!=0)
  {
    CHECK( (up->width==16) && (up->height==16) , "upscaled dimensions correct" );
    BasicImaging_Free(&up);
  }

  struct Image * same = BasicImaging_Resize(src,8,8);
  CHECK(same!=0,"same-size resize (fast path) succeeds");
  if ( (same!=0) && (src!=0) )
  {
    CHECK( memcmp(same->pixels,src->pixels,(size_t)8*8*3)==0 , "same-size resize is pixel-identical" );
    BasicImaging_Free(&same);
  }

  struct Image * thumb = BasicImaging_Thumbnail(src,4);
  CHECK(thumb!=0,"Thumbnail(maxDimension=4) succeeds");
  if (thumb!=0)
  {
    CHECK( (thumb->width<=4) && (thumb->height<=4) , "thumbnail fits maxDimension" );
    BasicImaging_Free(&thumb);
  }

  struct Image * noUpscale = BasicImaging_Thumbnail(src,100);
  CHECK(noUpscale!=0,"Thumbnail(maxDimension=100) on an 8x8 image succeeds");
  if (noUpscale!=0)
  {
    CHECK( (noUpscale->width==8) && (noUpscale->height==8) , "Thumbnail never upscales" );
    BasicImaging_Free(&noUpscale);
  }

  BasicImaging_Free(&src);
}


static void testBadInputsNeverCrash()
{
  CHECK( BasicImaging_Load(0)==0 , "Load(NULL) returns 0, doesn't crash" );
  CHECK( BasicImaging_Load("/does/not/exist/at/all.jpg")==0 , "Load(missing file) returns 0" );
  CHECK( BasicImaging_New(0,10,3)==0 , "New(width=0) returns 0" );
  CHECK( BasicImaging_Resize(0,10,10)==0 , "Resize(NULL) returns 0" );

  struct Image * img = BasicImaging_New(4,4,3);
  CHECK( BasicImaging_Resize(img,0,10)==0 , "Resize(newWidth=0) returns 0" );
  CHECK( BasicImaging_SaveJPEG(img,0,80)==0 , "SaveJPEG(NULL filename) returns 0" );
  CHECK( BasicImaging_Save(img,"output.weirdext",80)==0 , "Save() with an unrecognised extension returns 0" );
  BasicImaging_Free(&img);

  struct Image * nilImg=0;
  BasicImaging_Free(&nilImg); //must not crash on an already-null pointer
  BasicImaging_Free(0);       //must not crash on a null pointer-to-pointer either
  CHECK(1,"BasicImaging_Free tolerates NULL/already-freed pointers");
}


static void testCodecRoundTrip()
{
  fprintf(stderr,"BasicImaging_HasJPEG() = %d\n",BasicImaging_HasJPEG());
  fprintf(stderr,"BasicImaging_HasPNG()  = %d\n",BasicImaging_HasPNG());

  struct Image * src = makeGradient(32,20,3);
  CHECK(src!=0,"gradient image for codec round-trip allocates");
  if (src==0) { return; }

  if (BasicImaging_HasJPEG())
  {
    int ok = BasicImaging_SaveJPEG(src,"/tmp/basicimaging_test.jpg",85);
    CHECK(ok,"SaveJPEG succeeds when JPEG is compiled in");
    struct Image * back = BasicImaging_Load("/tmp/basicimaging_test.jpg");
    CHECK(back!=0,"reloading the saved JPEG succeeds");
    if (back!=0)
    {
      CHECK( (back->width==32) && (back->height==20) , "reloaded JPEG has the right dimensions" );
      BasicImaging_Free(&back);
    }
  } else
  {
    CHECK( BasicImaging_SaveJPEG(src,"/tmp/basicimaging_test.jpg",85)==0 , "SaveJPEG safely fails when JPEG is NOT compiled in" );
  }

  if (BasicImaging_HasPNG())
  {
    int ok = BasicImaging_SavePNG(src,"/tmp/basicimaging_test.png");
    CHECK(ok,"SavePNG succeeds when PNG is compiled in");
    struct Image * back = BasicImaging_Load("/tmp/basicimaging_test.png");
    CHECK(back!=0,"reloading the saved PNG succeeds");
    if (back!=0)
    {
      CHECK( (back->width==32) && (back->height==20) , "reloaded PNG has the right dimensions" );
      BasicImaging_Free(&back);
    }
  } else
  {
    CHECK( BasicImaging_SavePNG(src,"/tmp/basicimaging_test.png")==0 , "SavePNG safely fails when PNG is NOT compiled in" );
  }

  fprintf(stderr,"BasicImaging_HasPZP()  = %d\n",BasicImaging_HasPZP());

  if (BasicImaging_HasPZP())
  {
    int ok = BasicImaging_SavePZP(src,"/tmp/basicimaging_test.pzp");
    CHECK(ok,"SavePZP succeeds when PZP is compiled in");
    struct Image * back = BasicImaging_Load("/tmp/basicimaging_test.pzp");
    CHECK(back!=0,"reloading the saved PZP succeeds");
    if (back!=0)
    {
      CHECK( (back->width==32) && (back->height==20) , "reloaded PZP has the right dimensions" );
      BasicImaging_Free(&back);
    }

    //A truncated/corrupt .pzp must be rejected cleanly, never crash the process.
    FILE * f = fopen("/tmp/basicimaging_corrupt.pzp","wb");
    if (f!=0) { fputs("not a real pzp file",f); fclose(f); }
    struct Image * corrupt = BasicImaging_Load("/tmp/basicimaging_corrupt.pzp");
    CHECK(corrupt==0,"loading a corrupt .pzp file fails cleanly (process is still alive to report this!)");
    if (corrupt!=0) { BasicImaging_Free(&corrupt); }
  } else
  {
    CHECK( BasicImaging_SavePZP(src,"/tmp/basicimaging_test.pzp")==0 , "SavePZP safely fails when PZP is NOT compiled in" );
  }

  BasicImaging_Free(&src);
}


static void testThumbnailFileFallback()
{
  //An input that doesn't exist at all : ThumbnailFile must fail cleanly, no crash, no partial output claimed as success.
  int ok = BasicImaging_ThumbnailFile("/does/not/exist.jpg","/tmp/basicimaging_thumb_out.jpg",64,80);
  CHECK(ok==0,"ThumbnailFile on a missing input file reports failure (0), doesn't crash");
}


static struct Image * makeNoisy(unsigned int w,unsigned int h,unsigned int channels,unsigned int seed)
{
  struct Image * img = BasicImaging_New(w,h,channels);
  if (img==0) { return 0; }
  srand(seed);
  size_t i,n=(size_t)w*h*channels;
  for (i=0;i<n;i++) { img->pixels[i]=(unsigned char)(rand()&0xFF); }
  return img;
}

//The SSE2/AVX2 resize paths must be BYTE-IDENTICAL to the scalar reference , not just "close" - the
//fixed-point math is fully deterministic, so any discrepancy here means one of the SIMD kernels has an
//actual bug (wrong lane order, missing tail pixels, an off-by-one in the gather/scatter indices, ...).
static void testSIMDPathsAgreeWithScalar()
{
  const enum BasicImaging_ResizePath candidatePaths[] = { BASICIMAGING_RESIZE_SSE2, BASICIMAGING_RESIZE_AVX2 };
  const unsigned int channelsToTry[] = {1,2,3,4};
  //Sizes deliberately NOT multiples of 8/16 , so every path's batch-tail handling gets exercised too.
  const unsigned int sizesToTry[][2] = { {1,1},{3,5},{7,7},{15,9},{17,31},{64,64},{100,37},{257,129} };

  unsigned int pi,ci,si;
  for (pi=0; pi<sizeof(candidatePaths)/sizeof(candidatePaths[0]); pi++)
  {
    enum BasicImaging_ResizePath want = candidatePaths[pi];
    BasicImaging_Resize_ForcePath(want);
    enum BasicImaging_ResizePath got = BasicImaging_Resize_ActivePath();
    if (got!=want)
    {
      fprintf(stderr,"SKIP: %s not available on this CPU/build , cannot compare it against scalar\n",BasicImaging_Resize_PathName(want));
      continue;
    }

    for (ci=0; ci<sizeof(channelsToTry)/sizeof(channelsToTry[0]); ci++)
    {
      unsigned int channels=channelsToTry[ci];
      for (si=0; si<sizeof(sizesToTry)/sizeof(sizesToTry[0]); si++)
      {
        unsigned int srcW=sizesToTry[si][0], srcH=sizesToTry[si][1];
        unsigned int dstW=sizesToTry[(si+1)%(sizeof(sizesToTry)/sizeof(sizesToTry[0]))][0];
        unsigned int dstH=sizesToTry[(si+1)%(sizeof(sizesToTry)/sizeof(sizesToTry[0]))][1];

        struct Image * src = makeNoisy(srcW,srcH,channels,srcW*1000u+srcH*10u+channels);
        if (src==0) { CHECK(0,"makeNoisy allocation"); continue; }

        BasicImaging_Resize_ForcePath(BASICIMAGING_RESIZE_SCALAR);
        struct Image * refOut = BasicImaging_Resize(src,dstW,dstH);

        BasicImaging_Resize_ForcePath(want);
        struct Image * simdOut = BasicImaging_Resize(src,dstW,dstH);

        char label[160];
        snprintf(label,sizeof(label),"%s == scalar for %ux%ux%uch -> %ux%u",
                 BasicImaging_Resize_PathName(want),srcW,srcH,channels,dstW,dstH);

        int same = (refOut!=0) && (simdOut!=0)
                 && (refOut->width==simdOut->width) && (refOut->height==simdOut->height)
                 && (memcmp(refOut->pixels,simdOut->pixels,(size_t)dstW*dstH*channels)==0);
        CHECK(same,label);

        BasicImaging_Free(&refOut);
        BasicImaging_Free(&simdOut);
        BasicImaging_Free(&src);
      }
    }
  }

  BasicImaging_Resize_ForcePath(BASICIMAGING_RESIZE_AUTO); //leave dispatch as every real caller gets it
}


static void testResizeTimingSanity()
{
  //Not a hard perf assertion ( CI machines vary too much ) , just prints what's actually available on
  //this CPU/build so a human glancing at test output can see whether SIMD is even in play here.
  BasicImaging_Resize_ForcePath(BASICIMAGING_RESIZE_AUTO);
  fprintf(stderr,"Active resize path on this machine: %s\n",BasicImaging_Resize_PathName(BasicImaging_Resize_ActivePath()));
}


int main()
{
  testResizeBasics();
  testBadInputsNeverCrash();
  testCodecRoundTrip();
  testThumbnailFileFallback();
  testSIMDPathsAgreeWithScalar();
  testResizeTimingSanity();

  if (failures==0)
  {
    fprintf(stderr,"\nAll BasicImaging tests passed.\n");
    return 0;
  }

  fprintf(stderr,"\n%u BasicImaging test(s) FAILED.\n",failures);
  return 1;
}
