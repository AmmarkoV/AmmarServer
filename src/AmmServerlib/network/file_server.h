#ifndef FILE_SERVER_H_INCLUDED
#define FILE_SERVER_H_INCLUDED

#include "../header_analysis/http_header_analysis.h"


struct ResponseHeader
{
   int responseCode;
   char URI[128];
   char dateString[64];
};

unsigned long SendErrorCodeHeader(int clientsock,unsigned int error_code,char * verified_filename,char * templates_root);
unsigned long SendSuccessCodeHeader(int clientsock,int success_code,char * verified_filename);
unsigned long SendAuthorizationHeader(int clientsock,char * message,char * verified_filename);

unsigned long SendFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    char * verified_filename_pending_copy, // The filename to be served on the socket above
    unsigned int force_error_code
  );


unsigned long SendErrorFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    unsigned int errorCode
  );


unsigned long SendMemoryBlockAsFile
  (
    int clientsock, // The socket that will be used to send the data
    //char * path, // The filename to be served on the socket above
    char * mem, // The filename to be served on the socket above
    unsigned long mem_block
  );

#endif // FILE_SERVER_H_INCLUDED
