/** @file server_configuration.h
* @brief The Main Header for the settings used by AmmarServer
*
* Take extra care when changing something here , since its impact is global
*
* @author Ammar Qammaz (AmmarkoV)
* @bug Server configuration at some point should be ported from defines to a per instance configuration file , some of these defines will always remain since they control global allocations


*/
#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED

#include "AmmServerlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Enable POST request handling , switching this to 0 will completely deny them reducing attack surface */
#define MASTER_ENABLE_POST 1
#if MASTER_ENABLE_POST
 // #warning "POST Support is under construction and unsafe to use ( for now )"
#endif // MASTER_ENABLE_POST


/** @brief Enable a variety of debug messages in parts of the code that are not 100% bulletproof*/
#define DISABLE_DYNAMIC_REQUESTS 0

/** @brief Enable a variety of debug messages in parts of the code that are not 100% bulletproof*/
#define DEBUG_MESSAGES 0

/** @brief memset to 0 chunks of memory that might contain sensitive client data before freeing them and releasing them
           this adds a little overhead for each free operation but improves security */
#define CLEAN_MEMORY_BEFORE_DEALLOCATION 0


/** @brief Use qsort and bsearch in hashmaps to speed up results in big caches , should be 1 if we trust them to work correctly..!*/
#define USE_SORTING_IN_HASH_MAPS 0

/** @brief Redeclares a function that causes linking problems..   */
#define WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR 1


/** @brief Don't do reallocs this disables part of the functionality but could be more .   */
#define DO_NOT_ALLOW_MEMORY_REALLOCATIONS 0


/** @brief Time sleeping when a dynamic request that serves a common file across all clients is busy   */
#define CLIENT_SLEEP_TIME_WHEN_DYNAMIC_REQUEST_CALLBACK_IS_BUSY_NSEC 500000

/** @brief Time sleeping when a dynamic request that serves a common file across all clients is busy   */
#define CLIENT_SLEEP_TIME_INTERVAL_NSEC 5000


/** @brief Sleep time while waiting for new thread to kick in and read parameters to unblock main thread.. */
#define THREAD_SLEEP_TIME_WHILE_WAITING_FOR_NEW_CREATED_THREAD_TO_CONSUME_PARAMETERS 20


/** @brief Setting this to 1 will signal that all instances of AmmarServer need to die at once */
extern unsigned int GLOBAL_KILL_SERVER_SWITCH;

/** @brief Prespawned theads reduce overall latency but they increase CPU load  , 0 disables them */
extern unsigned int MAX_CLIENT_PRESPAWNED_THREADS;

/** @brief Prespawned theads reduce overall latency but they increase CPU load  , 0 disables them */
#define MAX_CLIENT_PRESPAWNED_THREADS_DEFAULT 0 //<- Disabled for now This is the number of prespawned threads that run to reduce overall latency

/** @brief Maximum Target of concurrent clients being listened at the same time C10K tests require this to be 10000 ( http://en.wikipedia.org/wiki/C10k_problem ) */
#define MAX_CLIENTS_LISTENING_FOR 10000 //C10K :P

/** @brief Maximum Number of concurrent threads being created at the same time , depending on the size of the listen pool this can be smaller than the MAX_CLIENTS_LISTENING_FOR and connections will
           be queued and served sequentially */
#define MAX_CLIENT_THREADS 3000 //3000 //This is the maximum number of simultaneous regular threads that serve incoming requests..!


/** @brief Calculate (And output) transmission speed for files broadcast by AmmarServer  */
#define CALCULATE_TIME_FOR_UPLOADS 1

/** @brief Precompiler switch that controls baking in ( or not ) the client list capabilities , currently disabled since client lists are not yet implemented */
#define COMPILE_WITH_CLIENT_LIST 0

/** @brief Sleep time after unsuccessfully trying to bind to port ( usleep(DELAY... ) */
#define DELAY_TRY_BINDING_TO_PORT 5000 *1000

/** @brief Maximum times to try to bind to port on initial server start up */
#define MAX_TRIES_TO_BIND_TO_PORT 5


#if MAX_CLIENT_THREADS == 1
   #define SINGLE_THREAD_MODE 1
#endif // MAX_CLIENT_THREADS

/** @brief Maximum connections per IP , this is a little dangerous since multiple PC's can have a single gateway , but it is a good heuristic to better share resources
           @bug MAX_CLIENTS_PER_IP is not used if there is no client list declared */
#define MAX_CLIENTS_PER_IP 3 //<- Not implemented yet

/** @brief An incoming header should not have more than X numbers of lines
    @ingroup security*/
#define MAX_HTTP_REQUEST_HEADER_LINES 1024 // A Header must not contain more than 1024 lines of directives
/** @brief Max slashes in a Resource ( i.e. http://xxx.xxx.xxx.xxx/test/resource has 4 slashes
    @ingroup security */
#define MAX_RESOURCE_SLASHES 15

/** @brief Maximum line length in configuration file */
#define MAX_CONFIGURATION_FILE_LINE_SIZE 512

/** @brief Maximum length of a content type record*/
#define MAX_CONTENT_TYPE 128

/** @brief Length of blocks allocated , read and sent in order to transmit a file to a client  , bigger values read faster from the disk and possibly better utilize
           bandwidth in the expense of memory consumption */
#define MAX_FILE_READ_BLOCK_KB 1024 //1MB - How much KB is the max fread , malloc for serving files


/** @brief Maximum size of an incoming HTTP Header */
#define MAX_HTTP_REQUEST_HEADER 8/*KB*/*1024

/** @brief Maximum size of an incoming HTTP Header allocation step */
#define HTTP_POST_GROWTH_STEP_REQUEST_HEADER 512/*KB*/*1024

/** @brief Maximum size of an incoming POST Header , since it carries files this should be big enough ( say 4 MB )  */
#define DEFAULT_MAX_HTTP_POST_REQUEST_HEADER 4/*MB*/*1024*1024


/** @brief This enables e-tag randomization on each creation of a cache , this makes clients automatically refresh when server is restarted */
#define RANDOMIZE_ETAG_PER_LAUNCH 1

/** @brief Maximum size of an E-Tag */
#define MAX_ETAG_SIZE 128


/** @brief Maximum size of an http header reply */
#define MAX_HTTP_REQUEST_HEADER_REPLY 1024

/** @brief Maximum size of a short , static ,  http header reply */
#define MAX_HTTP_REQUEST_SHORT_HEADER_REPLY 512

/** @brief Controls initial allocated size for a directory listing */
#define INITIAL_DIRECTORY_LIST_RESPONSE_BODY 64/*KB*/*1024
/** @brief Controls allocation step for when we run out of space for a directory listing */
#define GROWSTEP_DIRECTORY_LIST_RESPONSE_BODY 16/*KB*/*1024
/** @brief Maximum space allocated for a directory listing */
#define MAX_DIRECTORY_LIST_RESPONSE_BODY 256/*KB*/*1024

/** @brief When we compress a file we may have a buffer allocated for 16KB and the compressed size might be 1.6KB ( if we get an impressive 1:10 ratio )
           If that's the case we could do a system call to free memory and allocate a 1.6KB chunk of memory thus being economic in memory requirements*/
#define REALLOC_TO_SAVE_MORE_THAN_THIS_NUMBER_BYTES 4096 //This may happen during compression in file_caching.c

/** @brief If this enabled and we haven't specified a configuration file we will try to open an ammarServer.conf */
#define ENABLE_AUTOMATIC_CONFIGURATION_LOADING 1

/** @brief Enable Compression using ZLib , this increases CPU usage , code surface , requires the zlib library to be linked , but on the other hand conserves bandwidth and memory */
#define ENABLE_COMPRESSION 0 //Compression doesn't work all that well yet

/** @brief Enable Compression for dynamic content , this can be tuned per dynamic resource , but this is a global switch for all nodes
           This generally doesnt seem like a very good idea unless you have a dynamic html file of 20KB+ with very rare changes to compensate for the overhead */
#define ENABLE_DYNAMIC_CONTENT_COMPRESSION 0


/** @brief In order to bind ports under 1000 , a process needs to have Super user UID , after we bind the port we *really* don't want
           to have our process running as a super user , it is a serious security liability
           This should always be 1
    @ingroup security */
#define ENABLE_DROPPING_ROOT_UID_IF_ROOT 1
#ifndef ENABLE_DROPPING_ROOT_UID_IF_ROOT
 #warning "This version of AmmarServer is compiled *NOT* to drop super user privilleges , this is a security liability"
#endif

/** @brief If this is enabled we will always change our UID no matter if we are a super user or not ( if this is disabled only super user processes will get the UID change )
    @ingroup security */
#define ENABLE_DROPPING_UID_ALWAYS 0

/** @brief Default Username to change to if we are running from root
    @ingroup security */
#define DEFAULT_USERNAME_UID_FOR_DAEMON "www-data"

/** @brief Default Username that initially gets set to DEFAULT_USERNAME_UID_FOR_DAEMON but can be changed through a configuration file
    @ingroup security */
extern char USERNAME_UID_FOR_DAEMON[MAX_FILE_PATH];

/** @brief Non Root UID to change to
    @ingroup security */
#define NON_ROOT_UID_IF_USER_FAILS 1500

extern int CHANGE_TO_UID; //This is the default UID to use when USERNAME_UID_FOR_DAEMON doesn't provide a useful UID..

/** @brief Resolve internal resources to redirect them to point templates ( this should always be 1 , although its implementation is a little dodgy right now )  */
#define ENABLE_INTERNAL_RESOURCES_RESOLVE 1

/** @brief Enable directory listing , if this is disabled attack surface gets significantly reduced
    @ingroup security */
#define ENABLE_DIRECTORY_LISTING 1


/**  @brief TM structures carry the year after 1900 (see http://www.cplusplus.com/reference/ctime/tm/ )  so  this is encoded here as a reminder
     @ingroup security */
#define EPOCH_YEAR_IN_TM_YEAR 1900


/**  @brief Value that gets set from configuration files , and if it is non-zero it will trigger a priority change ( change nice value ) */
extern int CHANGE_PRIORITY;


/**  @brief Use Timeouts For Sending And Receiving */
#define USE_TIMEOUTS 1

/**  @brief Default timeout value before which a socket blocking on a read call should be considered dead */
#define DEFAULT_SOCKET_READ_TIMEOUT_SECS 8
extern int varSocketTimeoutREAD_seconds;

/**  @brief Default timeout value before which a socket blocking on a write call should be considered dead */
#define DEFAULT_SOCKET_WRITE_TIMEOUT_SECS 8
extern int varSocketTimeoutWRITE_seconds;

/**  @brief Max Send requests that return without transmitting anything */
#define MAX_TRANSMISSION_STALL 1
// ----------------- CACHE OPTIONS -----------------

/**  @brief If caching is disabled server becomes a very simple file server , dynamic requests are also disabled*/
extern unsigned char CACHING_ENABLED;


/** @brief Maximum Number of separate items in cache ( per instance of AmmarServer )*/
extern int MAX_SEPERATE_CACHE_ITEMS;
/** @brief Maximum memory usage ( Megabytes ) for the entire cache ( per instance of AmmarServer )  */
extern int MAX_CACHE_SIZE_IN_MB;
/** @brief Maximum memory usage ( Megabytes ) for a specific entry of the cache ( per instance of AmmarServer ) */
extern int MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB;
// ----------------- SESSION OPTIONS -----------------

/** @brief Number of random bytes a new session token is generated from ( see AmmServer_GenerateSecureToken() )*/
#define SESSION_TOKEN_RANDOM_BYTES 32
/** @brief Buffer size needed to hold a session token string ( base64url of SESSION_TOKEN_RANDOM_BYTES + NUL ),
           rounded up generously so callers don't need to compute the exact base64 expansion themselves */
#define SESSION_TOKEN_STRING_SIZE 64
/** @brief Name of the cookie the session token travels in */
#define SESSION_COOKIE_NAME "AMMSESSID"

/** @brief A session with no activity for this many seconds is treated as expired and lazily evicted on next touch */
extern int SESSION_IDLE_TIMEOUT_SECONDS;
/** @brief Maximum number of simultaneous sessions kept in memory ( per instance of AmmarServer ) - past this,
           the single oldest ( by last activity ) session is evicted to make room for a new one */
extern int MAX_SESSIONS;



/**  @brief Max size of a log file before it gets compressed..!*/
#define COMPRESS_LOG_FILE_AFTER_THIS_SIZE 10 * /*MB*/ 1024 * 1027

/**  @brief Number of accesses between log checks..!*/
#define POLL_LOG_SIZES_EVERY_X_ACCESSES 200

/** @brief Access/error log write buffering mode ( applied via setvbuf() in AccessLogAppend()/ErrorLogAppend(),
           tools/logs.c ) - a straight speed-vs-durability tradeoff, since every log write happens under a
           single global mutex ( AccessLogMutex/ErrorLogMutex ) shared by every request-serving thread :
             1 = _IOLBF ( line-buffered, the historical/safe default ). Every log line ends in '\n', so every
                 single logged request costs a real write() syscall *while holding that mutex* - under
                 concurrent load this serializes every thread through a syscall, one at a time, on every
                 request. Safest option : nothing can ever be lost except the one line ( if any ) that was
                 actively mid-write at the exact moment of an unrecognized-signal hard kill - everything before
                 it is already durably on disk the instant each fprintf() returns.
             0 = _IOFBF ( fully buffered, LOG_WRITE_BUFFER_SIZE bytes ). Log lines accumulate in a real
                 userspace buffer under the mutex - just a memcpy, no syscall - and only cost an actual write()
                 once that buffer fills ( or the log file gets reopened, e.g. by compressLog() ). Substantially
                 less lock-hold time and contention on the request-serving hot path under concurrent load. Cost:
                 up to one full LOG_WRITE_BUFFER_SIZE buffer's worth of the most recent, not-yet-flushed log
                 lines per log file can be lost - not just observability history, but also whatever evidence of
                 a request burst scripts/enforceBanlist.sh would otherwise have mined from those lines to ban an
                 abusive IP. AmmServer_Stop() and AmmServer_GlobalTerminationHandler() ( main.c ) both call
                 FlushAccessAndErrorLogs() ( tools/logs.h ) specifically to close this window on every
                 *recognized* clean-stop path ( including SIGINT/SIGHUP/SIGTERM - do NOT assume the C runtime's
                 own exit()-time stdio flush covers this on its own : verified live that it does not reliably
                 fire from AmmServer_GlobalTerminationHandler()'s signal-handler context, which is why the
                 explicit call exists ). The loss window only actually applies to a genuinely unrecognized kill
                 ( SIGKILL, a segfault, `kill -9`, power loss ).
           Benchmark this against your own traffic pattern before flipping it - see scripts/benchmark_ammarserver.sh. */
#define LOG_LINE_BUFFERED 0
/** @brief Buffer size used for log writes when LOG_LINE_BUFFERED is 0 ( _IOFBF ) - irrelevant when it's 1. */
#define LOG_WRITE_BUFFER_SIZE (64*1024)

/** @brief Number of independent access-log ( and, separately, error-log ) shards - each with its own FILE*, own
           mutex, and own on-disk file - that AccessLogAppend()/ErrorLogAppend() ( tools/logs.c ) split writes
           across, to cut lock contention under concurrent load : every logging thread hashes to exactly one
           shard ( currently `pthread_self() % LOG_SHARD_COUNT` ) and only ever contends with the other threads
           that hash to that *same* shard, not with all of them.
             1 = the historical, single-file/single-mutex behaviour, byte-for-byte - this is not an approximation,
                 the sharding logic is skipped entirely and the exact original filename is used with no suffix.
             >1 = that many files, named `<original>.0`, `<original>.1`, ... `<original>.N-1` ( same "append an
                 index" convention compressLog() already uses for rotated files, e.g. `access.log.3.gz` ), each
                 independently written, buffered ( per LOG_LINE_BUFFERED ), rotated ( per COMPRESS_LOG_FILE_AFTER_THIS_SIZE )
                 and flushed ( FlushAccessAndErrorLogs() covers every shard ). No coordination between shards at
                 all beyond the hash picking one - that's the entire point.
           Reconstituting one chronological log from N shards is a trivial k-way merge : open all N files, read
           the timestamp bracket `[...]` each line already carries, repeatedly take whichever open file's next
           line has the earliest timestamp. Not automated here - scripts/enforceBanlist.sh and anyone tailing
           logs by hand need to know to look at all N files (or merge them) once this is set above 1.
           Benchmark this against your own traffic pattern - see scripts/benchmark_ammarserver.sh - there's no
           reason to believe any particular N is right without measuring your own concurrency level. */
#define LOG_SHARD_COUNT 16


extern int  AccessLogEnable;
extern char AccessLog[MAX_FILE_PATH];

extern int  ErrorLogEnable;
extern char ErrorLog[MAX_FILE_PATH];


/** @brief String that corresponds to the template directory ( for directory_lists )
    @bug Please note that the file server has limits for filenames so this should not be very long
          _asvres_/filename.jpg is OK a filename like _asvres_/filenamemplampla.jpg will return a 404*/
#define TEMPLATE_INTERNAL_URI "_asvres_/"
extern char TemplatesInternalURI[MAX_RESOURCE];


/** @brief Check if we can commit more memory on an AmmarServer instance
    @param An AmmarServer instance
    @param Memory to additionally allocate
* @retval 1=Ok,0=Don'tAllocate*/
int instance_WeCanCommitMoreMemory(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes);

/** @brief Register a new memory Allocation to instance memory counters
    @param An AmmarServer instance
    @param Memory that was additionally allocated
* @retval 1=Success,0=Failure*/
int instance_CountNewMallocOP(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes);

/** @brief Register a new memory free operation to instance memory counters
    @param An AmmarServer instance
    @param Memory that was freed
* @retval 1=Success,0=Failure*/
int instance_CountFreeOP(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes);

/** @brief Printout disclaimer
  * @retval 1=Success,0=Failure*/
int printDisclaimer();

/** @brief Internal check of server configuration and possible error messages in impossible situations
  * @retval 1=Success,0=Failure*/
int EmmitPossibleConfigurationWarnings();

/** @param Load a configuration file
    @bug   LoadConfigurationFiles etc is not ready yet , although it relies on InputParser and should be easy to implement , there are just things missing still and that's why I postpone implementing it*/
int LoadConfigurationFile(struct AmmServer_Instance * instance,const char * conf_file);

int AssignStr(char ** dest ,const char * source);

/** @brief Set a username and password for clients to access specific webserver instance
    @param An AmmarServer instance
    @param String with new username
    @param String with new password
  * @retval 1=Success,0=Failure*/
int SetUsernameAndPassword(struct AmmServer_Instance * instance,char * username,char * password);


/** @brief Frees a pointer, or cleans it ( sets it to zero ) and then  frees it depending on the configuration of CLEAN_MEMORY_BEFORE_DEALLOCATION
    @param Pointer to free
    @param Size of allocated memory of the pointer*/
void safeFree (void* ptr,size_t size);


#ifdef __cplusplus
}
#endif


#endif // CONFIGURATION_H_INCLUDED
