
/** @file httpHeader.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef HTTPHEADER_H_INCLUDED
#define HTTPHEADER_H_INCLUDED


/** @brief Enumerator for the IDs of httpHeader so we can know what the result was*/
enum { 
 HTTPHEADER_EMPTY=0,
 HTTPHEADER_AUTHORIZATION, // 1 
 HTTPHEADER_ACCEPT_ENCODING, // 2 
 HTTPHEADER_COOKIE, // 3 
 HTTPHEADER_CONNECTION, // 4 
 HTTPHEADER_HOST, // 5 
 HTTPHEADER_IF_NONE_MATCH, // 6 
 HTTPHEADER_IF_MODIFIED_SINCE, // 7 
 HTTPHEADER_RANGE, // 8 
 HTTPHEADER_REFERRER, // 9 
 HTTPHEADER_REFERER, // 10 
 HTTPHEADER_USER_AGENT, // 11 
 HTTPHEADER_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the httpHeader word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_httpHeader(const char * str,unsigned int strLength); 

#endif
