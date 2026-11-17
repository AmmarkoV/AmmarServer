
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "archiveFiles.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_archiveFiles(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<2) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x6172 : /* 'A' */ /* 'R' */ 
     if ( (strLength>=2) && sr_match2(str+0,"\040\040","ar") ) { return ARCHIVEFILES_AR; } /* AR */ 
 break; 
 case 0x627a : /* 'B' */ /* 'Z' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","bz") && sr_match2(str+1,"\040\000","z2") ) { return ARCHIVEFILES_BZ2; } /* BZ2 */ 
 break; 
 case 0x6362 : /* 'C' */ /* 'B' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","cb") && sr_match2(str+1,"\040\040","bz") ) { return ARCHIVEFILES_CBZ; } /* CBZ */ 
 break; 
 case 0x6370 : /* 'C' */ /* 'P' */ 
         if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","cpio") ) { return ARCHIVEFILES_CPIO; } /* CPIO */ 
 break; 
 case 0x677a : /* 'G' */ /* 'Z' */ 
     if ( (strLength>=2) && sr_match2(str+0,"\040\040","gz") ) { return ARCHIVEFILES_GZ; } /* GZ */ 
 break; 
 case 0x6973 : /* 'I' */ /* 'S' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","is") && sr_match2(str+1,"\040\040","so") ) { return ARCHIVEFILES_ISO; } /* ISO */ 
 break; 
 case 0x6a61 : /* 'J' */ /* 'A' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","ja") && sr_match2(str+1,"\040\040","ar") ) { return ARCHIVEFILES_JAR; } /* JAR */ 
 break; 
 case 0x6c7a : /* 'L' */ /* 'Z' */ 
     if ( (strLength>=4) && sr_match4(str+0,"\040\040\040\040","lzma") ) { return ARCHIVEFILES_LZMA; } /* LZMA */ 
 break; 
 case 0x7461 : /* 'T' */ /* 'A' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x72 : /* 'R' */ 
             if ( (strLength>=3) && sr_match2(str+0,"\040\040","ta") && sr_match2(str+1,"\040\040","ar") ) { return ARCHIVEFILES_TAR; } /* TAR */ 
             else  if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+2,"\040\000\000\040","r.7z") ) { return ARCHIVEFILES_TAR_7Z; } /* TAR.7Z */ 
             else  if ( (strLength>=5) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+1,"\040\040\000\040","ar.z") ) { return ARCHIVEFILES_TAR_Z; } /* TAR.Z */ 
             else  if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+2,"\040\000\040\040","r.gz") ) { return ARCHIVEFILES_TAR_GZ; } /* TAR.GZ */ 
             else  if ( (strLength>=7) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+3,"\000\040\040\000",".bz2") ) { return ARCHIVEFILES_TAR_BZ2; } /* TAR.BZ2 */ 
             else  if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+2,"\040\000\040\040","r.bz") ) { return ARCHIVEFILES_TAR_BZ; } /* TAR.BZ */ 
             else  if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+2,"\040\000\040\040","r.lz") ) { return ARCHIVEFILES_TAR_LZ; } /* TAR.LZ */ 
             else  if ( (strLength>=8) && sr_match8(str+0,"\040\040\040\000\040\040\040\040","tar.lzma") ) { return ARCHIVEFILES_TAR_LZMA; } /* TAR.LZMA */ 
             else  if ( (strLength>=6) && sr_match4(str+0,"\040\040\040\000","tar.") && sr_match4(str+2,"\040\000\040\040","r.xz") ) { return ARCHIVEFILES_TAR_XZ; } /* TAR.XZ */ 
         break; 
        }; 
 break; 
 case 0x7467 : /* 'T' */ /* 'G' */ 
         if ( (strLength>=3) && sr_match2(str+0,"\040\040","tg") && sr_match2(str+1,"\040\040","gz") ) { return ARCHIVEFILES_TGZ; } /* TGZ */ 
 break; 
 case 0x787a : /* 'X' */ /* 'Z' */ 
     if ( (strLength>=2) && sr_match2(str+0,"\040\040","xz") ) { return ARCHIVEFILES_XZ; } /* XZ */ 
 break; 
 case 0x7a69 : /* 'Z' */ /* 'I' */ 
     if ( (strLength>=3) && sr_match2(str+0,"\040\040","zi") && sr_match2(str+1,"\040\040","ip") ) { return ARCHIVEFILES_ZIP; } /* ZIP */ 
 break; 
 case 0x377a : /* '7' */ /* 'Z' */ 
     if ( (strLength>=2) && sr_match2(str+0,"\000\040","7z") ) { return ARCHIVEFILES_7Z; } /* 7Z */ 
 break; 
}; 
 return 0;
}
