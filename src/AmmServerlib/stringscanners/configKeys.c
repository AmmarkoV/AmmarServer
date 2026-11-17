
/*                  
This file was automatically generated @ 17-11-2026 20:58:06 using StringRecognizer                  
https://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer                 
Please note that changes you make here may be automatically overwritten                  
if the String Recognizer generator runs again..!              
 */ 

#include <stdint.h>
#include <string.h>
#include "configKeys.h"

/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an
   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */
static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }
static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }
static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }
static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }

int scanFor_configKeys(const char * str,unsigned int strLength) 
{
 if (str==0) { return 0; } 
 if (strLength<7) { return 0; } 

 switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { 
 case 0x6361 : /* 'C' */ /* 'A' */ 
     if ( (strLength>=8) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","caching:") ) { return CONFIGKEYS_CACHING; } /* CACHING: */ 
 break; 
 case 0x6465 : /* 'D' */ /* 'E' */ 
         if ( (strLength>=12) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","defaultt") && sr_match8(str+4,"\040\040\040\040\040\040\040\000","ulttype:") ) { return CONFIGKEYS_DEFAULTTYPE; } /* DEFAULTTYPE: */ 
 break; 
 case 0x6469 : /* 'D' */ /* 'I' */ 
         if ( (strLength>=17) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","director") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","ylisting") && sr_match8(str+9,"\040\040\040\040\040\040\040\000","listing:") ) { return CONFIGKEYS_DIRECTORYLISTING; } /* DIRECTORYLISTING: */ 
 break; 
 case 0x646f : /* 'D' */ /* 'O' */ 
         if ( (strLength>=13) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","document") && sr_match8(str+5,"\040\040\040\040\040\040\040\000","entroot:") ) { return CONFIGKEYS_DOCUMENTROOT; } /* DOCUMENTROOT: */ 
 break; 
 case 0x6479 : /* 'D' */ /* 'Y' */ 
         if ( (strLength>=26) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","dynamicc") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","ontentco") && sr_match8(str+16,"\040\040\040\040\040\040\040\040","mpressio") && sr_match8(str+18,"\040\040\040\040\040\040\040\000","ression:") ) { return CONFIGKEYS_DYNAMICCONTENTCOMPRESSION; } /* DYNAMICCONTENTCOMPRESSION: */ 
 break; 
 case 0x656e : /* 'E' */ /* 'N' */ 
     if ( (strLength>=11) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","enablepo") && sr_match8(str+3,"\040\040\040\040\040\040\040\000","blepost:") ) { return CONFIGKEYS_ENABLEPOST; } /* ENABLEPOST: */ 
 break; 
 case 0x6669 : /* 'F' */ /* 'I' */ 
     if ( (strLength>=16) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","filecomp") && sr_match8(str+8,"\040\040\040\040\040\040\040\000","ression:") ) { return CONFIGKEYS_FILECOMPRESSION; } /* FILECOMPRESSION: */ 
 break; 
 case 0x6966 : /* 'I' */ /* 'F' */ 
     if ( (strLength>=26) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","ifuserdo") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","esntexis") && sr_match8(str+16,"\040\040\040\040\040\040\040\040","trunasui") && sr_match8(str+18,"\040\040\040\040\040\040\040\000","unasuid:") ) { return CONFIGKEYS_IFUSERDOESNTEXISTRUNASUID; } /* IFUSERDOESNTEXISTRUNASUID: */ 
 break; 
 case 0x6b65 : /* 'K' */ /* 'E' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x65 : /* 'E' */ 
             if ( (strLength>=10) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","keepaliv") && sr_match8(str+2,"\040\040\040\040\040\040\040\000","epalive:") ) { return CONFIGKEYS_KEEPALIVE; } /* KEEPALIVE: */ 
             else  if ( (strLength>=17) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","keepaliv") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","etimeout") && sr_match8(str+9,"\040\040\040\040\040\040\040\000","timeout:") ) { return CONFIGKEYS_KEEPALIVETIMEOUT; } /* KEEPALIVETIMEOUT: */ 
         break; 
        }; 
 break; 
 case 0x6c69 : /* 'L' */ /* 'I' */ 
     if ( (strLength>=7) && sr_match4(str+0,"\040\040\040\040","list") && sr_match4(str+3,"\040\040\040\000","ten:") ) { return CONFIGKEYS_LISTEN; } /* LISTEN: */ 
 break; 
 case 0x6d61 : /* 'M' */ /* 'A' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x78 : /* 'X' */ 
             if ( (strLength>=13) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxcache") && sr_match8(str+5,"\040\040\040\040\040\040\040\000","chesize:") ) { return CONFIGKEYS_MAXCACHESIZE; } /* MAXCACHESIZE: */ 
             else  if ( (strLength>=24) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxcache") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","sizefore") && sr_match8(str+16,"\040\040\040\040\040\040\040\000","achfile:") ) { return CONFIGKEYS_MAXCACHESIZEFOREACHFILE; } /* MAXCACHESIZEFOREACHFILE: */ 
             else  if ( (strLength>=22) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxseper") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","atecache") && sr_match8(str+14,"\040\040\040\040\040\040\040\000","heitems:") ) { return CONFIGKEYS_MAXSEPERATECACHEITEMS; } /* MAXSEPERATECACHEITEMS: */ 
             else  if ( (strLength>=23) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxpostt") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","ransacti") && sr_match8(str+15,"\040\040\040\040\040\040\040\000","ionsize:") ) { return CONFIGKEYS_MAXPOSTTRANSACTIONSIZE; } /* MAXPOSTTRANSACTIONSIZE: */ 
             else  if ( (strLength>=21) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxkeepa") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","liverequ") && sr_match8(str+13,"\040\040\040\040\040\040\040\000","equests:") ) { return CONFIGKEYS_MAXKEEPALIVEREQUESTS; } /* MAXKEEPALIVEREQUESTS: */ 
             else  if ( (strLength>=16) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxspare") && sr_match8(str+8,"\040\040\040\040\040\040\040\000","threads:") ) { return CONFIGKEYS_MAXSPARETHREADS; } /* MAXSPARETHREADS: */ 
             else  if ( (strLength>=11) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","maxclien") && sr_match8(str+3,"\040\040\040\040\040\040\040\000","clients:") ) { return CONFIGKEYS_MAXCLIENTS; } /* MAXCLIENTS: */ 
         break; 
        }; 
 break; 
 case 0x6d69 : /* 'M' */ /* 'I' */ 
         if ( (strLength>=16) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","minspare") && sr_match8(str+8,"\040\040\040\040\040\040\040\000","threads:") ) { return CONFIGKEYS_MINSPARETHREADS; } /* MINSPARETHREADS: */ 
 break; 
 case 0x7265 : /* 'R' */ /* 'E' */ 
         if ( (strLength>=54) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","realloct") && sr_match8(str+8,"\040\040\040\040\040\040\040\040","osavemor") && sr_match8(str+16,"\040\040\040\040\040\040\040\040","ethanthi") && sr_match8(str+24,"\040\040\040\040\040\040\040\040","snumbero") && sr_match8(str+32,"\040\040\040\040\040\040\040\040","fbyteswi") && sr_match8(str+40,"\040\040\040\040\040\040\040\040","thcompre") && sr_match8(str+46,"\040\040\040\040\040\040\040\000","ression:") ) { return CONFIGKEYS_REALLOCTOSAVEMORETHANTHISNUMBEROFBYTESWITHCOMPRESSION; } /* REALLOCTOSAVEMORETHANTHISNUMBEROFBYTESWITHCOMPRESSION: */ 
 break; 
 case 0x7275 : /* 'R' */ /* 'U' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x6e : /* 'N' */ 
             if ( (strLength>=14) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","runaspri") && sr_match8(str+6,"\040\040\040\040\040\040\040\000","riority:") ) { return CONFIGKEYS_RUNASPRIORITY; } /* RUNASPRIORITY: */ 
             else  if ( (strLength>=10) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","runasuse") && sr_match8(str+2,"\040\040\040\040\040\040\040\000","nasuser:") ) { return CONFIGKEYS_RUNASUSER; } /* RUNASUSER: */ 
         break; 
        }; 
 break; 
 case 0x7365 : /* 'S' */ /* 'E' */ 
         switch ( ((unsigned char) str[2]) | 0x20 ) { 
         case 0x72 : /* 'R' */ 
             if ( (strLength>=12) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","serverad") && sr_match8(str+4,"\040\040\040\040\040\040\040\000","eradmin:") ) { return CONFIGKEYS_SERVERADMIN; } /* SERVERADMIN: */ 
             else  if ( (strLength>=13) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","serverto") && sr_match8(str+5,"\040\040\040\040\040\040\040\000","rtokens:") ) { return CONFIGKEYS_SERVERTOKENS; } /* SERVERTOKENS: */ 
             else  if ( (strLength>=16) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","serversi") && sr_match8(str+8,"\040\040\040\040\040\040\040\000","gnature:") ) { return CONFIGKEYS_SERVERSIGNATURE; } /* SERVERSIGNATURE: */ 
         break; 
        }; 
 break; 
 case 0x7374 : /* 'S' */ /* 'T' */ 
         if ( (strLength>=13) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","startser") && sr_match8(str+5,"\040\040\040\040\040\040\040\000","servers:") ) { return CONFIGKEYS_STARTSERVERS; } /* STARTSERVERS: */ 
 break; 
 case 0x7468 : /* 'T' */ /* 'H' */ 
         if ( (strLength>=12) && sr_match8(str+0,"\040\040\040\040\040\040\040\040","threadli") && sr_match8(str+4,"\040\040\040\040\040\040\040\000","adlimit:") ) { return CONFIGKEYS_THREADLIMIT; } /* THREADLIMIT: */ 
 break; 
 case 0x7469 : /* 'T' */ /* 'I' */ 
         if ( (strLength>=8) && sr_match8(str+0,"\040\040\040\040\040\040\040\000","timeout:") ) { return CONFIGKEYS_TIMEOUT; } /* TIMEOUT: */ 
 break; 
}; 
 return 0;
}
