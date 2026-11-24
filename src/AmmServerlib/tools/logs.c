
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
// --------------------------------------------
#include "logs.h"
#include "http_tools.h"
#include "../server_configuration.h"



char *errorIDs[] =
 {
    //Be careful not to forget a comma beacause the strings will get concatenated and no compilation error will occur
    //( which is exactly what had happened after /* 5*/ and /*22*/ : C glued each of them onto the next entry ,
    //  so the array was shorter than this list and every id from 6 upwards printed the message belonging to a
    //  different one - e.g. an out-of-descriptors error reported itself as "Unable to serve request" )
    /* 0*/  "Failed to resolve GET Request",
    /* 1*/  "Instance not allocated",
    /* 2*/  "Request not allocated",
    /* 3*/  "Error binding master port, Server is already running?",
    /* 4*/  "UNIX does not allow binding a port below 1000 with a non root UID.. \n We tried to do it though.. \n Run with sudo or bind to a port higher than 1000",
    /* 5*/  "Our CHANGE_TO_UID non-root value is also a super user UID , we are forced to set a bogus non-root value..\n",
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
    /*22*/  "Call made without enough input",
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
     snprintf(filenameToTest,2048,"mv %s %s.%u && gzip -f %s.%u && rm %s.%u",filename,filename,i,filename,i,filename,i);
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


//See LOG_SHARD_COUNT's doc comment ( server_configuration.h ) for what/why this splits across.
static void GetShardedLogFilename(const char * baseFilename,unsigned int shardIndex,char * out,unsigned int outSize)
{
    if (LOG_SHARD_COUNT<=1) { snprintf(out,outSize,"%s",baseFilename); }
    else
    {
      //The %.*s precision keeps room for the ".%u" suffix , so a very long baseFilename gets its
      //tail cut instead of silently losing the shard number and collapsing every shard onto one file
      //( it also lets the compiler prove no truncation happens here , which -Wformat-truncation wants )
      snprintf(out,outSize,"%.*s.%u",(int) (outSize-12),baseFilename,shardIndex);
    }
}

//Which shard this calling thread's log line goes to - deliberately gettid() ( a small, sequentially-assigned
//kernel thread ID ) and not pthread_self() : pthread_t is glibc's pointer to the thread's control block, and
//thread stacks are allocated on aligned boundaries - found live that its low bits are always 0, so
//`pthread_self() % LOG_SHARD_COUNT` collapsed every single thread onto shard 0, defeating the entire point.
//gettid() has no such alignment, and different threads get different, non-power-of-two-aligned small integers.
static unsigned int GetLogShardForCurrentThread(void)
{
    if (LOG_SHARD_COUNT<=1) { return 0; }
    //syscall(SYS_gettid) directly rather than glibc's gettid() wrapper : the wrapper needs _GNU_SOURCE feature-
    //test-macro visibility to even be declared, which isn't guaranteed under this project's build flags ( showed
    //up as an ugly "implicit declaration" warning ) - the raw syscall avoids that dependency entirely.
    return (unsigned int)( (unsigned long)syscall(SYS_gettid) % LOG_SHARD_COUNT );
}

//One shard of the access or error log. Lines are formatted straight into `buffer` under `lock` ( a memcpy , no
//syscall ) and only ever leave it as whole lines , in one write() on an O_APPEND descriptor. stdio's _IOFBF used
//to cut its buffer at whatever byte it filled up on , so when two processes appended to the same shard file one
//of them could land its write between the two halves of the other's line ( seen live : "89.210.253.23127.0.0.1" ).
//O_APPEND write()s to a local file are positioned and applied atomically , so whole-line writes can no longer splice.
struct LogShard
{
    int isOpen;
    int fd;
    unsigned int used;       //Bytes of complete , not yet written lines in buffer
    unsigned int polls;      //Lines appended , drives the compressLog() size check
    time_t oldestUnwritten;  //When the first line now waiting in buffer was added
    char buffer[LOG_WRITE_BUFFER_SIZE];
};

static struct LogShard AccessLogShard[LOG_SHARD_COUNT] = {0};
static pthread_mutex_t AccessLogMutex[LOG_SHARD_COUNT] = { [0 ... LOG_SHARD_COUNT-1] = PTHREAD_MUTEX_INITIALIZER };
static struct LogShard ErrorLogShard[LOG_SHARD_COUNT] = {0};
static pthread_mutex_t ErrorLogMutex[LOG_SHARD_COUNT] = { [0 ... LOG_SHARD_COUNT-1] = PTHREAD_MUTEX_INITIALIZER };

//Caller holds the shard's mutex
static void WriteOutLogShard(struct LogShard * shard)
{
    unsigned int done=0;
    while ( (shard->isOpen) && (done<shard->used) )
    {
        ssize_t wrote = write(shard->fd,shard->buffer+done,shard->used-done);
        if (wrote>0) { done+=(unsigned int) wrote; } else
        if ( (wrote<0) && (errno==EINTR) ) { continue; } else
                                           { fprintf(stderr,"Failed writing log , %u bytes dropped\n",shard->used-done); break; }
    }
    shard->used=0;
}

//Caller holds the shard's mutex
static void CloseLogShard(struct LogShard * shard)
{
    WriteOutLogShard(shard);
    if (shard->isOpen) { close(shard->fd); shard->isOpen=0; }
}

static int FormatLogLine(char * out,unsigned int outSize,const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    return snprintf(out,outSize,"%s - - [%s] \"%s\" %u %lu \"%s\" \"%s\"\n",IP,DateStr,Request,ResponseCode,ResponseLength,Location,Useragent);
}

static int LogAppend(struct LogShard * shards,pthread_mutex_t * mutexes,const char * baseFilename,const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    unsigned int shardID = GetLogShardForCurrentThread();
    struct LogShard * shard = &shards[shardID];
    pthread_mutex_lock(&mutexes[shardID]);

    char shardFilename[MAX_FILE_PATH];
    GetShardedLogFilename(baseFilename,shardID,shardFilename,sizeof(shardFilename));

    if ( shard->polls % POLL_LOG_SIZES_EVERY_X_ACCESSES == 0 )
    {
        //compressLog may rename/gzip this file from under us , so write out and release our handle to it before
        //calling it and let it get lazily reopened ( against the fresh file ) right below
        CloseLogShard(shard);
        compressLog(shardFilename,shard->polls);
    }

    if (!shard->isOpen)
    {
        shard->fd = open(shardFilename,O_WRONLY|O_CREAT|O_APPEND|O_CLOEXEC,0666);
        if (shard->fd<0) { pthread_mutex_unlock(&mutexes[shardID]); return 0; }
        shard->isOpen=1;
    }

    time_t now = time(NULL);

    char autoDateBuffer[64]={"Unk Unk 0 0:0:0 Unkn"};
    if (DateStr==0)
    {
        struct tm tmNow;
        if (localtime_r(&now,&tmNow)!=0) { strftime(autoDateBuffer,sizeof(autoDateBuffer),"%c",&tmNow); }
        DateStr = autoDateBuffer;
    }
    if (IP==0)        { IP="0.0.0.0"; }
    if (Request==0)   { Request=" "; }
    if (Location==0)  { Location=" "; }
    if (Useragent==0) { Useragent=" "; }

    unsigned int room = LOG_WRITE_BUFFER_SIZE - shard->used;
    int length = FormatLogLine(shard->buffer+shard->used,room,IP,DateStr,Request,ResponseCode,ResponseLength,Location,Useragent);
    if ( (length>=0) && ((unsigned int) length>=room) )
    {
        //Does not fit behind the lines already waiting : write those out and format again into the empty buffer
        WriteOutLogShard(shard);
        room = LOG_WRITE_BUFFER_SIZE;
        length = FormatLogLine(shard->buffer,room,IP,DateStr,Request,ResponseCode,ResponseLength,Location,Useragent);
        //A single line longer than the whole buffer gets cut , but still ends in '\n'
        if ( (length>=0) && ((unsigned int) length>=room) ) { length=(int) room-1; shard->buffer[length-1]='\n'; }
    }
    if (length<0) { pthread_mutex_unlock(&mutexes[shardID]); return 0; }

    if (shard->used==0) { shard->oldestUnwritten=now; }
    shard->used += (unsigned int) length;
    ++shard->polls;

    //See LOG_LINE_BUFFERED / LOG_MAX_UNWRITTEN_SECONDS ( server_configuration.h ) for when lines leave the buffer
    if ( (LOG_LINE_BUFFERED) || (now - shard->oldestUnwritten >= LOG_MAX_UNWRITTEN_SECONDS) ) { WriteOutLogShard(shard); }

    pthread_mutex_unlock(&mutexes[shardID]);
    return 1;
}

int AccessLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    if (!AccessLogEnable) { return 0; }
    return LogAppend(AccessLogShard,AccessLogMutex,AccessLog,IP,DateStr,Request,ResponseCode,ResponseLength,Location,Useragent);
}

int ErrorLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent)
{
    if (!ErrorLogEnable) { return 0; }
    return LogAppend(ErrorLogShard,ErrorLogMutex,ErrorLog,IP,DateStr,Request,ResponseCode,ResponseLength,Location,Useragent);
}


void FlushAccessAndErrorLogs(void)
{
    unsigned int i;
    for (i=0; i<LOG_SHARD_COUNT; i++)
    {
        pthread_mutex_lock(&AccessLogMutex[i]);
        WriteOutLogShard(&AccessLogShard[i]);
        pthread_mutex_unlock(&AccessLogMutex[i]);
    }

    for (i=0; i<LOG_SHARD_COUNT; i++)
    {
        pthread_mutex_lock(&ErrorLogMutex[i]);
        WriteOutLogShard(&ErrorLogShard[i]);
        pthread_mutex_unlock(&ErrorLogMutex[i]);
    }
}

//Writes out and closes every log shard. A later append ( another instance still running ) lazily reopens its shard.
void CloseAccessAndErrorLogs(void)
{
    unsigned int i;
    for (i=0; i<LOG_SHARD_COUNT; i++)
    {
        pthread_mutex_lock(&AccessLogMutex[i]);
        CloseLogShard(&AccessLogShard[i]);
        pthread_mutex_unlock(&AccessLogMutex[i]);
    }

    for (i=0; i<LOG_SHARD_COUNT; i++)
    {
        pthread_mutex_lock(&ErrorLogMutex[i]);
        CloseLogShard(&ErrorLogShard[i]);
        pthread_mutex_unlock(&ErrorLogMutex[i]);
    }
}
