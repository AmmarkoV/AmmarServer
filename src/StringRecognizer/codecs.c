
/*                  
This file was automatically generated @ 23-05-2024 00:04:46 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "codecs.h"

int scanFor_codecs(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch (toupper(str[0])) { 
 case 'A' : 
     if (strLength<5) { return 0; } 
     if ( strncasecmp(str,"ASCII",5) == 0 ) { return CODECS_ASCII; } 
 break; 
 case 'B' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"BMP",3) == 0 ) { return CODECS_BMP; } 
 break; 
 case 'C' : 
     if (strLength<4) { return 0; } 
     if ( strncasecmp(str,"CPNM",4) == 0 ) { return CODECS_CPNM; } 
 break; 
 case 'J' : 
     switch (toupper(str[1])) { 
     case 'P' : 
         switch (toupper(str[2])) { 
         case 'E' : 
             if ( (strLength >= 4 )&& ( strncasecmp(str,"JPEG",4) == 0 ) ) { return CODECS_JPEG; } 
         break; 
         case 'G' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"JPG",3) == 0 ) ) { return CODECS_JPG; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'P' : 
     switch (toupper(str[1])) { 
     case 'N' : 
         switch (toupper(str[2])) { 
         case 'G' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"PNG",3) == 0 ) ) { return CODECS_PNG; } 
         break; 
         case 'M' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"PNM",3) == 0 ) ) { return CODECS_PNM; } 
         break; 
        }; 
     break; 
     case 'P' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"PPM",3) == 0 ) { return CODECS_PPM; } 
     break; 
    }; 
 break; 
 case 'R' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"RLE",3) == 0 ) { return CODECS_RLE; } 
 break; 
 case 'T' : 
     switch (toupper(str[1])) { 
     case 'E' : 
         if (strLength<4) { return 0; } 
         if ( strncasecmp(str,"TEXT",4) == 0 ) { return CODECS_TEXT; } 
     break; 
     case 'X' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"TXT",3) == 0 ) { return CODECS_TXT; } 
     break; 
    }; 
 break; 
}; 
 return 0;
}
