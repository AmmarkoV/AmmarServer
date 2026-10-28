
/*                  
This file was automatically generated @ 28-10-2026 22:44:26 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "applicationFiles.h"

int scanFor_applicationFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<2) { return 0; } 

 switch (toupper(str[0])) { 
 case 'C' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"CPL",3) == 0 ) { return APPLICATIONFILES_CPL; } 
 break; 
 case 'D' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"DLL",3) == 0 ) { return APPLICATIONFILES_DLL; } 
 break; 
 case 'E' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"EXE",3) == 0 ) { return APPLICATIONFILES_EXE; } 
 break; 
 case 'J' : 
     switch (toupper(str[1])) { 
     case 'S' : 
         switch (toupper(str[2])) { 
         case 'O' : 
             if ( (strLength >= 4 )&& ( strncasecmp(str,"JSON",4) == 0 ) ) { return APPLICATIONFILES_JSON; } 
         break; 
         case 0 : /*Null Terminator*/ 
             if ( (strLength >= 2 )&& ( strncasecmp(str,"JS",2) == 0 ) ) { return APPLICATIONFILES_JS; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'P' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"PDF",3) == 0 ) { return APPLICATIONFILES_PDF; } 
 break; 
 case 'S' : 
     switch (toupper(str[1])) { 
     case 'C' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"SCR",3) == 0 ) { return APPLICATIONFILES_SCR; } 
     break; 
     case 'W' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"SWF",3) == 0 ) { return APPLICATIONFILES_SWF; } 
     break; 
    }; 
 break; 
}; 
 return 0;
}
