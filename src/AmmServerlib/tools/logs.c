#include <stdio.h>
#include <time.h>
#include "logs.h"
#include "../server_configuration.h"

//This is a sample Apache access.log entry
//184.82.219.85 - - [05/Feb/2012:07:28:19 +0200] "GET /~ammar/guard_dog_project/blogs/index.php/2010/02/22/video-room-test?blog=1 HTTP/1.0" 301 528 "-" "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)"
//99.100.183.119 - - [05/Feb/2012:07:40:22 +0200] "GET /~ammar/guard_dog_project/blogs/skins/evopress/img/kubrickbgcolor.jpg HTTP/1.1" 200 2135 "http://ammar.gr/gddg/" "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)"



/*
red=$(printf "\033[31m")
green=$(printf "\033[32m")
yellow=$(printf "\033[33m")
blue=$(printf "\033[34m")
magenta=$(printf "\033[35m")
cyan=$(printf "\033[36m")
white=$(printf "\033[37m")
normal=$(printf "\033[m")

normalChars=$(printf "\033[0m")
boldChars=$(printf "\033[1m")
underlinedChars=$(printf "\033[4m")
blinkingChars=$(printf "\033[5m")
*/


int FileAppend(const char * filename,const char * msg)
{
  FILE * pFile = fopen (AccessLog, "a");
  if (pFile==0) { return 0; }

  fprintf(pFile,"%s\n",msg);
  fclose(pFile);
}


void error(const char * msg)
{
 fprintf(stderr,RED " ERROR MESSAGE : %s\n" NORMAL,msg);
 return;
}


void warning(const char * msg)
{
 fprintf(stderr,YELLOW " WARNING MESSAGE : %s\n" NORMAL,msg);
 return;
}


int AccessLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    if (!AccessLogEnable) { return 0; }
    FILE * pFile = fopen (AccessLog, "a");
    if (pFile==0) { return 0; }


    char nullIP[10]={"0.0.0.0"};
    const char * ipPTR = 0;
    if ( IP !=0 ) { ipPTR  = IP; } else
                  { ipPTR  = nullIP;  }

    char autoDateBuffer[64]={"Unk Unk 0 0:0:0 Unkn"};
    const char * datePtr = 0;
    if ( DateStr !=0 ) { datePtr = DateStr; } else
                       {
                         time_t t = time(NULL);
                         struct tm *tm = localtime(&t);
                         strftime(autoDateBuffer, sizeof(autoDateBuffer), "%c", tm);
                         datePtr = autoDateBuffer;
                        }


    char nullBuffer[10]={" "};
    const char * locationPtr = 0;
    if ( Location !=0 ) { locationPtr = Location; } else
                        { locationPtr = nullBuffer;  }


    const char * UseragentPtr = 0;
    if ( Useragent !=0 ) { UseragentPtr = Useragent; } else
                         { UseragentPtr = nullBuffer;  }


    const char * RequestPtr = 0;
    if ( Request !=0 ) { RequestPtr = Request; } else
                       { RequestPtr = nullBuffer;  }

    fprintf(pFile,"%s - - [%s] \"%s\" %u %u \"%s\" \"%s\"\n",
                  ipPTR,
                  datePtr,
                  RequestPtr,
                  ResponseCode,
                  (unsigned int) ResponseLength,
                  locationPtr,
                  UseragentPtr
             );

    fclose(pFile);
    return 1;
}


int ErrorLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    if (!ErrorLogEnable) { return 0; }
    FILE * pFile = fopen (ErrorLog, "a");
    if (pFile==0) { return 0; }



    char nullIP[10]={"0.0.0.0"};
    const char * ipPTR = 0;
    if ( IP !=0 ) { ipPTR  = IP; } else
                  { ipPTR  = nullIP;  }

    char autoDateBuffer[64]={"Unk Unk 0 0:0:0 Unkn"};
    const char * datePtr = 0;
    if ( DateStr !=0 ) { datePtr = DateStr; } else
                       {
                         time_t t = time(NULL);
                         struct tm *tm = localtime(&t);
                         strftime(autoDateBuffer, sizeof(autoDateBuffer), "%c", tm);
                         datePtr = autoDateBuffer;
                       }


    char nullBuffer[10]={" "};
    const char * locationPtr = 0;
    if ( Location !=0 ) { locationPtr = Location; } else
                        { locationPtr = nullBuffer;  }


    const char * UseragentPtr = 0;
    if ( Useragent !=0 ) { UseragentPtr = Useragent; } else
                         { UseragentPtr = nullBuffer;  }


    const char * RequestPtr = 0;
    if ( Request !=0 ) { RequestPtr = Request; } else
                       { RequestPtr = nullBuffer;  }

    fprintf(pFile,"%s - - [%s] \"%s\" %u %u \"%s\" \"%s\"\n",
                  ipPTR,
                  datePtr,
                  RequestPtr,
                  ResponseCode,
                  (unsigned int) ResponseLength,
                  locationPtr,
                  UseragentPtr
             );

    fclose(pFile);
    return 1;
}
