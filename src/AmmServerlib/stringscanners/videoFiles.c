
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "videoFiles.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_videoFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<3) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x6176 : /* 'A' */ /* 'V' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","av") && sr_match2(str+1,"\040\040","vi") ) { return VIDEOFILES_AVI; } /* AVI */ 
 break; 
 case 0x666c : /* 'F' */ /* 'L' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","fl") && sr_match2(str+1,"\040\040","lv") ) { return VIDEOFILES_FLV; } /* FLV */ 
 break; 
 case 0x6832 : /* 'H' */ /* '2' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x36 : /* '6' */ 
             if ( (strLength>=4) && sr_match4(str+0,"\040\000\000\000","h263") ) { return VIDEOFILES_H263; } /* H263 */ 
             else  if ( (strLength>=4) && sr_match4(str+0,"\040\000\000\000","h264") ) { return VIDEOFILES_H264; } /* H264 */ 
         break; 
        }; 
 break; 
 case 0x6d6b : /* 'M' */ /* 'K' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","mk") && sr_match2(str+1,"\040\040","kv") ) { return VIDEOFILES_MKV; } /* MKV */ 
 break; 
 case 0x6d70 : /* 'M' */ /* 'P' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x65 : /* 'E' */ 
             if ( (strLength>=5) && sr_match4(str+0,"\040\040\040\040","mpeg") && sr_match4(str+1,"\040\040\040\000","peg4") ) { return VIDEOFILES_MPEG4; } /* MPEG4 */ 
             else  if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","mpeg") ) { return VIDEOFILES_MPEG; } /* MPEG */ 
         break; 
         case 0x34 : /* '4' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","mp") && sr_match2(str+1,"\040\000","p4") ) { return VIDEOFILES_MP4; } /* MP4 */ 
         break; 
        }; 
 break; 
 case 0x6f67 : /* 'O' */ /* 'G' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","og") && sr_match2(str+1,"\040\040","gv") ) { return VIDEOFILES_OGV; } /* OGV */ 
 break; 
 case 0x7765 : /* 'W' */ /* 'E' */ 
     if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","webm") ) { return VIDEOFILES_WEBM; } /* WEBM */ 
 break; 
 case 0x3367 : /* '3' */ /* 'G' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\000\040","3g") && sr_match2(str+1,"\040\040","gp") ) { return VIDEOFILES_3GP; } /* 3GP */ 
 break; 
}; 
 return 0;
}
