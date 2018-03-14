
#include <stdlib.h>
#include <time.h>
// --------------------------------------------
#include "logs.h"
#include "http_tools.h"
#include "../server_configuration.h"



char *errorIDs[] =
 {
     "Failed to resolve GET Request",
     "Instance not allocated",
     "Request not allocated",
     "Error binding master port, Server is already running?",
     "UNIX does not allow binding a port below 1000 with a non root UID.. \n We tried to do it though.. \n Run with sudo or bind to a port higher than 1000",
     "Our CHANGE_TO_UID non-root value is also a super user UID , we are forced to set a bogus non-root value..\n"
     "Failed to create thread",
     "Failed to create socket",
     "Failed to bind socket",
     "Failed to listen on socket",
     "Failed to accept socket connection",
     "Failed to connect to socket",
     "Out of resource to accomodate client",
     "Unable to serve request , closing connections..",
     "Unable to serve template, closing connections..\n",
     "Failed to receive HTTP header",
     "Unauthorized Request",
     "No Callback registered , cannot serve content",
     "Dynamic request does pointer is corrupted",
     "Could not set socket timeout",
     "Could not set socket options",
     "Server is running as a root user, this is a security liability",
     "Call made without enough input"
     "Could not allocate memory",
     "Not enough memory allocated",
     "ASV_ERROR_REALLOCATION_R_X86_64_PC32_GCC_ERROR",
     "Shared content has an invalid RH_Scenario flag , this version of AmmarServer does not know what it means\nMaybe this has to do with a newer version , and stuff that haven't been invented yet in this build..",
     "Bug(?) detected , no cache payload\n",
     "Cache not allocated, so cannot be used",
     "Could not create resource in cache",
     "Missing or Wrong range request detected",
     "Could not find a threadID",
     "Could not find filesize",
     "Timed out while waiting for a new thread to get created and consume its message",
     "Failed to cancel new thread creation" ,
     "Failed to pass thread context, this is probably an internal bug due to race conditions",
     "ASV_ERROR_INNETOP_FAILED"
  };

char *warningIDs[] =
 {
     "Tried to perform dynamicRequest_serveContent, but got back null , if there is no regular fallback file request will probably 404\n"
     "Client connection closed while waiting for keepalive..",
     "Connection closed , while transmitting the file",
     "Reached max transmission stall, stopping file transmission",
     "Could not transmit error to client",
     "Client Denied access to resource",
     "Pretending that this is a GET request hack that needs fixing",
     "Main AmmarServer Thread Stopped..",
     "Client BUG : Size of request appears to be zero.. \n ",
     "PreSpawning Threads is disabled , call executable with -prefork X ( with X more than 0 ) to enable them if you target heavy loads..",
     "Not going to call callback function with an empty buffer..!",
     "Randomizer was not random..",
     "Creating a new cache definition, just to store a rule there",
     "AmmServer_Stop started",
     "AmmServer_Stop completed",
     "Enabling monitor",
     "Unrecognized request detected",
     "We received a request which is not currently implemented",
     "Predatory request received"
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
 fprintf(stderr,YELLOW " WARNING MESSAGE : %s\n" NORMAL,errorIDs[id]);
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
