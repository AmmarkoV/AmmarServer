#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "postHeader.h"

int scanFor_postHeader(char * str) 
{
 switch (toupper(str[0])) { 
 case 'C' : 
     switch (toupper(str[1])) { 
     case 'O' : 
         switch (toupper(str[2])) { 
         case 'N' : 
             if ( strncasecmp(str,"CONTENT-TYPE:",13) == 0 ) { return POSTHEADER_CONTENT_TYPE; } 
             else  if ( strncasecmp(str,"CONTENT-DISPOSITION:",20) == 0 ) { return POSTHEADER_CONTENT_DISPOSITION; } 
             else  if ( strncasecmp(str,"CONTENT-LENGTH:",15) == 0 ) { return POSTHEADER_CONTENT_LENGTH; } 
         break; 
        }; 
     break; 
    }; 
 break; 
}; 
 return 0;
}
