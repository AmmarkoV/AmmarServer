
/** @file imageFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef IMAGEFILES_H_INCLUDED
#define IMAGEFILES_H_INCLUDED


/** @brief Enumerator for the IDs of imageFiles so we can know what the result was*/
enum { 
 IMAGEFILES_EMPTY=0,
 IMAGEFILES_GIF, // 1 
 IMAGEFILES_PNG, // 2 
 IMAGEFILES_JPG, // 3 
 IMAGEFILES_JPEG, // 4 
 IMAGEFILES_WEBP, // 5 
 IMAGEFILES_BMP, // 6 
 IMAGEFILES_TIFF, // 7 
 IMAGEFILES_DIB, // 8 
 IMAGEFILES_RLE, // 9 
 IMAGEFILES_J2C, // 10 
 IMAGEFILES_ICO, // 11 
 IMAGEFILES_PPM, // 12 
 IMAGEFILES_PNM, // 13 
 IMAGEFILES_RAW, // 14 
 IMAGEFILES_SVG, // 15 
 IMAGEFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the imageFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_imageFiles(const char * str,unsigned int strLength); 

#endif
