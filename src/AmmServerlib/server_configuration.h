/** @file server_configuration.h
* @brief The Main Header for the settings used by AmmarServer
*
* Take extra care when changing something here , since its impact is global
*
* @author Ammar Qammaz (AmmarkoV)
* @bug AmmarServer is not properly pentested yet
*/
#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED

#include "AmmServerlib.h"


/** @brief Next prespawned thread , should be vigilant and ready to serve so it has a shorter delay than the other prespawned threads ( 0.7ms max delay seems like a good value ) */
#define THREAD_SLEEP_TIME_WHEN_OUR_PRESPAWNED_THREAD_IS_NEXT 700

/** @brief Sleep time for threads that are prespawned until they check for potential new work , the lowest the value here ,
           the shortest the wait time for clients , but this causes higher CPU usage ( for idle tasks ) and ultimately more power consumption
           A good default time is 25000 , ( 25ms ) */
#define THREAD_SLEEP_TIME_FOR_PRESPAWNED_THREADS 25000

/** @brief Calculate (And output) transmission speed for files broadcast by AmmarServer  */
#define CALCULATE_TIME_FOR_UPLOADS 1

/** @brief Precompiler switch that controls baking in ( or not ) the client list capabilities , currently disabled since client lists are not yet implemented */
#define COMPILE_WITH_CLIENT_LIST 0


/** @brief Setting this to 1 will signal that all instances of AmmarServer need to die */
extern unsigned int GLOBAL_KILL_SERVER_SWITCH;

/** @brief Maximum Target of concurrent clients being listened at the same time C10K tests require this to be 10000 ( http://en.wikipedia.org/wiki/C10k_problem ) */
#define MAX_CLIENTS_LISTENING_FOR 5000 //C10K :P

/** @brief Maximum Number of concurrent threads being created at the same time , depending on the size of the listen pool this can be smaller than the MAX_CLIENTS_LISTENING_FOR and connections will
           be queued and served sequentially */
#define MAX_CLIENT_THREADS 3000 //This is the maximum number of simultaneous regular threads that serve incoming requests..!

/** @brief Prespawned theads reduce overall latency but they increase CPU load  */
#define MAX_CLIENT_PRESPAWNED_THREADS 0 //<- Disabled for now This is the number of prespawned threads that run to reduce overall latency
#define MAX_CLIENTS_PER_IP 3 //<- Not implemented yet

#define MAX_RESOURCE_SLASHES 15
#define MAX_CONFIGURATION_FILE_LINE_SIZE 512
#define MAX_FILE_PATH_EXTENSION_SIZE 128

#define MAX_CONTENT_TYPE 128
#define MAX_FILE_READ_BLOCK_KB 1024 //1MB - How much KB is the max fread , malloc for serving files
#define MAX_HTTP_REQUEST_HEADER 4096

#define HTTP_POST_GROWTH_STEP_REQUEST_HEADER 512/*KB*/*1024
#define MAX_HTTP_POST_REQUEST_HEADER 4/*MB*/*1024*1024

#define MAX_HTTP_REQUEST_REPLY 512
#define MAX_HTTP_REQUEST_HEADER_LINES 1024 // A Header must not contain more than 1024 lines of directives
#define MAX_HTTP_RESPONSE_HEADER 2048

#define INITIAL_DIRECTORY_LIST_RESPONSE_BODY 56000
#define GROWSTEP_DIRECTORY_LIST_RESPONSE_BODY 56000
#define MAX_DIRECTORY_LIST_RESPONSE_BODY 1256000

#define REALLOC_TO_SAVE_MORE_THAN_THIS_NUMBER_BYTES 4096 //This may happen during compression in file_caching.c

#define ENABLE_AUTOMATIC_CONFIGURATION_LOADING 1

#define ENABLE_POST 1
#define ENABLE_COMPRESSION 0 //Compression doesn't work all that well yet
#define ENABLE_DYNAMIC_CONTENT_COMPRESSION 0 // Compression for dynamic content ( doesnt seem like a very good idea unless you have a dynamic html file of 20KB +
#define ENABLE_INTERFACE_ACCESS_TO_GET_POST_VARIABLES 1

#define ENABLE_DROPPING_ROOT_UID_IF_ROOT 1
#define ENABLE_DROPPING_UID_ALWAYS 0

extern char USERNAME_UID_FOR_DAEMON[MAX_FILE_PATH]; //Check this value on configuration.c if you want to set the daemon running as a specific user i.e. www-data
extern int CHANGE_TO_UID; //This is the default UID to use when USERNAME_UID_FOR_DAEMON doesn't provide a useful UID..

#define ENABLE_INTERNAL_RESOURCES_RESOLVE 1
#define ENABLE_DIRECTORY_LISTING 1

#define EPOCH_YEAR_IN_TM_YEAR 1900

extern int CHANGE_PRIORITY;

extern int varSocketTimeoutREAD_seconds;
extern int varSocketTimeoutWRITE_seconds;

// ----------------- CACHE OPTIONS -----------------
extern unsigned char CACHING_ENABLED;
extern unsigned char DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED;
extern int MAX_SEPERATE_CACHE_ITEMS;
extern int MAX_CACHE_SIZE_IN_MB;
extern int MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB;


extern int  AccessLogEnable;
extern char AccessLog[MAX_FILE_PATH];

extern int  ErrorLogEnable;
extern char ErrorLog[MAX_FILE_PATH];


extern char TemplatesInternalURI[MAX_RESOURCE];
//Please note that the file server limits filenames _asvres_/filename.jpg is OK
//a filename like _asvres_/filenamemplampla.jpg will return a 404


int instance_WeCanCommitMoreMemory(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes);
int instance_CountNewMallocOP(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes);
int instance_CountFreeOP(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes);


int EmmitPossibleConfigurationWarnings();

int LoadConfigurationFile(struct AmmServer_Instance * instance,char * conf_file);

int AssignStr(char ** dest , char * source);
int SetUsernameAndPassword(struct AmmServer_Instance * instance,char * username,char * password);

#endif // CONFIGURATION_H_INCLUDED
