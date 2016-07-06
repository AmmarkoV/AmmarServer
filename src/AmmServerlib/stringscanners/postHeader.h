 /** @file postHeader.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef POSTHEADER_H_INCLUDED
#define POSTHEADER_H_INCLUDED


/** @brief Enumerator for the IDs of postHeader so we can know what the result was*/
enum { 
 POSTHEADER_EMPTY=0,
 POSTHEADER_CONTENT_TYPE, // 1 
 POSTHEADER_CONTENT_DISPOSITION, // 2 
 POSTHEADER_CONTENT_LENGTH, // 3 
 POSTHEADER_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the postHeader word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_postHeader(const char * str,unsigned int strLength); 

#endif
