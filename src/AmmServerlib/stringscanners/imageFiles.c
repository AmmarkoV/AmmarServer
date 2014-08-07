/*                  
This file was automatically generated @ 07-08-2014 15:29:16 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "imageFiles.h"

int scanFor_imageFiles(char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch (toupper(str[0])) { 
 case 'B' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"BMP",3) == 0 ) { return IMAGEFILES_BMP; } 
 break; 
 case 'D' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"DIB",3) == 0 ) { return IMAGEFILES_DIB; } 
 break; 
 case 'G' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"GIF",3) == 0 ) { return IMAGEFILES_GIF; } 
 break; 
 case 'I' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"ICO",3) == 0 ) { return IMAGEFILES_ICO; } 
 break; 
 case 'J' : 
     switch (toupper(str[1])) { 
     case 'P' : 
         switch (toupper(str[2])) { 
         case 'E' : 
             if ( (strLength >= 4 )&& ( strncasecmp(str,"JPEG",4) == 0 ) ) { return IMAGEFILES_JPEG; } 
         break; 
         case 'G' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"JPG",3) == 0 ) ) { return IMAGEFILES_JPG; } 
         break; 
        }; 
     break; 
     case '2' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"J2C",3) == 0 ) { return IMAGEFILES_J2C; } 
     break; 
    }; 
 break; 
 case 'P' : 
     switch (toupper(str[1])) { 
     case 'N' : 
         switch (toupper(str[2])) { 
         case 'G' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"PNG",3) == 0 ) ) { return IMAGEFILES_PNG; } 
         break; 
         case 'M' : 
             if ( (strLength >= 3 )&& ( strncasecmp(str,"PNM",3) == 0 ) ) { return IMAGEFILES_PNM; } 
         break; 
        }; 
     break; 
     case 'P' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"PPM",3) == 0 ) { return IMAGEFILES_PPM; } 
     break; 
    }; 
 break; 
 case 'R' : 
     switch (toupper(str[1])) { 
     case 'A' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"RAW",3) == 0 ) { return IMAGEFILES_RAW; } 
     break; 
     case 'L' : 
         if (strLength<3) { return 0; } 
         if ( strncasecmp(str,"RLE",3) == 0 ) { return IMAGEFILES_RLE; } 
     break; 
    }; 
 break; 
 case 'S' : 
     if (strLength<3) { return 0; } 
     if ( strncasecmp(str,"SVG",3) == 0 ) { return IMAGEFILES_SVG; } 
 break; 
 case 'T' : 
     if (strLength<4) { return 0; } 
     if ( strncasecmp(str,"TIFF",4) == 0 ) { return IMAGEFILES_TIFF; } 
 break; 
 case 'W' : 
     if (strLength<4) { return 0; } 
     if ( strncasecmp(str,"WEBP",4) == 0 ) { return IMAGEFILES_WEBP; } 
 break; 
}; 
 return 0;
}
