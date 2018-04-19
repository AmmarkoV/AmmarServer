
#include <stdlib.h>
#include <time.h>
// --------------------------------------------
#include "logs.h"
#include "http_tools.h"
#include "../server_configuration.h"



char *errorIDs[] =
 {
    //Be careful not to forget a comma beacause the strings will get concatenated and no compilation error will occur
    /* 0*/  "Failed to resolve GET Request",
    /* 1*/  "Instance not allocated",
    /* 2*/  "Request not allocated",
    /* 3*/  "Error binding master port, Server is already running?",
    /* 4*/  "UNIX does not allow binding a port below 1000 with a non root UID.. \n We tried to do it though.. \n Run with sudo or bind to a port higher than 1000",
    /* 5*/  "Our CHANGE_TO_UID non-root value is also a super user UID , we are forced to set a bogus non-root value..\n"
    /* 6*/  "Failed to create thread",
    /* 7*/  "Failed to create socket",
    /* 8*/  "Failed to bind socket",
    /* 9*/  "Failed to listen on socket",
    /*10*/  "Failed to accept socket connection",
    /*11*/  "Failed to connect to socket",
    /*12*/  "Out of resource to accomodate client",
    /*13*/  "Unable to serve request , closing connections..",
    /*14*/  "Unable to serve template, closing connections..\n",
    /*15*/  "Failed to receive HTTP header",
    /*16*/  "Unauthorized Request",
    /*17*/  "No Callback registered , cannot serve content",
    /*18*/  "Dynamic request does pointer is corrupted",
    /*19*/  "Could not set socket timeout",
    /*20*/  "Could not set socket options",
    /*21*/  "Server is running as a root user, this is a security liability",
    /*22*/  "Call made without enough input"
    /*23*/  "Could not allocate memory",
    /*24*/  "Not enough memory allocated",
    /*25*/  "ASV_ERROR_REALLOCATION_R_X86_64_PC32_GCC_ERROR",
    /*26*/  "Shared content has an invalid RH_Scenario flag , this version of AmmarServer does not know what it means\nMaybe this has to do with a newer version , and stuff that haven't been invented yet in this build..",
    /*27*/  "Bug(?) detected , no cache payload\n",
    /*28*/  "Cache not allocated, so cannot be used",
    /*29*/  "Could not create resource in cache",
    /*30*/  "Missing or Wrong range request detected",
    /*31*/  "Could not find a threadID",
    /*32*/  "Could not find filesize",
    /*33*/  "Timed out while waiting for a new thread to get created and consume its message",
    /*34*/  "Failed to cancel new thread creation" ,
    /*35*/  "Failed to pass thread context, this is probably an internal bug due to race conditions",
    /*36*/  "ASV_ERROR_INNETOP_FAILED",
    /*37*/  "ReducePathSlashes_Inplace failed to reduce slashes..",
    /*38*/  "Failed to add NO cache rule..",
    /*39*/  "Dynamic request client caused an overflow, This is not the server`s fault and has been caused by the programmer not thinking well about setting limits @ AmmServer_AddResourceHandler "
  };

char *warningIDs[] =
 {
   //Be careful not to forget a comma beacause the strings will get concatenated and no compilation error will occur
   /* 0*/  "Tried to perform dynamicRequest_serveContent, but got back null , if there is no regular fallback file request will probably 404\n",
   /* 1*/  "Client connection closed while waiting for keepalive..",
   /* 2*/  "Connection closed , while transmitting the file",
   /* 3*/  "Reached max transmission stall, stopping file transmission",
   /* 4*/  "Could not transmit error to client",
   /* 5*/  "Client Denied access to resource",
   /* 6*/  "Pretending that this is a GET request hack that needs fixing",
   /* 7*/  "Main AmmarServer Thread Stopped..",
   /* 8*/  "Client BUG : Size of request appears to be zero.. \n ",
   /* 9*/  "PreSpawning Threads is disabled , call executable with -prefork X ( with X more than 0 ) to enable them if you target heavy loads..",
   /*10*/  "Not going to call callback function with an empty buffer..!",
   /*11*/  "Randomizer was not random..",
   /*12*/  "Creating a new resource cache definition, just to store a rule there",
   /*13*/  "AmmServer_Stop started",
   /*14*/  "AmmServer_Stop completed",
   /*15*/  "Enabling monitor",
   /*16*/  "Unrecognized request detected",
   /*17*/  "We received a request which is not currently implemented",
   /*18*/  "Predatory request received" ,
   /*19*/  "Wrong Argument",
   /*20*/  "Buffer underflow",
   /*21*/  "Reached the maximum buffer size"
  };




unsigned int logAccessPolls=0;
unsigned int logErrorPolls=0;
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
  FILE * pFile = fopen (filename, "a");
  if (pFile==0) { return 0; }

  fprintf(pFile,"%s\n",msg);
  fclose(pFile);
  return 1;
}

void errorID(int id)
{
 fprintf(stderr,RED " ERROR MESSAGE : %s\n" NORMAL,errorIDs[id]);
}

void warningID(int id)
{
 fprintf(stderr,YELLOW " WARNING MESSAGE : %s\n" NORMAL,warningIDs[id]);
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


int compressLog(const char * filename,unsigned int accessTimes)
{
  if (accessTimes%POLL_LOG_SIZES_EVERY_X_ACCESSES!=0) { return 0; } //Skip this most of the time
  if (FileSizeAmmServ(filename) < COMPRESS_LOG_FILE_AFTER_THIS_SIZE) { return 0; } //Skip this if the file is not big enough

 //Ok , lets compress..
  char filenameToTest[2048]={0};
  unsigned int i=1;
  while (i<100000)
  {
   snprintf(filenameToTest,2048,"%s.%u.gz",filename,i);
   if (!FileExistsAmmServ(filenameToTest))
   {
     //The last part ( rm %s.%u ) is not needed with regular gzip since it removes the source file ,
     //however it does not hurt to exist to make sure nothing remains that can screw up things..
     snprintf(filenameToTest,2048,"mv %s %s.%u &&&& gzip -f %s.%u &&&& rm %s.%u",filename,filename,i,filename,i,filename,i);
     i=system(filenameToTest);
     if (i==0)
     {
      fprintf(stderr,"Succeeded compacting log file %s \n",filename);
      return 1;
     } else
     {
      fprintf(stderr,"Failed to compact log file %s \n",filename);
      return 0;
     }
   }
    ++i;
  }
 return 0;
}


int AccessLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    if (!AccessLogEnable) { return 0; }

    compressLog(AccessLog,logAccessPolls);

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

    ++logAccessPolls;
    fclose(pFile);
    return 1;
}


int ErrorLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    if (!ErrorLogEnable) { return 0; }
    FILE * pFile = fopen (ErrorLog, "a");
    if (pFile==0) { return 0; }

    compressLog(ErrorLog,logErrorPolls);


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

    ++logErrorPolls;
    fclose(pFile);
    return 1;
}
