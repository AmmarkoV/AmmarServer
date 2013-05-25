#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imaging.h"
#include "img_warp.h"
#include "jpgInput.h"


unsigned int fontX = 19 , fontY = 22;
struct Image fontRAW={0};


int RenderString(struct Image * frame ,struct Image * font, unsigned int x,  unsigned int y , char * str)
{
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

    x+=fontX-2;
    ++i;
  }

  return 1;
}


char * AmmCaptcha_getCaptchaFrame(unsigned int captchaID, unsigned int * frameLength)
{
  struct Image * captcha = createImage(300,60,3);
  RenderString(captcha,&fontRAW, 10 ,  20, "AmmarServer FTW");

  char * retFrame = fontRAW.pixels;
  *frameLength = fontRAW.imageSize;
  free(captcha);
}

int AmmCaptcha_initialize(char * font)
{
  if (font==0) { return ReadPPM(&fontRAW,"font.ppm",0); } else
               { return ReadPPM(&fontRAW,font,0); }
  return 0;
}


int testAmmCaptcha()
{
    struct Image * captcha = createImage(300,60,3);


    RenderString(captcha,&fontRAW, 10 ,  20, "AmmarServer FTW");

   /*
    RenderString(captcha,&fontRAW, 0 ,  30, "abcdefghijklmnopqrstuvwxyz");
    RenderString(captcha,&fontRAW, 0 ,  50, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    RenderString(captcha,&fontRAW, 0 ,  70, "0123456789");

    RenderString(captcha,&fontRAW, 0 ,  90, "ABCDTest123");

    RenderString(captcha,&fontRAW, 0 ,  120, "012345Test123");*/



    warpImage(captcha,  40, 120 ,  60 , 150);


    WriteJPEGFile(captcha,"captcha.jpg");


    WritePPM(captcha,"captcha.ppm");

    return 0;
}
