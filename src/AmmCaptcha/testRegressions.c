//Regression tests for AmmCaptcha
//   ./testammcaptcha [path/to/font.ppm]   ( default src/AmmCaptcha/font.ppm , i.e. run from the repository root )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "AmmCaptcha.h"
#include "imaging.h"
#include "img_warp.h"
#include "../Hashmap/hashmap.h"

extern struct hashMap * captchaStrings;
int AmmCaptcha_loadDictionary(char * dictFilename);

static unsigned int failures=0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"FAIL %s:%d : %s\n",__FILE__,__LINE__,#cond); ++failures; } } while(0)

static char tempDir[64];
static char * tempPath(const char * name)
{
  static char path[128];
  snprintf(path,sizeof(path),"%s/%s",tempDir,name);
  return path;
}

static void writeFile(const char * name,const void * data,size_t length)
{
  FILE * fp = fopen(tempPath(name),"wb");
  if (fp==0) { perror("fopen"); exit(2); }
  fwrite(data,1,length,fp);
  fclose(fp);
}

//Using the captcha before ( or after ) it is initialized used to divide by zero and kill the server
static void test_uninitialized()
{
  char jpeg[16];
  unsigned long size = sizeof(jpeg);
  CHECK(AmmCaptcha_isReplyCorrect(5,"anything")==0);
  CHECK(AmmCaptcha_getCaptchaFrame(5,jpeg,&size)==0);
}

//feof() looping added the last word twice , and %s overflowed a 256 byte buffer on long words
static void test_dictionary()
{
  const char dict[] = "alpha\nbeta\ngamma\n";
  writeFile("dict.txt",dict,sizeof(dict)-1);
  CHECK(AmmCaptcha_loadDictionary(tempPath("dict.txt")));
  CHECK(hashMap_GetCurrentNumberOfEntries(captchaStrings)==3);
  CHECK(strcmp(hashMap_GetKeyAtIndex(captchaStrings,2),"gamma")==0);

  char longWord[402];
  memset(longWord,'w',400); longWord[400]='\n'; longWord[401]=0;
  writeFile("long.txt",longWord,401);
  CHECK(AmmCaptcha_loadDictionary(tempPath("long.txt"))); //Reloading also frees the previous dictionary
  CHECK(hashMap_GetCurrentNumberOfEntries(captchaStrings)>=1);

  writeFile("empty.txt","",0);
  CHECK(AmmCaptcha_loadDictionary(tempPath("empty.txt")));
  CHECK(AmmCaptcha_isReplyCorrect(0,"")==0); //An empty dictionary is "not ready" , not a division by zero

  CHECK(!AmmCaptcha_loadDictionary(tempPath("missing.txt")));
  CHECK(AmmCaptcha_isReplyCorrect(0,"alpha")==0);
}

//"%u\n" swallowed pixel bytes that look like whitespace , and a truncated file leaked its buffer
static void test_readPPM()
{
  const unsigned char ppm[] = { 'P','6','\n','2',' ','1','\n','2','5','5','\n', ' ','\n',7, 8,9,10 };
  writeFile("ws.ppm",ppm,sizeof(ppm));
  struct Image pic; memset(&pic,0,sizeof(pic));
  CHECK(AmmCaptcha_readPPM(&pic,tempPath("ws.ppm"),0));
  CHECK( (pic.pixels!=0) && (pic.pixels[0]==' ') && (pic.pixels[1]=='\n') && (pic.pixels[2]==7) && (pic.pixels[5]==10) );
  free(pic.pixels);

  writeFile("short.ppm",ppm,sizeof(ppm)-3);
  memset(&pic,0,sizeof(pic));
  CHECK(!AmmCaptcha_readPPM(&pic,tempPath("short.ppm"),0));
  CHECK(pic.pixels==0);

  const char zero[] = "P6\n0 0\n255\n";
  writeFile("zero.ppm",zero,sizeof(zero)-1);
  memset(&pic,0,sizeof(pic));
  CHECK(!AmmCaptcha_readPPM(&pic,tempPath("zero.ppm"),0));
}

//The wave effect cast negative floats to unsigned ( undefined ) , run it on many seeds under UBSan builds
static void test_wave()
{
  unsigned int seed;
  for (seed=1; seed<50; seed++)
  {
    srand(seed);
    struct Image * img = AmmCaptcha_createImage(300,70,3);
    CHECK( (img!=0) && (img->pixels!=0) );
    if ( (img==0) || (img->pixels==0) ) { return; }
    CHECK(coolPHPWave(img,11,12,5,14));
    AmmCaptcha_destroyImage(img);
  }
}

static void test_fullCycle(const char * font)
{
  const char dict[] = "Ammar\nServer\nCaptcha\n";
  writeFile("cycle.txt",dict,sizeof(dict)-1);
  CHECK(AmmCaptcha_initialize((char*)font,tempPath("cycle.txt")));

  static char jpeg[64*1024];
  unsigned long size = sizeof(jpeg);
  CHECK(AmmCaptcha_getCaptchaFrame(7,jpeg,&size));
  CHECK( (size>4) && (size<sizeof(jpeg)) && ((unsigned char)jpeg[0]==0xFF) && ((unsigned char)jpeg[1]==0xD8) );

  size = 8; //Far too small for a JPEG , must fail instead of overflowing
  CHECK(!AmmCaptcha_getCaptchaFrame(7,jpeg,&size));

  CHECK(AmmCaptcha_isReplyCorrect(7,0)==0);
  CHECK(AmmCaptcha_isReplyCorrect(7,"definitely wrong")==0);
  char answer[64];
  snprintf(answer,sizeof(answer),"%s",hashMap_GetKeyAtIndex(captchaStrings,7%3));
  CHECK(AmmCaptcha_isReplyCorrect(7,answer)==1);

  CHECK(AmmCaptcha_destroy());
  CHECK(captchaStrings==0);
  CHECK(AmmCaptcha_isReplyCorrect(7,answer)==0); //Used to be a use-after-free
  size = sizeof(jpeg);
  CHECK(AmmCaptcha_getCaptchaFrame(7,jpeg,&size)==0);

  CHECK(AmmCaptcha_initialize((char*)font,tempPath("cycle.txt"))); //Re-initializing after destroy works
  size = sizeof(jpeg);
  CHECK(AmmCaptcha_getCaptchaFrame(1,jpeg,&size));
  CHECK(AmmCaptcha_destroy());
}

int main(int argc,char * argv[])
{
  const char * font = (argc>1) ? argv[1] : "src/AmmCaptcha/font.ppm";
  strcpy(tempDir,"/tmp/ammcaptchatestXXXXXX");
  if (mkdtemp(tempDir)==0) { perror("mkdtemp"); return 2; }

  test_uninitialized();
  test_dictionary();
  test_readPPM();
  test_wave();
  if (access(font,R_OK)==0) { test_fullCycle(font); } else { fprintf(stderr,"FAIL : font %s not found ( run from the repository root )\n",font); ++failures; }
  AmmCaptcha_destroy();

  const char * files[] = { "dict.txt","long.txt","empty.txt","ws.ppm","short.ppm","zero.ppm","cycle.txt",0 };
  for (unsigned int i=0; files[i]!=0; i++) { unlink(tempPath(files[i])); }
  rmdir(tempDir);

  if (failures) { fprintf(stderr,"AmmCaptcha tests : %u failures\n",failures); return 1; }
  fprintf(stderr,"AmmCaptcha tests : all passed\n");
  return 0;
}
