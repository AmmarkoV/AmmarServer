#include <stdio.h>
#include "logs.h"
#include "configuration.h"

//This is a sample Apache access.log entry
//184.82.219.85 - - [05/Feb/2012:07:28:19 +0200] "GET /~ammar/guard_dog_project/blogs/index.php/2010/02/22/video-room-test?blog=1 HTTP/1.0" 301 528 "-" "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)"
//99.100.183.119 - - [05/Feb/2012:07:40:22 +0200] "GET /~ammar/guard_dog_project/blogs/skins/evopress/img/kubrickbgcolor.jpg HTTP/1.1" 200 2135 "http://ammar.gr/gddg/" "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)"


int AccessLogAppend(char * IP,char * DateStr,char * Request,unsigned int ResponseCode,unsigned long ResponseLength,char * Location,char * Useragent)
{
    if (!AccessLogEnable) { return 0; }
    FILE * pFile = fopen (AccessLog, "a");
    if (pFile==0) { return 0; }

    fprintf(pFile,"%s - - [%s] \"%s\" %u %u \"%s\" \"%s\"\n",
                  IP,
                  DateStr,
                  Request,
                  ResponseCode,
                  (unsigned int) ResponseLength,
                  Location,
                  Useragent
             );

    fclose(pFile);
    return 1;
}


int ErrorLogAppend(char * IP,char * DateStr,char * Request,unsigned int ResponseCode,unsigned long ResponseLength,char * Location,char * Useragent)
{
    if (!ErrorLogEnable) { return 0; }
    FILE * pFile = fopen (AccessLog, "a");
    if (pFile==0) { return 0; }

    fprintf(pFile,"%s - - [%s] \"%s\" %u %u \"%s\" \"%s\"\n",
                  IP,
                  DateStr,
                  Request,
                  ResponseCode,
                  (unsigned int) ResponseLength,
                  Location,
                  Useragent
             );

    fclose(pFile);
    return 1;
}
