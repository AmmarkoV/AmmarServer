#include "imaging.h"
#include <math.h>

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


int warpImage(struct Image * target , unsigned int posX,unsigned int posY , signed int warpDeltaX,signed int warpDeltaY )
{
 if (target==0)  { return 0; }
 if (target->pixels==0)  { return 0; }

 unsigned int dimX = abs(warpDeltaX-posX); dimX=dimX*dimX;
 unsigned int dimY = abs(warpDeltaY-posY); dimY=dimY*dimY;
 unsigned int distance = sqrt( dimX + dimY );

 unsigned int width  , height ;
 unsigned int targetWidthStep = target->width * 3;
 char * targetPixelsStart   = target->pixels + ( (posX*3) + posY * targetWidthStep );
 char * targetPixelsLineEnd = targetPixelsStart + (width*3);
 char * targetPixelsEnd     = targetPixelsLineEnd + ((height-1) * targetWidthStep );
 char * targetPixels = targetPixelsStart;

 struct Image * copy = copyImage(target);
 destroyImage(copy);

}
