#ifndef IMG_WARP_H_INCLUDED
#define IMG_WARP_H_INCLUDED

#include "imaging.h"

int warpImage(struct Image * target , unsigned int posX,unsigned int posY , signed int warpDeltaX,signed int warpDeltaY );
int coolPHPWave(struct Image * target , unsigned int periodX,unsigned int periodY, signed int amplitudeX,signed int amplitudeY );

#endif // IMG_WARP_H_INCLUDED
