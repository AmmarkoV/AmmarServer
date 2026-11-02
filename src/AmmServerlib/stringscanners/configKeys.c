
/*                  
This file was automatically generated @ 02-11-2026 23:27:52 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "configKeys.h"

int scanFor_configKeys(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<7) { return 0; } 

 switch (toupper(str[0])) { 
 case 'C' : 
     if (strLength<8) { return 0; } 
     if ( strncasecmp(str,"CACHING:",8) == 0 ) { return CONFIGKEYS_CACHING; } 
 break; 
 case 'D' : 
     switch (toupper(str[1])) { 
     case 'E' : 
         if (strLength<12) { return 0; } 
         if ( strncasecmp(str,"DEFAULTTYPE:",12) == 0 ) { return CONFIGKEYS_DEFAULTTYPE; } 
     break; 
     case 'I' : 
         if (strLength<17) { return 0; } 
         if ( strncasecmp(str,"DIRECTORYLISTING:",17) == 0 ) { return CONFIGKEYS_DIRECTORYLISTING; } 
     break; 
     case 'O' : 
         if (strLength<13) { return 0; } 
         if ( strncasecmp(str,"DOCUMENTROOT:",13) == 0 ) { return CONFIGKEYS_DOCUMENTROOT; } 
     break; 
     case 'Y' : 
         if (strLength<26) { return 0; } 
         if ( strncasecmp(str,"DYNAMICCONTENTCOMPRESSION:",26) == 0 ) { return CONFIGKEYS_DYNAMICCONTENTCOMPRESSION; } 
     break; 
    }; 
 break; 
 case 'E' : 
     if (strLength<11) { return 0; } 
     if ( strncasecmp(str,"ENABLEPOST:",11) == 0 ) { return CONFIGKEYS_ENABLEPOST; } 
 break; 
 case 'F' : 
     if (strLength<16) { return 0; } 
     if ( strncasecmp(str,"FILECOMPRESSION:",16) == 0 ) { return CONFIGKEYS_FILECOMPRESSION; } 
 break; 
 case 'I' : 
     if (strLength<26) { return 0; } 
     if ( strncasecmp(str,"IFUSERDOESNTEXISTRUNASUID:",26) == 0 ) { return CONFIGKEYS_IFUSERDOESNTEXISTRUNASUID; } 
 break; 
 case 'K' : 
     switch (toupper(str[1])) { 
     case 'E' : 
         switch (toupper(str[2])) { 
         case 'E' : 
             if ( (strLength >= 10 )&& ( strncasecmp(str,"KEEPALIVE:",10) == 0 ) ) { return CONFIGKEYS_KEEPALIVE; } 
             else  if ( (strLength >= 17 )&& ( strncasecmp(str,"KEEPALIVETIMEOUT:",17) == 0 ) ) { return CONFIGKEYS_KEEPALIVETIMEOUT; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'L' : 
     if (strLength<7) { return 0; } 
     if ( strncasecmp(str,"LISTEN:",7) == 0 ) { return CONFIGKEYS_LISTEN; } 
 break; 
 case 'M' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         switch (toupper(str[2])) { 
         case 'X' : 
             if ( (strLength >= 13 )&& ( strncasecmp(str,"MAXCACHESIZE:",13) == 0 ) ) { return CONFIGKEYS_MAXCACHESIZE; } 
             else  if ( (strLength >= 24 )&& ( strncasecmp(str,"MAXCACHESIZEFOREACHFILE:",24) == 0 ) ) { return CONFIGKEYS_MAXCACHESIZEFOREACHFILE; } 
             else  if ( (strLength >= 22 )&& ( strncasecmp(str,"MAXSEPERATECACHEITEMS:",22) == 0 ) ) { return CONFIGKEYS_MAXSEPERATECACHEITEMS; } 
             else  if ( (strLength >= 23 )&& ( strncasecmp(str,"MAXPOSTTRANSACTIONSIZE:",23) == 0 ) ) { return CONFIGKEYS_MAXPOSTTRANSACTIONSIZE; } 
             else  if ( (strLength >= 21 )&& ( strncasecmp(str,"MAXKEEPALIVEREQUESTS:",21) == 0 ) ) { return CONFIGKEYS_MAXKEEPALIVEREQUESTS; } 
             else  if ( (strLength >= 16 )&& ( strncasecmp(str,"MAXSPARETHREADS:",16) == 0 ) ) { return CONFIGKEYS_MAXSPARETHREADS; } 
             else  if ( (strLength >= 11 )&& ( strncasecmp(str,"MAXCLIENTS:",11) == 0 ) ) { return CONFIGKEYS_MAXCLIENTS; } 
         break; 
        }; 
     break; 
     case 'I' : 
         if (strLength<16) { return 0; } 
         if ( strncasecmp(str,"MINSPARETHREADS:",16) == 0 ) { return CONFIGKEYS_MINSPARETHREADS; } 
     break; 
    }; 
 break; 
 case 'R' : 
     switch (toupper(str[1])) { 
     case 'E' : 
         if (strLength<54) { return 0; } 
         if ( strncasecmp(str,"REALLOCTOSAVEMORETHANTHISNUMBEROFBYTESWITHCOMPRESSION:",54) == 0 ) { return CONFIGKEYS_REALLOCTOSAVEMORETHANTHISNUMBEROFBYTESWITHCOMPRESSION; } 
     break; 
     case 'U' : 
         switch (toupper(str[2])) { 
         case 'N' : 
             if ( (strLength >= 14 )&& ( strncasecmp(str,"RUNASPRIORITY:",14) == 0 ) ) { return CONFIGKEYS_RUNASPRIORITY; } 
             else  if ( (strLength >= 10 )&& ( strncasecmp(str,"RUNASUSER:",10) == 0 ) ) { return CONFIGKEYS_RUNASUSER; } 
         break; 
        }; 
     break; 
    }; 
 break; 
 case 'S' : 
     switch (toupper(str[1])) { 
     case 'E' : 
         switch (toupper(str[2])) { 
         case 'R' : 
             if ( (strLength >= 12 )&& ( strncasecmp(str,"SERVERADMIN:",12) == 0 ) ) { return CONFIGKEYS_SERVERADMIN; } 
             else  if ( (strLength >= 13 )&& ( strncasecmp(str,"SERVERTOKENS:",13) == 0 ) ) { return CONFIGKEYS_SERVERTOKENS; } 
             else  if ( (strLength >= 16 )&& ( strncasecmp(str,"SERVERSIGNATURE:",16) == 0 ) ) { return CONFIGKEYS_SERVERSIGNATURE; } 
         break; 
        }; 
     break; 
     case 'T' : 
         if (strLength<13) { return 0; } 
         if ( strncasecmp(str,"STARTSERVERS:",13) == 0 ) { return CONFIGKEYS_STARTSERVERS; } 
     break; 
    }; 
 break; 
 case 'T' : 
     switch (toupper(str[1])) { 
     case 'H' : 
         if (strLength<12) { return 0; } 
         if ( strncasecmp(str,"THREADLIMIT:",12) == 0 ) { return CONFIGKEYS_THREADLIMIT; } 
     break; 
     case 'I' : 
         if (strLength<8) { return 0; } 
         if ( strncasecmp(str,"TIMEOUT:",8) == 0 ) { return CONFIGKEYS_TIMEOUT; } 
     break; 
    }; 
 break; 
}; 
 return 0;
}
