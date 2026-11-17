
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "audioFiles.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_audioFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<2) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x616d : /* 'A' */ /* 'M' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","am") && sr_match2(str+1,"\040\040","mr") ) { return AUDIOFILES_AMR; } /* AMR */ 
 break; 
 case 0x6175 : /* 'A' */ /* 'U' */ 
         if ( (strLength>=2) && sr_match2(str+0,"\040\040","au") ) { return AUDIOFILES_AU; } /* AU */ 
 break; 
 case 0x6d69 : /* 'M' */ /* 'I' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","mi") && sr_match2(str+1,"\040\040","id") ) { return AUDIOFILES_MID; } /* MID */ 
 break; 
 case 0x6d70 : /* 'M' */ /* 'P' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","mp") && sr_match2(str+1,"\040\000","p3") ) { return AUDIOFILES_MP3; } /* MP3 */ 
 break; 
 case 0x6d34 : /* 'M' */ /* '4' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\000","m4") && sr_match2(str+1,"\000\040","4a") ) { return AUDIOFILES_M4A; } /* M4A */ 
 break; 
 case 0x6f67 : /* 'O' */ /* 'G' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","og") && sr_match2(str+1,"\040\040","gg") ) { return AUDIOFILES_OGG; } /* OGG */ 
 break; 
 case 0x766f : /* 'V' */ /* 'O' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","vo") && sr_match2(str+1,"\040\040","oc") ) { return AUDIOFILES_VOC; } /* VOC */ 
 break; 
 case 0x7761 : /* 'W' */ /* 'A' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","wa") && sr_match2(str+1,"\040\040","av") ) { return AUDIOFILES_WAV; } /* WAV */ 
 break; 
 case 0x7765 : /* 'W' */ /* 'E' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x62 : /* 'B' */ 
             if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","webm") ) { return AUDIOFILES_WEBM; } /* WEBM */ 
             else  if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","weba") ) { return AUDIOFILES_WEBA; } /* WEBA */ 
         break; 
        }; 
 break; 
}; 
 return 0;
}
