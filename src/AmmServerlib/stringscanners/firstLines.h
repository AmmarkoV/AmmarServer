 /** @file firstLines.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef FIRSTLINES_H_INCLUDED
#define FIRSTLINES_H_INCLUDED


/** @brief Enumerator for the IDs of firstLines so we can know what the result was*/
enum { 
 FIRSTLINES_EMPTY=0,
 FIRSTLINES_GET, // 1 
 FIRSTLINES_HEAD, // 2 
 FIRSTLINES_POST, // 3 
 FIRSTLINES_PUT, // 4 
 FIRSTLINES_DELETE, // 5 
 FIRSTLINES_TRACE, // 6 
 FIRSTLINES_OPTIONS, // 7 
 FIRSTLINES_CONNECT, // 8 
 FIRSTLINES_PATCH, // 9 
 FIRSTLINES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the firstLines word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_firstLines(const char * str,unsigned int strLength); 

#endif
