#ifndef IMAGEFILES_H_INCLUDED
#define IMAGEFILES_H_INCLUDED


enum { 
 IMAGEFILES_EMPTY=0,
 IMAGEFILES_GIF,
 IMAGEFILES_PNG,
 IMAGEFILES_JPG,
 IMAGEFILES_JPEG,
 IMAGEFILES_WEBP,
 IMAGEFILES_BMP,
 IMAGEFILES_TIFF,
 IMAGEFILES_DIB,
 IMAGEFILES_RLE,
 IMAGEFILES_J2C,
 IMAGEFILES_ICO,
 IMAGEFILES_PPM,
 IMAGEFILES_PNM,
 IMAGEFILES_RAW,
 IMAGEFILES_SVG,
 IMAGEFILES_END_OF_ITEMS
};



int scanFor_imageFiles(char * str,unsigned int strLength); 

#endif