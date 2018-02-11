
/** @file textFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef TEXTFILES_H_INCLUDED
#define TEXTFILES_H_INCLUDED


/** @brief Enumerator for the IDs of textFiles so we can know what the result was*/
enum { 
 TEXTFILES_EMPTY=0,
 TEXTFILES_HTML, // 1 
 TEXTFILES_HTM, // 2 
 TEXTFILES_CSS, // 3 
 TEXTFILES_TXT, // 4 
 TEXTFILES_DOC, // 5 
 TEXTFILES_RTF, // 6 
 TEXTFILES_ODF, // 7 
 TEXTFILES_ODT, // 8 
 TEXTFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the textFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_textFiles(const char * str,unsigned int strLength); 

#endif
