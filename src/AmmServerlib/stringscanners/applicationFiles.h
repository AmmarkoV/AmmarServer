/** @file applicationFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef APPLICATIONFILES_H_INCLUDED
#define APPLICATIONFILES_H_INCLUDED


/** @brief Enumerator for the IDs of applicationFiles so we can know what the result was*/
enum { 
 APPLICATIONFILES_EMPTY=0,
 APPLICATIONFILES_EXE,
 APPLICATIONFILES_DLL,
 APPLICATIONFILES_SCR,
 APPLICATIONFILES_CPL,
 APPLICATIONFILES_SWF,
 APPLICATIONFILES_PDF,
 APPLICATIONFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the applicationFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_applicationFiles(char * str,unsigned int strLength); 

#endif
