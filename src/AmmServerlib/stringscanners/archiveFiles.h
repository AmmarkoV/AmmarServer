
/** @file archiveFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef ARCHIVEFILES_H_INCLUDED
#define ARCHIVEFILES_H_INCLUDED


/** @brief Enumerator for the IDs of archiveFiles so we can know what the result was*/
enum { 
 ARCHIVEFILES_EMPTY=0,
 ARCHIVEFILES_7Z, // 1 
 ARCHIVEFILES_AR, // 2 
 ARCHIVEFILES_BZ2, // 3 
 ARCHIVEFILES_CBZ, // 4 
 ARCHIVEFILES_CPIO, // 5 
 ARCHIVEFILES_GZ, // 6 
 ARCHIVEFILES_ISO, // 7 
 ARCHIVEFILES_JAR, // 8 
 ARCHIVEFILES_LZMA, // 9 
 ARCHIVEFILES_TAR, // 10 
 ARCHIVEFILES_TGZ, // 11 
 ARCHIVEFILES_TAR_7Z, // 12 
 ARCHIVEFILES_TAR_Z, // 13 
 ARCHIVEFILES_TAR_GZ, // 14 
 ARCHIVEFILES_TAR_BZ2, // 15 
 ARCHIVEFILES_TAR_BZ, // 16 
 ARCHIVEFILES_TAR_LZ, // 17 
 ARCHIVEFILES_TAR_LZMA, // 18 
 ARCHIVEFILES_TAR_XZ, // 19 
 ARCHIVEFILES_XZ, // 20 
 ARCHIVEFILES_ZIP, // 21 
 ARCHIVEFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the archiveFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_archiveFiles(const char * str,unsigned int strLength); 

#endif
