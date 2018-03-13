/** @file logs.h
* @brief Logging functions
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef LOGS_H_INCLUDED
#define LOGS_H_INCLUDED

#include <stdio.h>

#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */


#define logEcho() fprintf(stderr," Reached %s , %lu \n ", __FILE__, __LINE__);

///----------------------------------------------------------------------
typedef enum {
               ASV_ERROR_INSTANCE_NOT_ALLOCATED,
               ASV_ERROR_REQUEST_NOT_ALLOCATED,
               ASV_ERROR_BINDING_MASTER_PORT,
               ASV_ERROR_BINDING_ROOTUID_MASTER_PORT,
               ASV_ERROR_FAILED_TO_DROP_ROOT_UID,
               ASV_ERROR_FAILED_TO_CREATE_THREAD,
               ASV_ERROR_FAILED_TO_SOCKET,
               ASV_ERROR_FAILED_TO_BIND,
               ASV_ERROR_FAILED_TO_LISTEN,
               ASV_ERROR_FAILED_TO_ACCEPT,
               ASV_ERROR_FAILED_TO_CONNECT,
               ASV_ERROR_OUT_OF_RESOURCES_TO_ACCOMODATE_CLIENT,
               ASV_ERROR_UNABLE_TO_SERVE_REQUEST,
               ASV_ERROR_UNABLE_TO_SERVE_TEMPLATE,
               ASV_ERROR_FAILED_TO_RECEIVE_HEADER,
               ASV_ERROR_UNAUTHORIZED_REQUEST,
               ASV_ERROR_NO_CALLBACK_REGISTERED,
               ASV_ERROR_CALLBACK_POINTER_CORRUPTION,
               ASV_ERROR_COULD_NOT_SET_SOCKET_TIMEOUT,
               ASV_ERROR_ERROR_SETTING_SOCKET_OPTIONS,
               ASV_ERROR_RUNNING_AS_ROOT,
               ASV_ERROR_CALL_WITHOUT_ENOUGH_INPUT,
               ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY,
               ASV_ERROR_NOT_ENOUGH_MEMORY_ALLOCATED,
               ASV_ERROR_REALLOCATION_R_X86_64_PC32_GCC_ERROR,
               ASV_ERROR_INVALID_RH_SCENARIO,
               ASV_ERROR_CACHE_HAS_NO_PAYLOAD,
               ASV_ERROR_CACHE_NOT_ALLOCATED,
               ASV_ERROR_CACHE_COULD_NOT_CREATE_RESOURCE,
               ASV_ERROR_WRONG_RANGE_REQUEST,
               ASV_ERROR_COULD_NOT_FIND_A_THREADID,
               ASV_ERROR_COULD_NOT_FIND_FILESIZE,
               ASV_ERROR_TIMED_OUT_WHILE_WAITING_FOR_THREAD_TO_BE_CREATED,
               ASV_ERROR_FAILED_TO_CANCEL_THREAD,
               ASV_ERROR_FAILED_TO_PASS_THREAD_CONTEXT,
               ASV_ERROR_INNETOP_FAILED
             } ErrorIDLabels;

extern char *errorIDs[] ;

void errorID(int id);
///----------------------------------------------------------------------
typedef enum {
               ASV_WARNING_DYNAMIC_REQUEST_RETURNED_NOTHING,
               ASV_WARNING_CONNECTION_CLOSED_WHILE_KEEPALIVE,
               ASV_WARNING_CONNECTION_CLOSED_WHILE_TRANSMITTING_FILE,
               ASV_WARNING_CONNECTION_MAX_TRANSMISSION_STALL,
               ASV_WARNING_COULD_NOT_TRANSMIT_ERROR_TO_CLIENT,
               ASV_WARNING_CLIENT_DENIED_ACCESS,
               ASV_WARNING_PRETENDING_IT_IS_A_GET_REQUEST,
               ASV_WARNING_SERVER_STOPPED,
               ASV_WARNING_RESOURCE_HAS_ZERO_ACCOMODATION_SIZE,
               ASV_WARNING_NO_PRESPAWNED_THREADS,
               ASV_WARNING_NOT_CALLING_CALLBACK_WITH_EMPTY_BUFFER,
               ASV_WARNING_RANDOMIZER_IS_NOT_RANDOM,
               ASV_WARNING_CREATING_WORKAROUND_CACHE_ITEM,
               ASV_WARNING_STOP_INITIATED,
               ASV_WARNING_STOP_COMPLETED,
               ASV_WARNING_ENABLING_MONITOR,
               ASV_WARNING_UNRECOGNIZED_REQUST,
               ASV_WARNING_NOTIMPLEMENTED_REQUST,
               ASV_WARNING_PREDATORY_REQUST,
             } WarningIDLabels;

extern char *warningIDs[];

void warningID(int id);
///----------------------------------------------------------------------


int FileAppend(const char * filename,const char * msg);


/**
* @brief Log Function to output Errors
* @ingroup logs
* @param String with message To log*/
void error(const char * msg);

/**
* @brief Log Function to output warnings
* @ingroup logs
* @param String with message To log*/
void warning(const char * msg);


int AccessLogAppend(const char * IP,const  char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent);
int ErrorLogAppend(const char * IP,const char * DateStr,const char * Request,unsigned int ResponseCode,unsigned long ResponseLength,const char * Location,const char * Useragent);

#endif // LOGS_H_INCLUDED
