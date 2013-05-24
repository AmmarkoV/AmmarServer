#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imaging.h"

unsigned int fontX = 10 , fontY = 22;
unsigned int startOfLowercase = 0;
unsigned int startOfCapital = 26;
unsigned int startOfZero = 52;
                                       //26                        //52
char fontText[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";


int RenderString(struct Image * frame ,struct Image * font, unsigned int x,  unsigned int y , char * str)
{
  unsigned int i=0;
  unsigned int iLen=strlen(str);
  unsigned int drift=0;
  while ( i<iLen )
  {
    if ( (str[i]>='0') && (str[i]<='9') )  {  drift=str[i]-'0';  bitBltImage(frame,x,y,font,(startOfZero+drift)*fontX      ,0, fontX , fontY ); } else
    if ( (str[i]>='a') && (str[i]<='z') )  {  drift=str[i]-'a';  bitBltImage(frame,x,y,font,(startOfLowercase+drift)*fontX ,0, fontX , fontY ); } else
    if ( (str[i]>='A') && (str[i]<='Z') )  {  drift=str[i]-'A';  bitBltImage(frame,x,y,font,(startOfCapital+drift)*fontX   ,0, fontX , fontY ); }
    x+=fontX;
    ++i;
  }

  return 1;
}

int main()
{
    struct Image * captcha = createImage(640,480,3);
    struct Image fontRAW={0};
    ReadPPM("font.ppm",&fontRAW,0);



    if ( ! bitBltImage(captcha,30,30,&fontRAW,60,0, 10, 22) )
    {
      fprintf(stderr,"Could not bitBlt\n");
    }

    RenderString(captcha,&fontRAW, 80 ,  50, "abcdTest123");

    RenderString(captcha,&fontRAW, 80 ,  80, "ABCDTest123");

    RenderString(captcha,&fontRAW, 80 ,  120, "012345Test123");

    WritePPM("captcha.ppm",captcha);

    return 0;
}
