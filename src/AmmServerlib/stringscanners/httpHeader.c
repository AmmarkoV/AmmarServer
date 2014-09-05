/*                  
This file was automatically generated @ 05-09-2014 22:48:13 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "httpHeader.h"

int scanFor_httpHeader(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<5) { return 0; } 

 switch (toupper(str[0])) { 
 case 'A' : 
     switch (toupper(str[1])) { 
     case 'C' : 
         if (strLength<16) { return 0; } 
         if ( strncasecmp(str,"ACCEPT-ENCODING:",16) == 0 ) { return HTTPHEADER_ACCEPT_ENCODING; } 
     break; 
     case 'U' : 
         if (strLength<14) { return 0; } 
         if ( strncasecmp(str,"AUTHORIZATION:",14) == 0 ) { return HTTPHEADER_AUTHORIZATION; } 
     break; 
    }; 
 break; 
 case 'C' : 
     switch (toupper(str[1])) { 
     case 'O' : 
         switch (toupper(str[2])) { 
         case 'N' : 
             if ( (strLength >= 11 )&& ( strncasecmp(str,"CONNECTION:",11) == 0 ) ) { return HTTPHEADER_CONNECTION; } 
         break; 
         case 'O' : 
             if ( (strLength >= 7 )&& ( strncasecmp(str,"COOKIE:",7) == 0 ) ) { return HTTPHEADER_COOKIE; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'H' : 
     if (strLength<5) { return 0; } 
     if ( strncasecmp(str,"HOST:",5) == 0 ) { return HTTPHEADER_HOST; } 
 break; 
 case 'I' : 
     switch (toupper(str[1])) { 
     case 'F' : 
         switch (toupper(str[2])) { 
         case '-' : 
             if ( (strLength >= 14 )&& ( strncasecmp(str,"IF-NONE-MATCH:",14) == 0 ) ) { return HTTPHEADER_IF_NONE_MATCH; } 
             else  if ( (strLength >= 18 )&& ( strncasecmp(str,"IF-MODIFIED-SINCE:",18) == 0 ) ) { return HTTPHEADER_IF_MODIFIED_SINCE; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'R' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         if (strLength<6) { return 0; } 
         if ( strncasecmp(str,"RANGE:",6) == 0 ) { return HTTPHEADER_RANGE; } 
     break; 
     case 'E' : 
         switch (toupper(str[2])) { 
         case 'F' : 
             if ( (strLength >= 9 )&& ( strncasecmp(str,"REFERRER:",9) == 0 ) ) { return HTTPHEADER_REFERRER; } 
             else  if ( (strLength >= 8 )&& ( strncasecmp(str,"REFERER:",8) == 0 ) ) { return HTTPHEADER_REFERER; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'U' : 
     if (strLength<11) { return 0; } 
     if ( strncasecmp(str,"USER-AGENT:",11) == 0 ) { return HTTPHEADER_USER_AGENT; } 
 break; 
}; 
 return 0;
}
