#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imaging.h"

unsigned int fontX = 19 , fontY = 22;

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

int main()
{
    struct Image * captcha = createImage(640,480,3);
    struct Image fontRAW={0};
    ReadPPM("font.ppm",&fontRAW,0);


    RenderString(captcha,&fontRAW, 0 ,  30, "abcdefghijklmnopqrstuvwxyz");
    RenderString(captcha,&fontRAW, 0 ,  50, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    RenderString(captcha,&fontRAW, 0 ,  70, "0123456789");

    RenderString(captcha,&fontRAW, 0 ,  90, "ABCDTest123");

    RenderString(captcha,&fontRAW, 0 ,  120, "012345Test123");


    RenderString(captcha,&fontRAW, 0 ,  120, "AmmarServer FTW");

    WritePPM("captcha.ppm",captcha);

    return 0;
}
