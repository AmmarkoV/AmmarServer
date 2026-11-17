
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "httpHeader.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_httpHeader(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<5) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x6163 : /* 'A' */ /* 'C' */ 
         if ( (strLength>=16) && sr_match8(str+0,"\040\040\040\040\040\040\000\040","accept-e") && sr_match8(str+8,"\040\040\040\040\040\040\040\000","ncoding:") ) { return HTTPHEADER_ACCEPT_ENCODING; } /* ACCEPT-ENCODING: */ 
 break; 
 case 0x6175 : /* 'A' */ /* 'U' */ 
         if ( (strLength>=14) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","authoriz") && sr_match8(str+6,"\040\040\040\040\040\040\040\000","ization:") ) { return HTTPHEADER_AUTHORIZATION; } /* AUTHORIZATION: */ 
 break; 
 case 0x636f : /* 'C' */ /* 'O' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x6e : /* 'N' */ 
             if ( (strLength>=11) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","connecti") && sr_match8(str+3,"\040\040\040\040\040\040\040\000","nection:") ) { return HTTPHEADER_CONNECTION; } /* CONNECTION: */ 
         break; 
         case 0x6f : /* 'O' */ 
             if ( (strLength>=7) && sr_match4(str+0,"\040\040\040\040","cook") && sr_match4(str+3,"\040\040\040\000","kie:") ) { return HTTPHEADER_COOKIE; } /* COOKIE: */ 
         break; 
        }; 
 break; 
 case 0x6578 : /* 'E' */ /* 'X' */ 
     if ( (strLength>=7) && sr_match4(str+0,"\040\040\040\040","expe") && sr_match4(str+3,"\040\040\040\000","ect:") ) { return HTTPHEADER_EXPECT; } /* EXPECT: */ 
 break; 
 case 0x686f : /* 'H' */ /* 'O' */ 
     if ( (strLength>=5) && sr_match4(str+0,"\040\040\040\040","host") && sr_match4(str+1,"\040\040\040\000","ost:") ) { return HTTPHEADER_HOST; } /* HOST: */ 
 break; 
 case 0x6966 : /* 'I' */ /* 'F' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x2d : /* '-' */ 
             if ( (strLength>=14) && sr_match8(str+0,"\040\040\000\040\040\040\040\000","if-none-") && sr_match8(str+6,"\040\000\040\040\040\040\040\000","e-match:") ) { return HTTPHEADER_IF_NONE_MATCH; } /* IF-NONE-MATCH: */ 
             else  if ( (strLength>=18) && sr_match8(str+0,"\040\040\000\040\040\040\040\040","if-modif") && sr_match8(str+8,"\040\040\040\000\040\040\040\040","ied-sinc") && sr_match8(str+10,"\040\000\040\040\040\040\040\000","d-since:") ) { return HTTPHEADER_IF_MODIFIED_SINCE; } /* IF-MODIFIED-SINCE: */ 
             else  if ( (strLength>=20) && sr_match8(str+0,"\040\040\000\040\040\040\040\040","if-unmod") && sr_match8(str+8,"\040\040\040\040\040\000\040\040","ified-si") && sr_match8(str+12,"\040\000\040\040\040\040\040\000","d-since:") ) { return HTTPHEADER_IF_UNMODIFIED_SINCE; } /* IF-UNMODIFIED-SINCE: */ 
         break; 
        }; 
 break; 
 case 0x7261 : /* 'R' */ /* 'A' */ 
         if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\040","rang") && sr_match4(str+2,"\040\040\040\000","nge:") ) { return HTTPHEADER_RANGE; } /* RANGE: */ 
 break; 
 case 0x7265 : /* 'R' */ /* 'E' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x66 : /* 'F' */ 
             if ( (strLength>=9) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","referrer") && sr_match8(str+1,"\040\040\040\040\040\040\040\000","eferrer:") ) { return HTTPHEADER_REFERRER; } /* REFERRER: */ 
             else  if ( (strLength>=8) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","referer:") ) { return HTTPHEADER_REFERER; } /* REFERER: */ 
         break; 
        }; 
 break; 
 case 0x7472 : /* 'T' */ /* 'R' */ 
     if ( (strLength>=18) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","transfer") && sr_match8(str+8,"\000\040\040\040\040\040\040\040","-encodin") && sr_match8(str+10,"\040\040\040\040\040\040\040\000","ncoding:") ) { return HTTPHEADER_TRANSFER_ENCODING; } /* TRANSFER-ENCODING: */ 
 break; 
 case 0x7573 : /* 'U' */ /* 'S' */ 
     if ( (strLength>=11) && sr_match8(str+0,"\040\040\040\040\000\040\040\040","user-age") && sr_match8(str+3,"\040\000\040\040\040\040\040\000","r-agent:") ) { return HTTPHEADER_USER_AGENT; } /* USER-AGENT: */ 
 break; 
}; 
 return 0;
}
