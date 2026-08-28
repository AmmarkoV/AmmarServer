/** @file benchmarkResize.c
* @brief Timing/profiling utility for BasicImaging_Resize() : loads real images ( committed under
*        public_html/ ) and times resizing them with each SIMD path this build/CPU actually supports,
*        reporting the difference. Also has a --profile mode meant to be run under
*        `valgrind --tool=callgrind` ( see profile.sh ) for instruction-level hotspot analysis in
*        kcachegrind.
*
*        This is a development/benchmarking tool, not part of the library : it's the ONLY thing in
*        BasicImaging allowed to #include resize_internal.h and force a specific resize path - every
*        real caller just calls BasicImaging_Resize() and gets whatever the running CPU supports best.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> //strcasecmp
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include "basicImaging.h"
#include "resize_internal.h"

#define MAX_IMAGES 512
#define MAX_PATH_LEN 1024

static double nowSeconds(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec*1e-9;
}

static int hasSuffixCI(const char * s,const char * suffix)
{
  size_t ls=strlen(s), lf=strlen(suffix);
  if (lf>ls) { return 0; }
  return (strcasecmp(s+ls-lf,suffix)==0);
}

/* Recursively collect every .jpg/.jpeg/.png path under `dir` , depth-first, into `outPaths`
 * ( caller-owned array of MAX_IMAGES char[MAX_PATH_LEN] buffers ). Returns how many were found. */
static unsigned int collectImages(const char * dir,char paths[][MAX_PATH_LEN],unsigned int maxCount,unsigned int count)
{
  DIR * dp = opendir(dir);
  if (dp==0) { return count; }

  struct dirent * entry;
  while ( ((entry=readdir(dp))!=0) && (count<maxCount) )
  {
    if (strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0) { continue; }

    char path[MAX_PATH_LEN];
    snprintf(path,sizeof(path),"%s/%s",dir,entry->d_name);

    struct stat st;
    if (stat(path,&st)!=0) { continue; }

    if (S_ISDIR(st.st_mode))
    {
      count = collectImages(path,paths,maxCount,count);
    } else
    if ( hasSuffixCI(path,".jpg") || hasSuffixCI(path,".jpeg") || hasSuffixCI(path,".png") )
    {
      snprintf(paths[count],MAX_PATH_LEN,"%s",path);
      ++count;
    }
  }

  closedir(dp);
  return count;
}


struct Workload
{
  const char * name;
  unsigned int width,height;
  int isThumbnail; //1 = call BasicImaging_Thumbnail(img,width) instead of an exact BasicImaging_Resize()
};

static const struct Workload workloads[] =
{
  //A small destination is the common "make a thumbnail" case - included for realism, even though ( as
  //the comment in resize.c explains ) a small destination means little total work regardless of path.
  { "thumbnail(200px)",  200,  200, 1 },
  //A large destination is where the SIMD paths actually have the most work to do per call, and so
  //where the difference between scalar/SSE2/AVX2 is most visible.
  { "resize->1280x720",  1280, 720, 0 },
  { "upscale->1600x1600",1600,1600, 0 },
};
#define NUM_WORKLOADS (sizeof(workloads)/sizeof(workloads[0]))


static double timePath(enum BasicImaging_ResizePath path,struct Image ** images,unsigned int imageCount,unsigned int iterations)
{
  BasicImaging_Resize_ForcePath(path);
  if (BasicImaging_Resize_ActivePath()!=path) { return -1.0; } //not available on this CPU/build

  double start = nowSeconds();

  unsigned int iter,i,w;
  for (iter=0; iter<iterations; iter++)
  {
    for (i=0; i<imageCount; i++)
    {
      for (w=0; w<NUM_WORKLOADS; w++)
      {
        struct Image * out = workloads[w].isThumbnail
                           ? BasicImaging_Thumbnail(images[i],workloads[w].width)
                           : BasicImaging_Resize(images[i],workloads[w].width,workloads[w].height);
        BasicImaging_Free(&out);
      }
    }
  }

  double elapsed = nowSeconds()-start;
  BasicImaging_Resize_ForcePath(BASICIMAGING_RESIZE_AUTO);
  return elapsed;
}


static void runProfileMode(struct Image ** images,unsigned int imageCount,enum BasicImaging_ResizePath path,unsigned int iterations)
{
  BasicImaging_Resize_ForcePath(path);
  enum BasicImaging_ResizePath active = BasicImaging_Resize_ActivePath();
  fprintf(stderr,"profile mode : requested %s , actually running %s , %u iteration(s) over %u image(s)\n",
          BasicImaging_Resize_PathName(path),BasicImaging_Resize_PathName(active),iterations,imageCount);

  unsigned int iter,i,w;
  for (iter=0; iter<iterations; iter++)
  {
    for (i=0; i<imageCount; i++)
    {
      for (w=0; w<NUM_WORKLOADS; w++)
      {
        struct Image * out = workloads[w].isThumbnail
                           ? BasicImaging_Thumbnail(images[i],workloads[w].width)
                           : BasicImaging_Resize(images[i],workloads[w].width,workloads[w].height);
        BasicImaging_Free(&out);
      }
    }
  }
}


static enum BasicImaging_ResizePath parsePathName(const char * s)
{
  if (strcasecmp(s,"scalar")==0) { return BASICIMAGING_RESIZE_SCALAR; }
  if (strcasecmp(s,"sse2")==0)   { return BASICIMAGING_RESIZE_SSE2; }
  if (strcasecmp(s,"avx2")==0)   { return BASICIMAGING_RESIZE_AVX2; }
  return BASICIMAGING_RESIZE_AUTO;
}


int main(int argc,char ** argv)
{
  const char * imageDir = "../../public_html";
  unsigned int iterations = 20;
  int iterationsSet = 0;
  int profileMode = 0;
  enum BasicImaging_ResizePath profilePath = BASICIMAGING_RESIZE_AUTO;

  int i;
  for (i=1; i<argc; i++)
  {
    if (strncmp(argv[i],"--dir=",6)==0)         { imageDir = argv[i]+6; } else
    if (strncmp(argv[i],"--iterations=",13)==0) { iterations = (unsigned int) atoi(argv[i]+13); iterationsSet=1; } else
    if (strncmp(argv[i],"--profile=",10)==0)    { profileMode=1; profilePath=parsePathName(argv[i]+10); } else
    if (strcmp(argv[i],"--profile")==0)         { profileMode=1; profilePath=BASICIMAGING_RESIZE_AUTO; } else
    if (strcmp(argv[i],"--help")==0)
    {
      printf("usage: %s [--dir=PATH] [--iterations=N] [--profile[=scalar|sse2|avx2]]\n",argv[0]);
      printf("  --dir=PATH        directory of .jpg/.jpeg/.png images to benchmark (default: %s)\n",imageDir);
      printf("  --iterations=N    repeat the full workload matrix N times per path (default: %u\n",iterations);
      printf("                    in normal mode , 1 in --profile mode unless given explicitly)\n");
      printf("  --profile[=PATH]  run once under a single forced path with no report - meant to be\n");
      printf("                    invoked under `valgrind --tool=callgrind` , see profile.sh\n");
      return 0;
    }
  }

  //A callgrind profile run is only interesting for WHERE time goes, not for throughput : under
  //valgrind's ~20x slowdown the normal 20-iteration default would take several minutes per image set.
  if (profileMode && !iterationsSet) { iterations = 1; }

  static char paths[MAX_IMAGES][MAX_PATH_LEN];
  unsigned int imageCount = collectImages(imageDir,paths,MAX_IMAGES,0);
  if (imageCount==0)
  {
    fprintf(stderr,"No .jpg/.jpeg/.png images found under `%s` - pass --dir=PATH to point at some.\n",imageDir);
    return 1;
  }

  struct Image * images[MAX_IMAGES] = {0};
  unsigned int loaded=0;
  for (i=0; i<(int)imageCount; i++)
  {
    struct Image * img = BasicImaging_Load(paths[i]);
    if (img!=0) { images[loaded++] = img; }
    else { fprintf(stderr,"warning: could not decode %s , skipping\n",paths[i]); }
  }

  if (loaded==0)
  {
    fprintf(stderr,"None of the %u images under `%s` could be decoded ( JPEG support=%d , PNG support=%d )\n",
            imageCount,imageDir,BasicImaging_HasJPEG(),BasicImaging_HasPNG());
    return 1;
  }

  if (profileMode)
  {
    runProfileMode(images,loaded,profilePath,iterations);
    for (i=0; i<(int)loaded; i++) { BasicImaging_Free(&images[i]); }
    return 0;
  }

  printf("BasicImaging resize benchmark\n");
  printf("  images loaded   : %u (from %u found under %s)\n",loaded,imageCount,imageDir);
  printf("  workload matrix : %u resize op(s) per image per iteration\n",(unsigned int)NUM_WORKLOADS);
  printf("  iterations      : %u\n\n",iterations);

  static const enum BasicImaging_ResizePath allPaths[] =
    { BASICIMAGING_RESIZE_SCALAR, BASICIMAGING_RESIZE_SSE2, BASICIMAGING_RESIZE_AVX2 };

  double elapsed[3];
  int available[3];
  unsigned int p;
  for (p=0; p<3; p++)
  {
    elapsed[p] = timePath(allPaths[p],images,loaded,iterations);
    available[p] = (elapsed[p]>=0.0);
  }

  double scalarTime = available[0] ? elapsed[0] : -1.0;

  printf("%-10s %12s %16s %10s\n","path","total sec","resizes/sec","speedup");
  printf("%-10s %12s %16s %10s\n","----","---------","-----------","-------");
  for (p=0; p<3; p++)
  {
    const char * name = BasicImaging_Resize_PathName(allPaths[p]);
    if (!available[p]) { printf("%-10s %12s %16s %10s\n",name,"-","-","not available"); continue; }

    double totalResizes = (double)iterations*(double)loaded*(double)NUM_WORKLOADS;
    double perSec = totalResizes/elapsed[p];
    if (scalarTime>0.0)
    {
      printf("%-10s %12.4f %16.1f %9.2fx\n",name,elapsed[p],perSec,scalarTime/elapsed[p]);
    } else
    {
      printf("%-10s %12.4f %16.1f %10s\n",name,elapsed[p],perSec,"n/a");
    }
  }

  for (i=0; i<(int)loaded; i++) { BasicImaging_Free(&images[i]); }
  return 0;
}
