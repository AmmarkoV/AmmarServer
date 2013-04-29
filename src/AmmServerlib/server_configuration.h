#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED

#include "AmmServerlib.h"

//Prespawned threads sleep until its their time to serve , the next thread to serve is vigilant ( 0.7ms max delay )
#define THREAD_SLEEP_TIME_WHEN_OUR_PRESPAWNED_THREAD_IS_NEXT 700
//Other prespawned threads sleep for 20ms
#define THREAD_SLEEP_TIME_FOR_PRESPAWNED_THREADS 25000


#define COMPILE_WITH_CLIENT_LIST 1


extern unsigned int GLOBAL_KILL_SERVER_SWITCH;

#define MAX_CLIENT_THREADS 512 //This is the maximum number of simultaneous regular threads that serve incoming requests..!
#define MAX_CLIENTS_LISTENING_FOR 1000 //This should always be larger than MAX_CLIENT_THREADS
#define MAX_CLIENT_PRESPAWNED_THREADS 16 //16 //8 //This is the number of prespawned threads that run to reduce overall latency
#define MAX_CLIENTS_PER_IP 3 //<- Not implemented yet

#define MAX_RESOURCE_SLASHES 15
#define MAX_CONFIGURATION_FILE_LINE_SIZE 512
#define MAX_FILE_PATH_EXTENSION_SIZE 128

#define MAX_CONTENT_TYPE 128
#define MAX_FILE_READ_BLOCK_KB 1024 //1MB - How much KB is the max fread , malloc for serving files
#define MAX_HTTP_REQUEST_HEADER 4096
#define MAX_HTTP_REQUEST_HEADER_LINE 1024
#define MAX_HTTP_RESPONSE_HEADER 2048
#define MAX_DIRECTORY_LIST_RESPONSE_BODY 4096

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

#define ENABLE_SEPERATE_MALLOC_FOR_CHANGING_DYNAMIC_PAGES 0

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

int EmmitPossibleConfigurationWarnings(struct AmmServer_Instance * instance);

int LoadConfigurationFile(struct AmmServer_Instance * instance,char * conf_file);

int AssignStr(char ** dest , char * source);
int SetUsernameAndPassword(struct AmmServer_Instance * instance,char * username,char * password);

#endif // CONFIGURATION_H_INCLUDED
