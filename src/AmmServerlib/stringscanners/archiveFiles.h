/** @file archiveFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef ARCHIVEFILES_H_INCLUDED
#define ARCHIVEFILES_H_INCLUDED


/** @brief Enumerator for the IDs of archiveFiles so we can know what the result was*/
enum { 
 ARCHIVEFILES_EMPTY=0,
 ARCHIVEFILES_7Z,
 ARCHIVEFILES_AR,
 ARCHIVEFILES_BZ2,
 ARCHIVEFILES_CBZ,
 ARCHIVEFILES_CPIO,
 ARCHIVEFILES_GZ,
 ARCHIVEFILES_ISO,
 ARCHIVEFILES_JAR,
 ARCHIVEFILES_LZMA,
 ARCHIVEFILES_TAR,
 ARCHIVEFILES_TGZ,
 ARCHIVEFILES_TAR_7Z,
 ARCHIVEFILES_TAR_Z,
 ARCHIVEFILES_TAR_GZ,
 ARCHIVEFILES_TAR_BZ2,
 ARCHIVEFILES_TAR_BZ,
 ARCHIVEFILES_TAR_LZ,
 ARCHIVEFILES_TAR_LZMA,
 ARCHIVEFILES_TAR_XZ,
 ARCHIVEFILES_XZ,
 ARCHIVEFILES_ZIP,
 ARCHIVEFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the archiveFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_archiveFiles(char * str,unsigned int strLength); 

#endif
