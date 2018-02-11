
/** @file audioFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef AUDIOFILES_H_INCLUDED
#define AUDIOFILES_H_INCLUDED


/** @brief Enumerator for the IDs of audioFiles so we can know what the result was*/
enum { 
 AUDIOFILES_EMPTY=0,
 AUDIOFILES_MP3, // 1 
 AUDIOFILES_WAV, // 2 
 AUDIOFILES_MID, // 3 
 AUDIOFILES_OGG, // 4 
 AUDIOFILES_VOC, // 5 
 AUDIOFILES_AU, // 6 
 AUDIOFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the audioFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_audioFiles(const char * str,unsigned int strLength); 

#endif
