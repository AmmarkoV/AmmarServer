/** @file imageFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef IMAGEFILES_H_INCLUDED
#define IMAGEFILES_H_INCLUDED


/** @brief Enumerator for the IDs of imageFiles so we can know what the result was*/
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



/** @brief Scan a string for one of the words of the imageFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_imageFiles(char * str,unsigned int strLength); 

#endif
