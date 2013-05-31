#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "imaging.h"
#include "img_warp.h"
#include "jpgInput.h"

//Difficulty For Crackers ++ , DDOS Problems ++
#define RANDOMIZE_AFTER_FAILED_ATTEMPT 1
//This is "potentially dangerous" for a DDos attack that might constantly invalidate captchaIDs
//changing the actual IDs for  2/32889 every time


#include "../AmmServerlib/hashmap/hashmap.h"

unsigned int fontX = 19 , fontY = 22;
struct Image fontRAW={0};
struct hashMap * captchaStrings=0;

int RenderString(struct Image * frame ,struct Image * font, unsigned int x,  unsigned int y , char * str)
{
  if ( (frame==0)||(font==0) ) { return 0;}
  if ( (frame->pixels==0)||(font->pixels==0) ) { return 0;}

  unsigned int i=0;
  unsigned int iLen=strlen(str);
  unsigned int drift=10 , column = 35;
  while ( i<iLen )
  {
    drift = 10; column = 35;
    if ( (str[i]>='a') && (str[i]<='z') )  {  drift=str[i]-'a';  column = 1; } else
    if ( (str[i]>='A') && (str[i]<='Z') )  {  drift=str[i]-'A';  column = 18; } else
    if ( (str[i]>='0') && (str[i]<='9') )  {  drift=str[i]-'0';  column = 35;  }
    bitBltImage(frame,x,y,font, column , drift*fontY , fontX , fontY );

    x+=fontX-3;
    ++i;
  }

  return 1;
}

unsigned int convertExternalIDToInternal(unsigned int captchaID)
{
    return captchaID % hashMap_GetCurrentNumberOfEntries(captchaStrings);
}


int AmmCaptcha_isReplyCorrect(unsigned int captchaID, char * reply)
{
   if ( strcmp(reply,hashMap_GetKeyAtIndex(captchaStrings,convertExternalIDToInternal(captchaID))) == 0 )
   {
     //We have a winner!! , Lets make this captchaID invalid!
     unsigned int randomString = rand()%hashMap_GetCurrentNumberOfEntries(captchaStrings);
     hashmap_SwapRecords(captchaStrings,convertExternalIDToInternal(captchaID),randomString);
     return 1;
   }

   #if RANDOMIZE_AFTER_FAILED_ATTEMPT
     unsigned int randomString = rand()%hashMap_GetCurrentNumberOfEntries(captchaStrings);
     hashmap_SwapRecords(captchaStrings,convertExternalIDToInternal(captchaID),randomString);
   #endif // RANDOMIZE_AFTER_FAILED_ATTEMPT

   return 0;
}

int AmmCaptcha_getCaptchaFrame(unsigned int captchaID, char *mem,unsigned long * mem_size)
{
  struct Image * captcha = createImage(300,70,3);
  RenderString(captcha,&fontRAW, 10 ,  20, hashMap_GetKeyAtIndex(captchaStrings,convertExternalIDToInternal(captchaID)));

  //Apply Swirling effect!
  coolPHPWave(captcha, 11,12,5,14);

  //WriteJPEGFile(captcha,"captcha.jpg");
  WriteJPEGMemory(captcha,mem,mem_size);
  fprintf(stderr,"Survived WriteJPEG");
  destroyImage(captcha);
  fprintf(stderr,"Survived destroyImage");
  return 1;
}


int AmmCaptcha_copyCaptchaJPEGImageWithCopy(unsigned int captchaID, char *mem,unsigned long * mem_size)
{
  unsigned long frameLength = *mem_size; //10KB more than enough
  char * captchaFrame = (char *) malloc(sizeof(char) * frameLength);
  if (captchaFrame!=0)
  {
   AmmCaptcha_getCaptchaFrame(captchaID,captchaFrame,&frameLength);
   fprintf(stderr,"Copying back %lu bytes of captcha.jpg\n",frameLength);
   memcpy(mem,captchaFrame,sizeof(char) * frameLength);
   fprintf(stderr,"Survived , marking frameLength as %lu \n",frameLength);
   *mem_size=frameLength;
   free(captchaFrame);
  } else
  {
   fprintf(stderr,"Could not allocate frame for captcha image ( size %lu ) \n",frameLength);
  }
}


int AmmCaptcha_loadDictionary(char * dictFilename)
{
 captchaStrings = hashMap_Create(32889,1000,0);
 if (captchaStrings==0) { fprintf(stderr,"Could not loadDictionary %s \n",dictFilename); return 0; }
 FILE *fd=0;
 fd = fopen(dictFilename,"r");
 if (fd!=0)
 {
  char str[100]={0};
  while (!feof(fd))
  {
    fscanf (fd, "%s", str);
    hashMap_Add(captchaStrings,str,0,0);
  }
  fclose(fd);
  return 1;
 }
 return 0;
}



int AmmCaptcha_initialize(char * font,char * dictFilename)
{
  int retres=0;
  if (font==0) { retres=ReadPPM(&fontRAW,"font.ppm",0); } else
               { retres=ReadPPM(&fontRAW,font,0); }
  if (!retres) { fprintf(stderr,"Could not read font for captcha system\n"); return 0; }

  srand (time(NULL));
  return AmmCaptcha_loadDictionary(dictFilename);
}


int AmmCaptcha_destroy()
{
  hashMap_Destroy(captchaStrings);

  //Font RAW is a stack variable , do not destroy it!
  //destroyImage(&fontRAW);
  if(fontRAW.pixels!=0) { free(fontRAW.pixels); fontRAW.pixels=0; }
}


int testAmmCaptcha()
{
    AmmCaptcha_initialize("font.ppm","ourDictionaryCaptcha.txt");

    struct Image * captcha = createImage(300,70,3);

    RenderString(captcha,&fontRAW, 0 ,  20, "AmmarServer FTW");
    WritePPM(captcha,"captcha.ppm");

    coolPHPWave(captcha, 11,12,5,14);
    WriteJPEGFile(captcha,"captcha.jpg");

   /*
    RenderString(captcha,&fontRAW, 0 ,  30, "abcdefghijklmnopqrstuvwxyz");
    RenderString(captcha,&fontRAW, 0 ,  50, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    RenderString(captcha,&fontRAW, 0 ,  70, "0123456789");

    RenderString(captcha,&fontRAW, 0 ,  90, "ABCDTest123");

    RenderString(captcha,&fontRAW, 0 ,  120, "012345Test123");*/



    //warpImage(captcha,  40, 120 ,  60 , 150);



    AmmCaptcha_destroy();

    return 0;
}
