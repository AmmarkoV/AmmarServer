/*                  
This file was automatically generated @ 05-09-2014 22:48:14 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "archiveFiles.h"

int scanFor_archiveFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<2) { return 0; } 

 switch (toupper(str[0])) { 
 case 'A' : 
     if (strLength<2) { return 0; } 
     if ( strncasecmp(str,"AR",2) == 0 ) { return ARCHIVEFILES_AR; } 
 break; 
 case 'B' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"BZ2",3) == 0 ) { return ARCHIVEFILES_BZ2; } 
 break; 
 case 'C' : 
     switch (toupper(str[1])) { 
     case 'B' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"CBZ",3) == 0 ) { return ARCHIVEFILES_CBZ; } 
     break; 
     case 'P' : 
         if (strLength<4) { return 0; } 
         if ( strncasecmp(str,"CPIO",4) == 0 ) { return ARCHIVEFILES_CPIO; } 
     break; 
    }; 
 break; 
 case 'G' : 
     if (strLength<2) { return 0; } 
     if ( strncasecmp(str,"GZ",2) == 0 ) { return ARCHIVEFILES_GZ; } 
 break; 
 case 'I' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"ISO",3) == 0 ) { return ARCHIVEFILES_ISO; } 
 break; 
 case 'J' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"JAR",3) == 0 ) { return ARCHIVEFILES_JAR; } 
 break; 
 case 'L' : 
     if (strLength<4) { return 0; } 
     if ( strncasecmp(str,"LZMA",4) == 0 ) { return ARCHIVEFILES_LZMA; } 
 break; 
 case 'T' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         switch (toupper(str[2])) { 
         case 'R' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"TAR",3) == 0 ) ) { return ARCHIVEFILES_TAR; } 
             else  if ( (strLength >= 6 )&& ( strncasecmp(str,"TAR.7Z",6) == 0 ) ) { return ARCHIVEFILES_TAR_7Z; } 
             else  if ( (strLength >= 5 )&& ( strncasecmp(str,"TAR.Z",5) == 0 ) ) { return ARCHIVEFILES_TAR_Z; } 
             else  if ( (strLength >= 6 )&& ( strncasecmp(str,"TAR.GZ",6) == 0 ) ) { return ARCHIVEFILES_TAR_GZ; } 
             else  if ( (strLength >= 7 )&& ( strncasecmp(str,"TAR.BZ2",7) == 0 ) ) { return ARCHIVEFILES_TAR_BZ2; } 
             else  if ( (strLength >= 6 )&& ( strncasecmp(str,"TAR.BZ",6) == 0 ) ) { return ARCHIVEFILES_TAR_BZ; } 
             else  if ( (strLength >= 6 )&& ( strncasecmp(str,"TAR.LZ",6) == 0 ) ) { return ARCHIVEFILES_TAR_LZ; } 
             else  if ( (strLength >= 8 )&& ( strncasecmp(str,"TAR.LZMA",8) == 0 ) ) { return ARCHIVEFILES_TAR_LZMA; } 
             else  if ( (strLength >= 6 )&& ( strncasecmp(str,"TAR.XZ",6) == 0 ) ) { return ARCHIVEFILES_TAR_XZ; } 
         break; 
        }; 
     break; 
     case 'G' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"TGZ",3) == 0 ) { return ARCHIVEFILES_TGZ; } 
     break; 
    }; 
 break; 
 case 'X' : 
     if (strLength<2) { return 0; } 
     if ( strncasecmp(str,"XZ",2) == 0 ) { return ARCHIVEFILES_XZ; } 
 break; 
 case 'Z' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"ZIP",3) == 0 ) { return ARCHIVEFILES_ZIP; } 
 break; 
 case '7' : 
     if (strLength<2) { return 0; } 
     if ( strncasecmp(str,"7Z",2) == 0 ) { return ARCHIVEFILES_7Z; } 
 break; 
}; 
 return 0;
}
