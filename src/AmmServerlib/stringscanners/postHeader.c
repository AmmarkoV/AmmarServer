/*                  
This file was automatically generated @ 06-07-2016 18:02:43 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "postHeader.h"

int scanFor_postHeader(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<13) { return 0; } 

 switch (toupper(str[0])) { 
 case 'C' : 
     switch (toupper(str[1])) { 
     case 'O' : 
         switch (toupper(str[2])) { 
         case 'N' : 
             if ( (strLength >= 13 )&& ( strncasecmp(str,"CONTENT-TYPE:",13) == 0 ) ) { return POSTHEADER_CONTENT_TYPE; } 
             else  if ( (strLength >= 20 )&& ( strncasecmp(str,"CONTENT-DISPOSITION:",20) == 0 ) ) { return POSTHEADER_CONTENT_DISPOSITION; } 
             else  if ( (strLength >= 15 )&& ( strncasecmp(str,"CONTENT-LENGTH:",15) == 0 ) ) { return POSTHEADER_CONTENT_LENGTH; } 
         break; 
        }; 
     break; 
    }; 
 break; 
}; 
 return 0;
}
