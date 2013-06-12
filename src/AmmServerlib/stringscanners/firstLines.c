#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "firstLines.h"

int scanFor_firstLines(char * str) 
{
 switch (toupper(str[0])) { 
 case 'C' : 
     if ( strncasecmp(str,"CONNECT",7) == 0 ) { return FIRSTLINES_CONNECT; } 
 break; 
 case 'D' : 
     if ( strncasecmp(str,"DELETE",6) == 0 ) { return FIRSTLINES_DELETE; } 
 break; 
 case 'G' : 
     if ( strncasecmp(str,"GET",3) == 0 ) { return FIRSTLINES_GET; } 
 break; 
 case 'H' : 
     if ( strncasecmp(str,"HEAD",4) == 0 ) { return FIRSTLINES_HEAD; } 
 break; 
 case 'O' : 
     if ( strncasecmp(str,"OPTIONS",7) == 0 ) { return FIRSTLINES_OPTIONS; } 
 break; 
 case 'P' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         if ( strncasecmp(str,"PATCH",5) == 0 ) { return FIRSTLINES_PATCH; } 
     break; 
     case 'O' : 
         if ( strncasecmp(str,"POST",4) == 0 ) { return FIRSTLINES_POST; } 
     break; 
     case 'U' : 
         if ( strncasecmp(str,"PUT",3) == 0 ) { return FIRSTLINES_PUT; } 
     break; 
    }; 
 break; 
 case 'T' : 
     if ( strncasecmp(str,"TRACE",5) == 0 ) { return FIRSTLINES_TRACE; } 
 break; 
}; 
 return 0;
}
