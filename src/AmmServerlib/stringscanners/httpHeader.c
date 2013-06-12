#include <stdio.h>
#include "httpHeader.h"

int scanFor_httpHeader(char * str) 
{
 switch (str[0]) { 
 case 'A' : 
     switch (str[1]) { 
     case 'C' : 
         if ( strcmp(str,"ACCEPT-ENCODING:") == 0 ) { return HTTPHEADER_ACCEPT_ENCODING; } 
     break; 
     case 'U' : 
         if ( strcmp(str,"AUTHORIZATION:") == 0 ) { return HTTPHEADER_AUTHORIZATION; } 
     break; 
    }; 
 break; 
 case 'C' : 
     switch (str[1]) { 
     case 'O' : 
         switch (str[2]) { 
         case 'N' : 
             if ( strcmp(str,"CONNECTION:") == 0 ) { return HTTPHEADER_CONNECTION; } 
         break; 
         case 'O' : 
             if ( strcmp(str,"COOKIE:") == 0 ) { return HTTPHEADER_COOKIE; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'H' : 
     if ( strcmp(str,"HOST:") == 0 ) { return HTTPHEADER_HOST; } 
 break; 
 case 'I' : 
     switch (str[1]) { 
     case 'F' : 
 //Error ( AUTHORIZATION: ) 
     break; 
    }; 
 break; 
 case 'R' : 
     switch (str[1]) { 
     case 'A' : 
         if ( strcmp(str,"RANGE:") == 0 ) { return HTTPHEADER_RANGE; } 
     break; 
     case 'E' : 
         switch (str[2]) { 
         case 'F' : 
             if ( strcmp(str,"REFERRER:") == 0 ) { return HTTPHEADER_REFERRER; } 
             else  if ( strcmp(str,"REFERER:") == 0 ) { return HTTPHEADER_REFERER; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'U' : 
     if ( strcmp(str,"USER-AGENT:") == 0 ) { return HTTPHEADER_USER_AGENT; } 
 break; 
}; 
 return 0;
}


int main(int argc, char *argv[]) 
 {
  if (argc<1) { fprintf(stderr,"No parameter\n"); return 1; }
  if ( scanFor_httpHeader(argv[0]) ) { fprintf(stderr,"Found it"); } 
  return 0; 
 }
