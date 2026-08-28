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


static void testSaveJPEGToMemory()
{
  struct Image * src = makeGradient(32,20,3);
  CHECK(src!=0,"gradient image for SaveJPEGToMemory allocates");
  if (src==0) { return; }

  unsigned char buffer[65536];
  unsigned long written=0;

  CHECK( BasicImaging_SaveJPEGToMemory(0,buffer,sizeof(buffer),&written,90)==0 , "SaveJPEGToMemory(NULL image) returns 0" );
  CHECK( BasicImaging_SaveJPEGToMemory(src,0,sizeof(buffer),&written,90)==0 , "SaveJPEGToMemory(NULL target) returns 0" );
  CHECK( BasicImaging_SaveJPEGToMemory(src,buffer,0,&written,90)==0          , "SaveJPEGToMemory(targetMaxSize=0) returns 0" );
  CHECK( BasicImaging_SaveJPEGToMemory(src,buffer,sizeof(buffer),0,90)==0    , "SaveJPEGToMemory(NULL bytesWritten) returns 0" );

  if (BasicImaging_HasJPEG())
  {
    int ok = BasicImaging_SaveJPEGToMemory(src,buffer,sizeof(buffer),&written,90);
    CHECK(ok,"SaveJPEGToMemory succeeds when JPEG is compiled in");
    if (ok)
    {
      CHECK( (written>=2) && (written<sizeof(buffer)) , "SaveJPEGToMemory reports a sane byte count" );
      CHECK( (buffer[0]==0xFF) && (buffer[1]==0xD8) , "encoded bytes start with the JPEG SOI magic" );

      //Round-trip : write the in-memory JPEG to disk and load it back
      FILE * f = fopen("/tmp/basicimaging_mem.jpg","wb");
      if (f!=0) { fwrite(buffer,1,written,f); fclose(f); }
      struct Image * back = BasicImaging_Load("/tmp/basicimaging_mem.jpg");
      CHECK(back!=0,"reloading the in-memory JPEG succeeds");
      if (back!=0)
      {
        CHECK( (back->width==32) && (back->height==20) , "reloaded in-memory JPEG has the right dimensions" );
        BasicImaging_Free(&back);
      }
    }

    //Too-small target buffer : the setjmp guard must turn the inevitable libjpeg error into a clean
    //return 0 - no overflow past the buffer , no exit() , no crash ( regression test for the guard )
    unsigned char small[64];
    unsigned long smallWritten=0;
    int okSmall = BasicImaging_SaveJPEGToMemory(src,small,sizeof(small),&smallWritten,90);
    CHECK(okSmall==0,"too-small target buffer returns 0 (setjmp guard fired, process still alive)");
  } else
  {
    CHECK( BasicImaging_SaveJPEGToMemory(src,buffer,sizeof(buffer),&written,90)==0 , "SaveJPEGToMemory safely fails when JPEG is NOT compiled in" );
  }

  BasicImaging_Free(&src);
}


static void testCoverResize()
{
  struct Image * wide = makeGradient(200,100,3);
  CHECK(wide!=0,"200x100 gradient for CoverResize allocates");
  if (wide!=0)
  {
    struct Image * cover = BasicImaging_CoverResize(wide,100,100);
    CHECK(cover!=0,"CoverResize 200x100 -> 100x100 succeeds");
    if (cover!=0)
    {
      CHECK( (cover->width==100) && (cover->height==100) , "CoverResize fills the exact target box" );
      //scale = max(100/200,100/100) = 1.0 -> the same-size resize fast path ( pixel-identical ) ,
      //then a centered crop at x=(200-100)/2=50 , y=0 : cover(x,y) must equal src(50+x,y) byte-exact
      unsigned int x,y,c;
      int same=1;
      for (y=0; (y<100) && same; y++)
        for (x=0; (x<100) && same; x++)
          for (c=0; (c<3) && same; c++)
            if ( cover->pixels[(y*100+x)*3+c] != wide->pixels[(y*200+50+x)*3+c] ) { same=0; }
      CHECK(same,"CoverResize center-crop matches src(50+x,y) byte-exact");
      BasicImaging_Free(&cover);
    }
    BasicImaging_Free(&wide);
  }

  struct Image * small = makeGradient(50,40,3);
  CHECK(small!=0,"50x40 gradient for upscaling CoverResize allocates");
  if (small!=0)
  {
    struct Image * cover = BasicImaging_CoverResize(small,100,100);
    CHECK(cover!=0,"CoverResize 50x40 -> 100x100 (upscale+fill) succeeds");
    if (cover!=0)
    {
      CHECK( (cover->width==100) && (cover->height==100) , "upscaled CoverResize fills the exact target box" );
      BasicImaging_Free(&cover);
    }
    BasicImaging_Free(&small);
  }

  struct Image * rgba = makeGradient(8,8,4);
  CHECK(rgba!=0,"8x8x4 image for channel-generic CoverResize allocates");
  if (rgba!=0)
  {
    struct Image * cover = BasicImaging_CoverResize(rgba,4,4);
    CHECK(cover!=0,"CoverResize 8x8x4 -> 4x4 succeeds");
    if (cover!=0)
    {
      CHECK( (cover->width==4) && (cover->height==4) && (cover->channels==4) , "4-channel CoverResize keeps its channel count" );
      BasicImaging_Free(&cover);
    }
    BasicImaging_Free(&rgba);
  }
}


//Splice an APP1/Exif segment carrying the given Orientation tag right after the SOI of an existing
//JPEG file's bytes ( in place , growing the buffer ) . Returns the new size or 0 on bad input.
static size_t spliceExifOrientationAPP1(unsigned char * jpegBytes,size_t jpegSize,unsigned int orientation,size_t maxSize)
{
  if ( (jpegBytes==0) || (jpegSize<2) || (jpegBytes[0]!=0xFF) || (jpegBytes[1]!=0xD8) ) { return 0; }

  //APP1 marker FFE1 , segment length 0x0022=34 ( including the 2 length bytes ) , payload "Exif\0\0"
  //then a little-endian TIFF : magic 42 , IFD0 at offset 8 , one entry = tag 0x0112 , type SHORT ,
  //count 1 , value = orientation
  unsigned char app1[36] = {
    0xFF,0xE1, 0x00,0x22,
    'E','x','i','f',0,0,
    'I','I', 0x2A,0x00, 0x08,0x00,0x00,0x00,
    0x01,0x00,
    0x12,0x01, 0x03,0x00, 0x01,0x00,0x00,0x00,
    (unsigned char)(orientation & 0xFF), (unsigned char)(orientation >> 8), 0x00,0x00,
    0x00,0x00,0x00,0x00
  };

  if ( jpegSize + sizeof(app1) > maxSize ) { return 0; }

  memmove(jpegBytes+2+sizeof(app1),jpegBytes+2,jpegSize-2);
  memcpy(jpegBytes+2,app1,sizeof(app1));
  return jpegSize + sizeof(app1);
}


static void testEXIFOrientation()
{
  struct Image * src = makeGradient(32,20,3);
  CHECK(src!=0,"gradient image for EXIF orientation test allocates");
  if (src==0) { return; }

  if (!BasicImaging_HasJPEG())
  {
    fprintf(stderr,"SKIP: EXIF orientation test needs JPEG support\n");
    BasicImaging_Free(&src);
    return;
  }

  CHECK( BasicImaging_SaveJPEG(src,"/tmp/basicimaging_exif_control.jpg",95) , "saving the EXIF test control JPEG" );
  BasicImaging_Free(&src);

  //Control : the plain JPEG has no EXIF , so Load must return it unchanged
  struct Image * control = BasicImaging_Load("/tmp/basicimaging_exif_control.jpg");
  CHECK(control!=0,"loading the EXIF control JPEG succeeds");
  if (control==0) { return; }
  CHECK( (control->width==32) && (control->height==20) , "no-EXIF JPEG loads with unchanged dimensions" );

  //Splice an Orientation=6 APP1 into the saved JPEG's bytes and load that : Load must auto-rotate
  FILE * f = fopen("/tmp/basicimaging_exif_control.jpg","rb");
  int spliced=0;
  if (f!=0)
  {
    fseek(f,0,SEEK_END);
    long sizeL = ftell(f);
    if ( (sizeL>0) && (fseek(f,0,SEEK_SET)==0) )
    {
      unsigned char * jpegBytes = (unsigned char *) malloc((size_t)sizeL+64);
      if (jpegBytes!=0)
      {
        size_t size = (size_t)sizeL;
        if ( fread(jpegBytes,1,size,f)==size )
        {
          size_t newSize = spliceExifOrientationAPP1(jpegBytes,size,6,size+64);
          if (newSize>0)
          {
            FILE * out = fopen("/tmp/basicimaging_exif_o6.jpg","wb");
            if (out!=0) { fwrite(jpegBytes,1,newSize,out); fclose(out); spliced=1; }
          }
        }
        free(jpegBytes);
      }
    }
    fclose(f);
  }
  CHECK(spliced,"spliced an Orientation=6 APP1 segment into the control JPEG");

  if (spliced)
  {
    struct Image * o6 = BasicImaging_Load("/tmp/basicimaging_exif_o6.jpg");
    CHECK(o6!=0,"loading the Orientation=6 JPEG succeeds");
    if (o6!=0)
    {
      CHECK( (o6->width==20) && (o6->height==32) , "Orientation=6 JPEG loads dimension-swapped (20x32)" );
      //Rotation 90 CW : o6(x,y) must equal control(col=y,row=19-x) . Both images are decodes of the
      //same JPEG stream so this is byte-exact , and the unique x-gradient distinguishes 6 from 5 / 8
      //and from a no-op.
      unsigned int x,y,c;
      int same=1;
      for (y=0; (y<o6->height) && same; y++)
        for (x=0; (x<o6->width) && same; x++)
          for (c=0; (c<3) && same; c++)
            if ( o6->pixels[((size_t)y*o6->width+x)*3+c] != control->pixels[((size_t)(19-x)*control->width + y)*3+c] ) { same=0; }
      CHECK(same,"rotated pixels match the Orientation=6 permutation byte-exact");
      BasicImaging_Free(&o6);
    }
  }

  BasicImaging_Free(&control);
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

//The SSE3/AVX2 resize paths must be BYTE-IDENTICAL to the scalar reference , not just "close" - the
//fixed-point math is fully deterministic, so any discrepancy here means one of the SIMD kernels has an
//actual bug (wrong lane order, missing tail pixels, an off-by-one in the gather/window indices, ...).
static void testSIMDPathsAgreeWithScalar()
{
  const enum BasicImaging_ResizePath candidatePaths[] = { BASICIMAGING_RESIZE_SSE3, BASICIMAGING_RESIZE_AVX2 };
  const unsigned int channelsToTry[] = {1,2,3,4};
  //Sizes deliberately NOT multiples of the 4-pixel window / 8/16-pixel y-blend batches , so every
  //path's tail handling gets exercised too.
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
  testSaveJPEGToMemory();
  testCoverResize();
  testEXIFOrientation();
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
