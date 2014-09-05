/** @file textFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef TEXTFILES_H_INCLUDED
#define TEXTFILES_H_INCLUDED


/** @brief Enumerator for the IDs of textFiles so we can know what the result was*/
enum { 
 TEXTFILES_EMPTY=0,
 TEXTFILES_HTML,
 TEXTFILES_HTM,
 TEXTFILES_CSS,
 TEXTFILES_TXT,
 TEXTFILES_DOC,
 TEXTFILES_RTF,
 TEXTFILES_ODF,
 TEXTFILES_ODT,
 TEXTFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the textFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_textFiles(const char * str,unsigned int strLength); 

#endif
