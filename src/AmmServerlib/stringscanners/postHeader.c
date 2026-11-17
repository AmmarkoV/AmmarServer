
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "postHeader.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_postHeader(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<11) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x636f : /* 'C' */ /* 'O' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x6e : /* 'N' */ 
             if ( (strLength>=13) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","content-") && sr_match8(str+5,"\040\040\000\040\040\040\040\000","nt-type:") ) { return POSTHEADER_CONTENT_TYPE; } /* CONTENT-TYPE: */ 
             else  if ( (strLength>=20) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","content-") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","disposit") && sr_match8(str+12,"\040\040\040\040\040\040\040\000","osition:") ) { return POSTHEADER_CONTENT_DISPOSITION; } /* CONTENT-DISPOSITION: */ 
             else  if ( (strLength>=15) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","content-") && sr_match8(str+7,"\000\040\040\040\040\040\040\000","-length:") ) { return POSTHEADER_CONTENT_LENGTH; } /* CONTENT-LENGTH: */ 
             else  if ( (strLength>=11) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","connecti") && sr_match8(str+3,"\040\040\040\040\040\040\040\000","nection:") ) { return POSTHEADER_CONNECTION; } /* CONNECTION: */ 
         break; 
        }; 
 break; 
 case 0x7570 : /* 'U' */ /* 'P' */ 
     if ( (strLength>=26) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","upgrade-") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","insecure") && sr_match8(str+16,"\000\040\040\040\040\040\040\040","-request") && sr_match8(str+18,"\040\040\040\040\040\040\040\000","equests:") ) { return POSTHEADER_UPGRADE_INSECURE_REQUESTS; } /* UPGRADE-INSECURE-REQUESTS: */ 
 break; 
}; 
 return 0;
}
