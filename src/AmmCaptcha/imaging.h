#ifndef _IMAGING_H_INCLUDED
#define _IMAGING_H_INCLUDED

struct Image
{
  unsigned char * pixels;
  unsigned int width;
  unsigned int height;
  unsigned int depth;
  unsigned int imageSize;
};

struct Image * createImageUsingExistingBuffer( unsigned int width , unsigned int height , unsigned int channels , unsigned int bitsPerPixel , unsigned char * pixels);
struct Image * createImage(unsigned int width,unsigned int height,unsigned int depth);
struct Image * copyImage(struct Image * source);
int destroyImage(struct Image * source);


int bitBltImage(struct Image * target , unsigned int targetX,unsigned int targetY ,
                struct Image * source , unsigned int sourceX,unsigned int sourceY ,  unsigned int width , unsigned int height );

int ReadPPM(struct Image * pic,char * filename,char read_only_header);
int WritePPM(struct Image * pic,char * filename);


#endif
