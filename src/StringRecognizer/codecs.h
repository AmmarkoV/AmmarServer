
/** @file codecs.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef CODECS_H_INCLUDED
#define CODECS_H_INCLUDED


/** @brief Enumerator for the IDs of codecs so we can know what the result was*/
enum { 
 CODECS_EMPTY=0,
 CODECS_JPG, // 1 
 CODECS_PNG, // 2 
 CODECS_JPEG, // 3 
 CODECS_PNM, // 4 
 CODECS_BMP, // 5 
 CODECS_RLE, // 6 
 CODECS_CPNM, // 7 
 CODECS_PPM, // 8 
 CODECS_ASCII, // 9 
 CODECS_TEXT, // 10 
 CODECS_TXT, // 11 
 CODECS_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the codecs word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_codecs(const char * str,unsigned int strLength); 

#endif
