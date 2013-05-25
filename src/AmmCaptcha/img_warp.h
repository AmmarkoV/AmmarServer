#ifndef IMG_WARP_H_INCLUDED
#define IMG_WARP_H_INCLUDED

#include "imaging.h"

int warpImage(struct Image * target , unsigned int posX,unsigned int posY , signed int warpDeltaX,signed int warpDeltaY );

#endif // IMG_WARP_H_INCLUDED
