/** @file httpHeader.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef HTTPHEADER_H_INCLUDED
#define HTTPHEADER_H_INCLUDED


/** @brief Enumerator for the IDs of httpHeader so we can know what the result was*/
enum { 
 HTTPHEADER_EMPTY=0,
 HTTPHEADER_AUTHORIZATION,
 HTTPHEADER_ACCEPT_ENCODING,
 HTTPHEADER_COOKIE,
 HTTPHEADER_CONNECTION,
 HTTPHEADER_HOST,
 HTTPHEADER_IF_NONE_MATCH,
 HTTPHEADER_IF_MODIFIED_SINCE,
 HTTPHEADER_RANGE,
 HTTPHEADER_REFERRER,
 HTTPHEADER_REFERER,
 HTTPHEADER_USER_AGENT,
 HTTPHEADER_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the httpHeader word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_httpHeader(char * str,unsigned int strLength); 

#endif
