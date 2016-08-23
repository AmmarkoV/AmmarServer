 /** @file videoFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef VIDEOFILES_H_INCLUDED
#define VIDEOFILES_H_INCLUDED


/** @brief Enumerator for the IDs of videoFiles so we can know what the result was*/
enum { 
 VIDEOFILES_EMPTY=0,
 VIDEOFILES_AVI, // 1 
 VIDEOFILES_MPEG4, // 2 
 VIDEOFILES_MPEG, // 3 
 VIDEOFILES_MP4, // 4 
 VIDEOFILES_WEBM, // 5 
 VIDEOFILES_OGV, // 6 
 VIDEOFILES_MKV, // 7 
 VIDEOFILES_3GP, // 8 
 VIDEOFILES_H263, // 9 
 VIDEOFILES_H264, // 10 
 VIDEOFILES_FLV, // 11 
 VIDEOFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the videoFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_videoFiles(const char * str,unsigned int strLength); 

#endif
