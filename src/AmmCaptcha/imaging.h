#ifndef _IMAGING_H_INCLUDED
#define _IMAGING_H_INCLUDED

struct Image
{
  unsigned char * pixels;
  unsigned int width;
  unsigned int height;
  unsigned int depth;
  unsigned int image_size;
};

struct Image * createImage(unsigned int width,unsigned int height,unsigned int depth);

int bitBltImage(struct Image * target , unsigned int targetX,unsigned int targetY ,
                struct Image * source , unsigned int sourceX,unsigned int sourceY ,  unsigned int width , unsigned int height );

int ReadPPM(char * filename,struct Image * pic,char read_only_header);
int WritePPM(char * filename,struct Image * pic);


#endif
