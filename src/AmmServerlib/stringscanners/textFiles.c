
/*                  
This file was automatically generated @ 11-02-2018 19:58:27 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "textFiles.h"

int scanFor_textFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch (toupper(str[0])) { 
 case 'C' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"CSS",3) == 0 ) { return TEXTFILES_CSS; } 
 break; 
 case 'D' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"DOC",3) == 0 ) { return TEXTFILES_DOC; } 
 break; 
 case 'H' : 
     switch (toupper(str[1])) { 
     case 'T' : 
         switch (toupper(str[2])) { 
         case 'M' : 
             if ( (strLength >= 4 )&& ( strncasecmp(str,"HTML",4) == 0 ) ) { return TEXTFILES_HTML; } 
             else  if ( (strLength >= 3 )&& ( strncasecmp(str,"HTM",3) == 0 ) ) { return TEXTFILES_HTM; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'O' : 
     switch (toupper(str[1])) { 
     case 'D' : 
         switch (toupper(str[2])) { 
         case 'F' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"ODF",3) == 0 ) ) { return TEXTFILES_ODF; } 
         break; 
         case 'T' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"ODT",3) == 0 ) ) { return TEXTFILES_ODT; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'R' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"RTF",3) == 0 ) { return TEXTFILES_RTF; } 
 break; 
 case 'T' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"TXT",3) == 0 ) { return TEXTFILES_TXT; } 
 break; 
}; 
 return 0;
}
