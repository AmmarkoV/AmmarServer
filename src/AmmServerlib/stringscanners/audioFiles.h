/** @file audioFiles.h
* @brief A tool that scans for a string in a very fast and robust way
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef AUDIOFILES_H_INCLUDED
#define AUDIOFILES_H_INCLUDED


/** @brief Enumerator for the IDs of audioFiles so we can know what the result was*/
enum { 
 AUDIOFILES_EMPTY=0,
 AUDIOFILES_MP3,
 AUDIOFILES_WAV,
 AUDIOFILES_MID,
 AUDIOFILES_OGG,
 AUDIOFILES_VOC,
 AUDIOFILES_AU,
 AUDIOFILES_END_OF_ITEMS
};



/** @brief Scan a string for one of the words of the audioFiles word set
* @ingroup stringParsing
* @param Input String , to be scanned
* @param Length of Input String
* @retval See above enumerator*/
 int scanFor_audioFiles(char * str,unsigned int strLength); 

#endif
