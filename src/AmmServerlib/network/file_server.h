/** @file file_server.h
* @brief Basic file server functionality of AmmarServer
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef FILE_SERVER_H_INCLUDED
#define FILE_SERVER_H_INCLUDED

#include "../header_analysis/http_header_analysis.h"

/**
* @brief Send a File to a client
* @ingroup network
* @param An AmmarServer Instance
* @param HTTPTransaction this send file is part of
* @param Filename that has been verified but has not been copied to the http checked for safety
* @param Force SendFile to fail with a specific error code ( 0 = dont force error )
* @retval 1=Success,0=Failure*/
unsigned long SendFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    char * verified_filename_pending_copy, // The filename to be served on the socket above
    unsigned int force_error_code
  );

/**
* @brief Send an Error "file" response to a client , this is just a wrapper for a SendFile call with a force_error_code set
* @ingroup network
* @param An AmmarServer Instance
* @param HTTPTransaction this send file is part of
* @param Error Code to send
* @retval 1=Success,0=Failure*/
unsigned long SendErrorFile
  (
    struct AmmServer_Instance * instance,
    struct HTTPTransaction * transaction,
    unsigned int errorCode
  );

/**
* @brief Send a memory block to a client as a file
* @ingroup network
* @param An AmmarServer Instance
* @param Filename to pretend that we are sending for
* @param Socket we want to write to
* @param Pointer to memory that holds what we want to send to the client
* @param Length of memory block we want to send
* @retval 1=Success,0=Failure*/
unsigned long SendMemoryBlockAsFile
  (
    struct AmmServer_Instance * instance,
    char * filename,
    int clientsock, // The socket that will be used to send the data
    //char * path, // The filename to be served on the socket above
    char * mem, // The filename to be served on the socket above
    unsigned long mem_block
  );


#endif // FILE_SERVER_H_INCLUDED
