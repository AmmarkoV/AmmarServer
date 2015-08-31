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


#define WORKAROUND_REALLOCATION_R_X86_64_PC32_GCC_ERROR 1




/** @brief Time sleeping when a dynamic request that serves a common file across all clients is busy   */
#define CLIENT_SLEEP_TIME_WHEN_DYNAMIC_REQUEST_CALLBACK_IS_BUSY_NSEC 1500000

/** @brief Time sleeping when a dynamic request that serves a common file across all clients is busy   */
#define CLIENT_SLEEP_TIME_INTERVAL_NSEC 10000


/** @brief Next prespawned thread , should be vigilant and ready to serve so it has a shorter delay than the other prespawned threads ( 0.7ms max delay seems like a good value ) */
#define THREAD_SLEEP_TIME_WHEN_OUR_PRESPAWNED_THREAD_IS_NEXT 700



/** @brief Max sleep time while waiting for new thread to kick in and read parameters to unblock main thread.. */
#define THREAD_MAXIMUM_TIME_TO_WAIT_FOR_A_NEWLY_CREATED_THREAD_MS 250000

/** @brief Sleep time while waiting for new thread to kick in and read parameters to unblock main thread.. */
#define THREAD_SLEEP_TIME_WHILE_WAITING_FOR_NEW_CREATED_THREAD_TO_CONSUME_PARAMETERS 20




/** @brief Sleep time for threads that are prespawned until they check for potential new work , the lowest the value here ,
           the shortest the wait time for clients , but this causes higher CPU usage ( for idle tasks ) and ultimately more power consumption
           A good default time is 25000 , ( 25ms ) */
#define THREAD_SLEEP_TIME_FOR_PRESPAWNED_THREADS 25000

/** @brief Calculate (And output) transmission speed for files broadcast by AmmarServer  */
#define CALCULATE_TIME_FOR_UPLOADS 1

/** @brief Precompiler switch that controls baking in ( or not ) the client list capabilities , currently disabled since client lists are not yet implemented */
#define COMPILE_WITH_CLIENT_LIST 0


/** @brief Sleep time after unsuccessfully trying to bind to port ( usleep(DELAY... ) */
#define DELAY_TRY_BINDING_TO_PORT 5000 *1000

/** @brief Maximum times to try to bind to port on initial server start up */
#define MAX_TRIES_TO_BIND_TO_PORT 5

/** @brief Setting this to 1 will signal that all instances of AmmarServer need to die at once */
extern unsigned int GLOBAL_KILL_SERVER_SWITCH;

/** @brief Maximum Target of concurrent clients being listened at the same time C10K tests require this to be 10000 ( http://en.wikipedia.org/wiki/C10k_problem ) */
#define MAX_CLIENTS_LISTENING_FOR 5000 //C10K :P

/** @brief Maximum Number of concurrent threads being created at the same time , depending on the size of the listen pool this can be smaller than the MAX_CLIENTS_LISTENING_FOR and connections will
           be queued and served sequentially */
#define MAX_CLIENT_THREADS 3000 //This is the maximum number of simultaneous regular threads that serve incoming requests..!

/** @brief Prespawned theads reduce overall latency but they increase CPU load  , 0 disables them */
#define MAX_CLIENT_PRESPAWNED_THREADS 0 //<- Disabled for now This is the number of prespawned threads that run to reduce overall latency
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
#define MAX_HTTP_REQUEST_HEADER 4096

/** @brief Maximum size of an incoming HTTP Header allocation step */
#define HTTP_POST_GROWTH_STEP_REQUEST_HEADER 512/*KB*/*1024

/** @brief Maximum size of an incoming POST Header , since it carries files this should be big enough ( say 4 MB )  */
#define MAX_HTTP_POST_REQUEST_HEADER 4/*MB*/*1024*1024



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

/** @brief Enable POST request handling , switching this to 0 will completely deny them reducing attack surface */
#define ENABLE_POST 1
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

/**  @brief Default timeout value before which a socket blocking on a read call should be considered dead */
#define DEFAULT_SOCKET_READ_TIMEOUT_SECS 5
extern int varSocketTimeoutREAD_seconds;

/**  @brief Default timeout value before which a socket blocking on a write call should be considered dead */
#define DEFAULT_SOCKET_WRITE_TIMEOUT_SECS 5
extern int varSocketTimeoutWRITE_seconds;

// ----------------- CACHE OPTIONS -----------------

/**  @brief If caching is disabled server becomes a very simple file server , dynamic requests are also disabled*/
extern unsigned char CACHING_ENABLED;


/** @brief Maximum Number of separate items in cache ( per instance of AmmarServer )*/
extern int MAX_SEPERATE_CACHE_ITEMS;
/** @brief Maximum memory usage ( Megabytes ) for the entire cache ( per instance of AmmarServer )  */
extern int MAX_CACHE_SIZE_IN_MB;
/** @brief Maximum memory usage ( Megabytes ) for a specific entry of the cache ( per instance of AmmarServer ) */
extern int MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB;


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


#ifdef __cplusplus
}
#endif


#endif // CONFIGURATION_H_INCLUDED
