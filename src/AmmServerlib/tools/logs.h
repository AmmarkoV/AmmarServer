#ifndef LOGS_H_INCLUDED
#define LOGS_H_INCLUDED


int AccessLogAppend(char * IP,char * DateStr,char * Request,unsigned int ResponseCode,unsigned long ResponseLength,char * Location,char * Useragent);
int ErrorLogAppend(char * IP,char * DateStr,char * Request,unsigned int ResponseCode,unsigned long ResponseLength,char * Location,char * Useragent);

#endif // LOGS_H_INCLUDED
