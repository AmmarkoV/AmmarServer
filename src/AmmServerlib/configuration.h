#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED

#define MAX_CLIENTS 255
#define MAX_CLIENTS_PER_IP 3 //<- Not implemented yet

#define MAX_RESOURCE 1024
#define MAX_FILE_PATH 1024

extern int varSocketTimeoutREAD_ms;
extern int varSocketTimeoutWRITE_ms;

extern int AccessLogEnable;
extern char AccessLog[MAX_FILE_PATH];

extern int ErrorLogEnable;
extern char ErrorLog[MAX_FILE_PATH];

#endif // CONFIGURATION_H_INCLUDED
