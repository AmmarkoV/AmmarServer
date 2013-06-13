/*                  
This file was automatically generated using StringRecognizer                 
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "audioFiles.h"

int scanFor_audioFiles(char * str,unsigned int strLength) 
{
 if (strLength<2) { return 0; } 

 switch (toupper(str[0])) { 
 case 'A' : 
     if (strLength<2) { return 0; } 
     if ( strncasecmp(str,"AU",2) == 0 ) { return AUDIOFILES_AU; } 
 break; 
 case 'M' : 
     switch (toupper(str[1])) { 
     case 'I' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"MID",3) == 0 ) { return AUDIOFILES_MID; } 
     break; 
     case 'P' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"MP3",3) == 0 ) { return AUDIOFILES_MP3; } 
     break; 
    }; 
 break; 
 case 'O' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"OGG",3) == 0 ) { return AUDIOFILES_OGG; } 
 break; 
 case 'V' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"VOC",3) == 0 ) { return AUDIOFILES_VOC; } 
 break; 
 case 'W' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"WAV",3) == 0 ) { return AUDIOFILES_WAV; } 
 break; 
}; 
 return 0;
}
