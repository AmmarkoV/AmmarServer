/*                  
This file was automatically generated @ 07-08-2014 15:29:16 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "videoFiles.h"

int scanFor_videoFiles(char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch (toupper(str[0])) { 
 case 'A' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"AVI",3) == 0 ) { return VIDEOFILES_AVI; } 
 break; 
 case 'F' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"FLV",3) == 0 ) { return VIDEOFILES_FLV; } 
 break; 
 case 'H' : 
     switch (toupper(str[1])) { 
     case '2' : 
         switch (toupper(str[2])) { 
         case '6' : 
             if ( (strLength >= 4 )&& ( strncasecmp(str,"H263",4) == 0 ) ) { return VIDEOFILES_H263; } 
             else  if ( (strLength >= 4 )&& ( strncasecmp(str,"H264",4) == 0 ) ) { return VIDEOFILES_H264; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'M' : 
     switch (toupper(str[1])) { 
     case 'K' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"MKV",3) == 0 ) { return VIDEOFILES_MKV; } 
     break; 
     case 'P' : 
         switch (toupper(str[2])) { 
         case 'E' : 
             if ( (strLength >= 5 )&& ( strncasecmp(str,"MPEG4",5) == 0 ) ) { return VIDEOFILES_MPEG4; } 
             else  if ( (strLength >= 4 )&& ( strncasecmp(str,"MPEG",4) == 0 ) ) { return VIDEOFILES_MPEG; } 
         break; 
         case '4' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"MP4",3) == 0 ) ) { return VIDEOFILES_MP4; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'W' : 
     if (strLength<4) { return 0; } 
     if ( strncasecmp(str,"WEBM",4) == 0 ) { return VIDEOFILES_WEBM; } 
 break; 
 case '3' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"3GP",3) == 0 ) { return VIDEOFILES_3GP; } 
 break; 
}; 
 return 0;
}
