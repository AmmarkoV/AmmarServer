
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "firstLines.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_firstLines(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x636f : /* 'C' */ /* 'O' */ 
     if ( (strLength>=7) && sr_match4(str+0,"\040\040\040\040","conn") && sr_match4(str+3,"\040\040\040\040","nect") ) { return FIRSTLINES_CONNECT; } /* CONNECT */ 
 break; 
 case 0x6465 : /* 'D' */ /* 'E' */ 
     if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\040","dele") && sr_match4(str+2,"\040\040\040\040","lete") ) { return FIRSTLINES_DELETE; } /* DELETE */ 
 break; 
 case 0x6765 : /* 'G' */ /* 'E' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","ge") && sr_match2(str+1,"\040\040","et") ) { return FIRSTLINES_GET; } /* GET */ 
 break; 
 case 0x6865 : /* 'H' */ /* 'E' */ 
     if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","head") ) { return FIRSTLINES_HEAD; } /* HEAD */ 
 break; 
 case 0x6f70 : /* 'O' */ /* 'P' */ 
     if ( (strLength>=7) && sr_match4(str+0,"\040\040\040\040","opti") && sr_match4(str+3,"\040\040\040\040","ions") ) { return FIRSTLINES_OPTIONS; } /* OPTIONS */ 
 break; 
 case 0x7061 : /* 'P' */ /* 'A' */ 
         if ( (strLength>=5) && sr_match4(str+0,"\040\040\040\040","patc") && sr_match4(str+1,"\040\040\040\040","atch") ) { return FIRSTLINES_PATCH; } /* PATCH */ 
 break; 
 case 0x706f : /* 'P' */ /* 'O' */ 
         if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","post") ) { return FIRSTLINES_POST; } /* POST */ 
 break; 
 case 0x7075 : /* 'P' */ /* 'U' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","pu") && sr_match2(str+1,"\040\040","ut") ) { return FIRSTLINES_PUT; } /* PUT */ 
 break; 
 case 0x7472 : /* 'T' */ /* 'R' */ 
     if ( (strLength>=5) && sr_match4(str+0,"\040\040\040\040","trac") && sr_match4(str+1,"\040\040\040\040","race") ) { return FIRSTLINES_TRACE; } /* TRACE */ 
 break; 
}; 
 return 0;
}
