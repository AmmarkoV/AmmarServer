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
               /* 0*/  ASV_ERROR_FAILED_TO_RESOLVE_GET_QUERY,
               /* 1*/  ASV_ERROR_INSTANCE_NOT_ALLOCATED,
               /* 2*/  ASV_ERROR_REQUEST_NOT_ALLOCATED,
               /* 3*/  ASV_ERROR_BINDING_MASTER_PORT,
               /* 4*/  ASV_ERROR_BINDING_ROOTUID_MASTER_PORT,
               /* 5*/  ASV_ERROR_FAILED_TO_DROP_ROOT_UID,
               /* 6*/  ASV_ERROR_FAILED_TO_CREATE_THREAD,
               /* 7*/  ASV_ERROR_FAILED_TO_SOCKET,
               /* 8*/  ASV_ERROR_FAILED_TO_BIND,
               /* 9*/  ASV_ERROR_FAILED_TO_LISTEN,
               /*10*/  ASV_ERROR_FAILED_TO_ACCEPT,
               /*11*/  ASV_ERROR_FAILED_TO_CONNECT,
               /*12*/  ASV_ERROR_OUT_OF_RESOURCES_TO_ACCOMODATE_CLIENT,
               /*13*/  ASV_ERROR_UNABLE_TO_SERVE_REQUEST,
               /*14*/  ASV_ERROR_UNABLE_TO_SERVE_TEMPLATE,
               /*15*/  ASV_ERROR_FAILED_TO_RECEIVE_HEADER,
               /*16*/  ASV_ERROR_UNAUTHORIZED_REQUEST,
               /*17*/  ASV_ERROR_NO_CALLBACK_REGISTERED,
               /*18*/  ASV_ERROR_CALLBACK_POINTER_CORRUPTION,
               /*19*/  ASV_ERROR_COULD_NOT_SET_SOCKET_TIMEOUT,
               /*20*/  ASV_ERROR_ERROR_SETTING_SOCKET_OPTIONS,
               /*21*/  ASV_ERROR_RUNNING_AS_ROOT,
               /*22*/  ASV_ERROR_CALL_WITHOUT_ENOUGH_INPUT,
               /*23*/  ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY,
               /*24*/  ASV_ERROR_NOT_ENOUGH_MEMORY_ALLOCATED,
               /*25*/  ASV_ERROR_REALLOCATION_R_X86_64_PC32_GCC_ERROR,
               /*26*/  ASV_ERROR_INVALID_RH_SCENARIO,
               /*27*/  ASV_ERROR_CACHE_HAS_NO_PAYLOAD,
               /*28*/  ASV_ERROR_CACHE_NOT_ALLOCATED,
               /*29*/  ASV_ERROR_CACHE_COULD_NOT_CREATE_RESOURCE,
               /*30*/  ASV_ERROR_WRONG_RANGE_REQUEST,
               /*31*/  ASV_ERROR_COULD_NOT_FIND_A_THREADID,
               /*32*/  ASV_ERROR_COULD_NOT_FIND_FILESIZE,
               /*33*/  ASV_ERROR_TIMED_OUT_WHILE_WAITING_FOR_THREAD_TO_BE_CREATED,
               /*34*/  ASV_ERROR_FAILED_TO_CANCEL_THREAD,
               /*35*/  ASV_ERROR_FAILED_TO_PASS_THREAD_CONTEXT,
               /*36*/  ASV_ERROR_INNETOP_FAILED,
               /*37*/  ASV_ERROR_REDUCE_SLASHSES_FAILED,
               /*38*/  ASV_ERROR_FAILED_TO_ADD_NOCACHE_RULE,
               /*39*/  ASV_ERROR_CLIENT_CAUSED_AN_OVERFLOW
             } ErrorIDLabels;

extern char *errorIDs[] ;

void errorID(int id);
///----------------------------------------------------------------------
typedef enum {
               /* 0*/  ASV_WARNING_DYNAMIC_REQUEST_RETURNED_NOTHING,
               /* 1*/  ASV_WARNING_CONNECTION_CLOSED_WHILE_KEEPALIVE,
               /* 2*/  ASV_WARNING_CONNECTION_CLOSED_WHILE_TRANSMITTING_FILE,
               /* 3*/  ASV_WARNING_CONNECTION_MAX_TRANSMISSION_STALL,
               /* 4*/  ASV_WARNING_COULD_NOT_TRANSMIT_ERROR_TO_CLIENT,
               /* 5*/  ASV_WARNING_CLIENT_DENIED_ACCESS,
               /* 6*/  ASV_WARNING_PRETENDING_IT_IS_A_GET_REQUEST,
               /* 7*/  ASV_WARNING_SERVER_STOPPED,
               /* 8*/  ASV_WARNING_RESOURCE_HAS_ZERO_ACCOMODATION_SIZE,
               /* 9*/  ASV_WARNING_NO_PRESPAWNED_THREADS,
               /*10*/  ASV_WARNING_NOT_CALLING_CALLBACK_WITH_EMPTY_BUFFER,
               /*11*/  ASV_WARNING_RANDOMIZER_IS_NOT_RANDOM,
               /*12*/  ASV_WARNING_CREATING_WORKAROUND_CACHE_ITEM,
               /*13*/  ASV_WARNING_STOP_INITIATED,
               /*14*/  ASV_WARNING_STOP_COMPLETED,
               /*15*/  ASV_WARNING_ENABLING_MONITOR,
               /*16*/  ASV_WARNING_UNRECOGNIZED_REQUEST,
               /*17*/  ASV_WARNING_NOTIMPLEMENTED_REQUEST,
               /*18*/  ASV_WARNING_PREDATORY_REQUST,
               /*19*/  ASV_WARNING_WRONG_ARGUMENT
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
