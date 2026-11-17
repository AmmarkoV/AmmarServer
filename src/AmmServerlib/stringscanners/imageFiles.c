
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "imageFiles.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_imageFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x626d : /* 'B' */ /* 'M' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","bm") && sr_match2(str+1,"\040\040","mp") ) { return IMAGEFILES_BMP; } /* BMP */ 
 break; 
 case 0x6469 : /* 'D' */ /* 'I' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","di") && sr_match2(str+1,"\040\040","ib") ) { return IMAGEFILES_DIB; } /* DIB */ 
 break; 
 case 0x6769 : /* 'G' */ /* 'I' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","gi") && sr_match2(str+1,"\040\040","if") ) { return IMAGEFILES_GIF; } /* GIF */ 
 break; 
 case 0x6963 : /* 'I' */ /* 'C' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","ic") && sr_match2(str+1,"\040\040","co") ) { return IMAGEFILES_ICO; } /* ICO */ 
 break; 
 case 0x6a70 : /* 'J' */ /* 'P' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x65 : /* 'E' */ 
             if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","jpeg") ) { return IMAGEFILES_JPEG; } /* JPEG */ 
         break; 
         case 0x67 : /* 'G' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","jp") && sr_match2(str+1,"\040\040","pg") ) { return IMAGEFILES_JPG; } /* JPG */ 
         break; 
        }; 
 break; 
 case 0x6a32 : /* 'J' */ /* '2' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\000","j2") && sr_match2(str+1,"\000\040","2c") ) { return IMAGEFILES_J2C; } /* J2C */ 
 break; 
 case 0x706e : /* 'P' */ /* 'N' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x67 : /* 'G' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","pn") && sr_match2(str+1,"\040\040","ng") ) { return IMAGEFILES_PNG; } /* PNG */ 
         break; 
         case 0x6d : /* 'M' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","pn") && sr_match2(str+1,"\040\040","nm") ) { return IMAGEFILES_PNM; } /* PNM */ 
         break; 
        }; 
 break; 
 case 0x7070 : /* 'P' */ /* 'P' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","pp") && sr_match2(str+1,"\040\040","pm") ) { return IMAGEFILES_PPM; } /* PPM */ 
 break; 
 case 0x7261 : /* 'R' */ /* 'A' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","ra") && sr_match2(str+1,"\040\040","aw") ) { return IMAGEFILES_RAW; } /* RAW */ 
 break; 
 case 0x726c : /* 'R' */ /* 'L' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","rl") && sr_match2(str+1,"\040\040","le") ) { return IMAGEFILES_RLE; } /* RLE */ 
 break; 
 case 0x7376 : /* 'S' */ /* 'V' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","sv") && sr_match2(str+1,"\040\040","vg") ) { return IMAGEFILES_SVG; } /* SVG */ 
 break; 
 case 0x7469 : /* 'T' */ /* 'I' */ 
     if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","tiff") ) { return IMAGEFILES_TIFF; } /* TIFF */ 
 break; 
 case 0x7765 : /* 'W' */ /* 'E' */ 
     if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","webp") ) { return IMAGEFILES_WEBP; } /* WEBP */ 
 break; 
}; 
 return 0;
}
