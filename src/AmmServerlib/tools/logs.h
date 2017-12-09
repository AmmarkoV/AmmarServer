/** @file logs.h
* @brief Logging functions
* @author Ammar Qammaz (AmmarkoV)
*/
#ifndef LOGS_H_INCLUDED
#define LOGS_H_INCLUDED

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
