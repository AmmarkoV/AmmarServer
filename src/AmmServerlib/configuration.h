#ifndef CONFIGURATION_H_INCLUDED
#define CONFIGURATION_H_INCLUDED

#define MAX_CLIENT_THREADS 255
#define MAX_CLIENTS_PER_IP 3 //<- Not implemented yet

#define MAX_RESOURCE 512
#define MAX_RESOURCE_SLASHES 15
#define MAX_FILE_PATH 1024
#define MAX_FILE_PATH_EXTENSION_SIZE 128

#define MAX_CONTENT_TYPE 128
#define MAX_FILE_READ_BLOCK_KB 1024 //1MB - How much KB is the max fread , malloc for serving files
#define MAX_HTTP_REQUEST_HEADER 4096
#define MAX_HTTP_REQUEST_HEADER_LINE 1024
#define MAX_HTTP_RESPONSE_HEADER 2048
#define MAX_DIRECTORY_LIST_RESPONSE_BODY 4096

#define ENABLE_INTERNAL_RESOURCES_RESOLVE 1
#define ENABLE_DIRECTORY_LISTING 1

extern int varSocketTimeoutREAD_ms;
extern int varSocketTimeoutWRITE_ms;

extern int AccessLogEnable;
extern char AccessLog[MAX_FILE_PATH];

extern int ErrorLogEnable;
extern char ErrorLog[MAX_FILE_PATH];


extern char TemplatesInternalURI[MAX_RESOURCE];
//Please note that the file server limits filenames _asvres_/filename.jpg is OK
//a filename like _asvres_/filenamemplampla.jpg will return a 404

#endif // CONFIGURATION_H_INCLUDED
