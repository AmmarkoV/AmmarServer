
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "textFiles.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_textFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x6373 : /* 'C' */ /* 'S' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","cs") && sr_match2(str+1,"\040\040","ss") ) { return TEXTFILES_CSS; } /* CSS */ 
 break; 
 case 0x646f : /* 'D' */ /* 'O' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","do") && sr_match2(str+1,"\040\040","oc") ) { return TEXTFILES_DOC; } /* DOC */ 
 break; 
 case 0x6874 : /* 'H' */ /* 'T' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x6d : /* 'M' */ 
             if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","html") ) { return TEXTFILES_HTML; } /* HTML */ 
             else  if ( (strLength>=3) && sr_match2(str+0,"\040\040","ht") && sr_match2(str+1,"\040\040","tm") ) { return TEXTFILES_HTM; } /* HTM */ 
         break; 
        }; 
 break; 
 case 0x6f64 : /* 'O' */ /* 'D' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x66 : /* 'F' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","od") && sr_match2(str+1,"\040\040","df") ) { return TEXTFILES_ODF; } /* ODF */ 
         break; 
         case 0x74 : /* 'T' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","od") && sr_match2(str+1,"\040\040","dt") ) { return TEXTFILES_ODT; } /* ODT */ 
         break; 
        }; 
 break; 
 case 0x7274 : /* 'R' */ /* 'T' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","rt") && sr_match2(str+1,"\040\040","tf") ) { return TEXTFILES_RTF; } /* RTF */ 
 break; 
 case 0x7478 : /* 'T' */ /* 'X' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","tx") && sr_match2(str+1,"\040\040","xt") ) { return TEXTFILES_TXT; } /* TXT */ 
 break; 
}; 
 return 0;
}
