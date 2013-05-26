#include "imaging.h"
#include <math.h>
#include <stdlib.h>     /* srand, rand */
/*
 * 1.0 +----+----+----+----+
 *     |    |    |    |    |
 *     +----+----+----+----+
 *     |    |    |    |    |
 * 0.5 +----+----+----+----+
 *     |    |    |    |    |
 *     +----+----+----+----+
 *     |    |    |    |    |
 * 0.0 +----+----+----+----+
 *    0.0       0.5       1.0
*/

#define ABS(num1)  ( (num1) >=0 ? (num1) : (-1*num1) )
#define ABSDIFF(num1,num2)  ( (num1-num2) >=0 ? (num1-num2) : (num2 - num1) )

int warpImage(struct Image * target , unsigned int posX,unsigned int posY , signed int warpDeltaX,signed int warpDeltaY )
{
 if (target==0)  { return 0; }
 if (target->pixels==0)  { return 0; }

 unsigned int dimX = ABS(warpDeltaX-posX); dimX=dimX*dimX;
 unsigned int dimY = ABS(warpDeltaY-posY); dimY=dimY*dimY;
 unsigned int distance = sqrt( dimX + dimY );

 unsigned int width =0  , height =0;
 unsigned int targetWidthStep = target->width * 3;
 char * targetPixelsStart   = (char*) target->pixels + ( (posX*3) + posY * targetWidthStep );
 char * targetPixelsLineEnd = targetPixelsStart + (width*3);
 char * targetPixelsEnd     = targetPixelsLineEnd + ((height-1) * targetWidthStep );
 char * targetPixels = targetPixelsStart;

 struct Image * copy = copyImage(target);
 destroyImage(copy);
 return 1;
}



/**
 * This is the PHP script from Jose Rodriguez , currently used as a swirling mechanism , it is GPLv3 as this project :)
 *
 * @author  Jose Rodriguez <jose.rodriguez@exec.cl>
 * @license GPLv3
 * @link    http://code.google.com/p/cool-php-captcha
 * @package captcha
 * @version 0.3
 *
 */
int coolPHPWave(struct Image * target , unsigned int periodX,unsigned int periodY, signed int amplitudeX,signed int amplitudeY )
{
 if (target==0)  { return 0; }
 if (target->pixels==0)  { return 0; }

 float scale=1.0;
 float sinPart;
 float res;
 unsigned int randThree= 1+ rand()%3;
 unsigned int randTwo= 1+ rand()%2;
 float xp = scale * periodX * randThree;
 unsigned int k= (unsigned int) rand()%100;
 unsigned int i=0;

 for (i=0; i<(target->width*scale); i++)
 {
  sinPart = (float) (k+i);
  sinPart = (float) sinPart/xp;
  res = (float) sin(sinPart)  * (float) scale * (float) amplitudeX;
  bitBltImage(target, i-1 , (unsigned int) res ,
              target , i, 0 , 1  , (unsigned int) target->height * scale);
 }


 float yp = scale * periodY * randTwo;
 k= (unsigned int) rand()%100;
 for (i=0; i<(target->height*scale); i++)
 {
  sinPart = (float) (k+i);
  sinPart = (float) sinPart/yp;
  res = (float) sin(sinPart) * scale * amplitudeY;
  bitBltImage(target, (unsigned int) res , i-1 ,
              target , 0 , i , (unsigned int) target->width * scale , 1 );
 }

 return 1;
}

/*

    public $Yperiod    = 12;
    public $Yamplitude = 14;
    public $Xperiod    = 11;
    public $Xamplitude = 5;
   FROM -> cool-php-captcha-0.3.1
//bool imagecopy ( resource $dst_im , resource $src_im , int $dst_x , int $dst_y , int $src_x , int $src_y , int $src_w , int $src_h )

    protected function WaveImage() {
        // X-axis wave generation
        $xp = $this->scale*$this->Xperiod*rand(1,3);
        $k = rand(0, 100);
        for ($i = 0; $i < ($this->width*$this->scale); $i++) {
            imagecopy($this->im, $this->im,
                $i-1, sin($k+$i/$xp) * ($this->scale*$this->Xamplitude),
                $i, 0, 1, $this->height*$this->scale);
        }

        // Y-axis wave generation
        $k = rand(0, 100);
        $yp = $this->scale*$this->Yperiod*rand(1,2);
        for ($i = 0; $i < ($this->height*$this->scale); $i++) {
            imagecopy($this->im, $this->im,
                sin($k+$i/$yp) * ($this->scale*$this->Yamplitude), $i-1,
                0, $i, $this->width*$this->scale, 1);
        }
    }




*/
