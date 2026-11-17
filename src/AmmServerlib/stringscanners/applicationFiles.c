
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "applicationFiles.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_applicationFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<2) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x6370 : /* 'C' */ /* 'P' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","cp") && sr_match2(str+1,"\040\040","pl") ) { return APPLICATIONFILES_CPL; } /* CPL */ 
 break; 
 case 0x646c : /* 'D' */ /* 'L' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","dl") && sr_match2(str+1,"\040\040","ll") ) { return APPLICATIONFILES_DLL; } /* DLL */ 
 break; 
 case 0x6578 : /* 'E' */ /* 'X' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","ex") && sr_match2(str+1,"\040\040","xe") ) { return APPLICATIONFILES_EXE; } /* EXE */ 
 break; 
 case 0x6a73 : /* 'J' */ /* 'S' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x6f : /* 'O' */ 
             if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","json") ) { return APPLICATIONFILES_JSON; } /* JSON */ 
         break; 
         case 0x20 : /*Null Terminator*/ 
             if ( (strLength>=2) && sr_match2(str+0,"\040\040","js") && (str[2]==0) ) { return APPLICATIONFILES_JS; } /* JS */ 
         break; 
        }; 
 break; 
 case 0x7064 : /* 'P' */ /* 'D' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","pd") && sr_match2(str+1,"\040\040","df") ) { return APPLICATIONFILES_PDF; } /* PDF */ 
 break; 
 case 0x7363 : /* 'S' */ /* 'C' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","sc") && sr_match2(str+1,"\040\040","cr") ) { return APPLICATIONFILES_SCR; } /* SCR */ 
 break; 
 case 0x7377 : /* 'S' */ /* 'W' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","sw") && sr_match2(str+1,"\040\040","wf") ) { return APPLICATIONFILES_SWF; } /* SWF */ 
 break; 
}; 
 return 0;
}
