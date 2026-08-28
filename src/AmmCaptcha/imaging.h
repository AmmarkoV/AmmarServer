#ifndef _IMAGING_H_INCLUDED
#define _IMAGING_H_INCLUDED

//The local struct Image ( pixels,width,height,depth,imageSize ) was replaced by BasicImaging's
//( pixels,width,height,channels,bitsperpixel,image_size ) so the two headers can coexist in one
//translation unit and AmmCaptcha can encode through BasicImaging_SaveJPEGToMemory().
#include "../BasicImaging/basicImaging.h"

//These are AmmCaptcha_-prefixed because the old plain names ( createImage , destroyImage , ReadPPM ,
//WritePPM , copyImage , createImageUsingExistingBuffer ) collide at link time with the same-named
//globals inside BasicImaging's vendored codecs library , which AmmCaptcha now links. The two that had
//no remaining callers after the JPEG-pipeline migration ( copyImage , createImageUsingExistingBuffer )
//were removed outright.
struct Image * AmmCaptcha_createImage(unsigned int width,unsigned int height,unsigned int depth);
int AmmCaptcha_destroyImage(struct Image * source);


int bitBltImage(struct Image * target , unsigned int targetX,unsigned int targetY ,
                struct Image * source , unsigned int sourceX,unsigned int sourceY ,  unsigned int width , unsigned int height );

int AmmCaptcha_readPPM(struct Image * pic,char * filename,char read_only_header);
int AmmCaptcha_writePPM(struct Image * pic,char * filename);


#endif
