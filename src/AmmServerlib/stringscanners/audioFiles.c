
/*                  
This file was automatically generated @ 11-02-2018 19:58:27 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "audioFiles.h"

int scanFor_audioFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<2) { return 0; } 

 switch (toupper(str[0])) { 
 case 'A' : 
     if (strLength<2) { return 0; } 
     if ( strncasecmp(str,"AU",2) == 0 ) { return AUDIOFILES_AU; } 
     if (strLength<3) { return 0; }
     if ( strncasecmp(str,"AMR",3) == 0 ) { return AUDIOFILES_AMR; }
 break; 
 case 'M' : 
     switch (toupper(str[1])) { 
     case '4' :
         if (strLength<3) { return 0; }
         if ( strncasecmp(str,"M4A",3) == 0 ) { return AUDIOFILES_M4A; }
     break;
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
     switch (toupper(str[1])) {
     case 'A' :
         if ( strncasecmp(str,"WAV",3) == 0 ) { return AUDIOFILES_WAV; }
     break;
     case 'E' :
         if (strLength<4) { return 0; }
         if ( (strncasecmp(str,"WEBM",4) == 0) || (strncasecmp(str,"WEBA",4) == 0) ) { return AUDIOFILES_WEBM; }
     break;
     };
 break; 
}; 
 return 0;
}
