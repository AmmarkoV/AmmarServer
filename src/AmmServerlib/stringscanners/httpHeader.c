#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "httpHeader.h"

int scanFor_httpHeader(char * str) 
{
 switch (toupper(str[0])) { 
 case 'A' : 
     switch (toupper(str[1])) { 
     case 'C' : 
         if ( strncasecmp(str,"ACCEPT-ENCODING:",16) == 0 ) { return HTTPHEADER_ACCEPT_ENCODING; } 
     break; 
     case 'U' : 
         if ( strncasecmp(str,"AUTHORIZATION:",14) == 0 ) { return HTTPHEADER_AUTHORIZATION; } 
     break; 
    }; 
 break; 
 case 'C' : 
     switch (toupper(str[1])) { 
     case 'O' : 
         switch (toupper(str[2])) { 
         case 'N' : 
             if ( strncasecmp(str,"CONNECTION:",11) == 0 ) { return HTTPHEADER_CONNECTION; } 
         break; 
         case 'O' : 
             if ( strncasecmp(str,"COOKIE:",7) == 0 ) { return HTTPHEADER_COOKIE; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'H' : 
     if ( strncasecmp(str,"HOST:",5) == 0 ) { return HTTPHEADER_HOST; } 
 break; 
 case 'I' : 
     switch (toupper(str[1])) { 
     case 'F' : 
 //Error ( AUTHORIZATION: ) 
     break; 
    }; 
 break; 
 case 'R' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         if ( strncasecmp(str,"RANGE:",6) == 0 ) { return HTTPHEADER_RANGE; } 
     break; 
     case 'E' : 
         switch (toupper(str[2])) { 
         case 'F' : 
             if ( strncasecmp(str,"REFERRER:",9) == 0 ) { return HTTPHEADER_REFERRER; } 
             else  if ( strncasecmp(str,"REFERER:",8) == 0 ) { return HTTPHEADER_REFERER; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'U' : 
     if ( strncasecmp(str,"USER-AGENT:",11) == 0 ) { return HTTPHEADER_USER_AGENT; } 
 break; 
}; 
 return 0;
}
