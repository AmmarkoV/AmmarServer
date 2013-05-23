#include <stdio.h>
#include <stdlib.h>
#include "imaging.h"

char fontText[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";


int main()
{
    struct Image fontRAW={0};
    ReadPPM("font.ppm",&fontRAW,0);


    struct Image * captcha = createImage(640,480,3);
    WritePPM("captcha.ppm",captcha);

    return 0;
}
