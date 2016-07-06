/*                  
This file was automatically generated @ 06-07-2016 18:02:43 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "firstLines.h"

int scanFor_firstLines(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch (toupper(str[0])) { 
 case 'C' : 
     if (strLength<7) { return 0; } 
     if ( strncasecmp(str,"CONNECT",7) == 0 ) { return FIRSTLINES_CONNECT; } 
 break; 
 case 'D' : 
     if (strLength<6) { return 0; } 
     if ( strncasecmp(str,"DELETE",6) == 0 ) { return FIRSTLINES_DELETE; } 
 break; 
 case 'G' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"GET",3) == 0 ) { return FIRSTLINES_GET; } 
 break; 
 case 'H' : 
     if (strLength<4) { return 0; } 
     if ( strncasecmp(str,"HEAD",4) == 0 ) { return FIRSTLINES_HEAD; } 
 break; 
 case 'O' : 
     if (strLength<7) { return 0; } 
     if ( strncasecmp(str,"OPTIONS",7) == 0 ) { return FIRSTLINES_OPTIONS; } 
 break; 
 case 'P' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         if (strLength<5) { return 0; } 
         if ( strncasecmp(str,"PATCH",5) == 0 ) { return FIRSTLINES_PATCH; } 
     break; 
     case 'O' : 
         if (strLength<4) { return 0; } 
         if ( strncasecmp(str,"POST",4) == 0 ) { return FIRSTLINES_POST; } 
     break; 
     case 'U' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"PUT",3) == 0 ) { return FIRSTLINES_PUT; } 
     break; 
    }; 
 break; 
 case 'T' : 
     if (strLength<5) { return 0; } 
     if ( strncasecmp(str,"TRACE",5) == 0 ) { return FIRSTLINES_TRACE; } 
 break; 
}; 
 return 0;
}
