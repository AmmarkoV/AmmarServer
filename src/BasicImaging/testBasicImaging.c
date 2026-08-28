#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "basicImaging.h"

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


int main()
{
  testResizeBasics();
  testBadInputsNeverCrash();
  testCodecRoundTrip();
  testThumbnailFileFallback();

  if (failures==0)
  {
    fprintf(stderr,"\nAll BasicImaging tests passed.\n");
    return 0;
  }

  fprintf(stderr,"\n%u BasicImaging test(s) FAILED.\n",failures);
  return 1;
}
