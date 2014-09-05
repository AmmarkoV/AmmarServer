/** @file http_tools.h
* @brief A collection of tools required by the server and gathered here since they do a very specific job
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef HTTP_TOOLS_H_INCLUDED
#define HTTP_TOOLS_H_INCLUDED


#include "../AmmServerlib.h"
#include "../cache/client_list.h"

typedef unsigned int contentType;

enum contentTypeEnumerator
{
    NO_FILETYPE=0,
    RESERVED_CTE_VALUE, //This to prevent some functions that return 1 get used the wrong way :P
    TEXT,
    IMAGE,
    AUDIO,
    VIDEO,
    EXECUTABLE,
    FOLDER
};

/**
* @brief Drop Root UID , if we have one ( and according to server_configuration.h )
* @ingroup tools
*/
unsigned int ServerThreads_DropRootUID();

/**
* @brief Check if file Exists
* @ingroup tools
* @param Path to file
* @retval 1=Exists,0=Does not Exist*/
char FileExistsAmmServ(const char * filename);

/**
* @brief Check if directory Exists
* @ingroup tools
* @param Path to directory
* @retval 1=Exists,0=Does not Exist*/
char DirectoryExistsAmmServ(const char* dirpath );

/**
* @brief Convert an Extension Type to a contentTypeEnumerator
* @ingroup tools
* @param String with the extension type
* @retval contentTypeEnumerator*/
int GetExtentionType(const char * theextension);

/**
* @brief Convert a filename to a contentType
* @ingroup tools
* @param String with the filename we want to examine
* @param Output String with the contentType
* @param Output contentType length
* @retval contentTypeEnumerator*/
int GetContentType(char * filename,char * contentType,unsigned int contentTypeLength);

/**
* @brief Return template image for specific content type ( for directory listings etc )
* @ingroup tools
* @param Filename of file
* @param Path to Image
* @param Length of path to Image
* @retval 1=Exists,0=Does not Exist*/
int GetExtensionImage(char * filename, char * theimagepath,unsigned int theimagepath_length);

int FindIndexFile(struct AmmServer_Instance * instance,char * webserver_root,char * directory,char * indexfile,unsigned int indexFileLength);

int StripGETRequestQueryAndFragment(char * filename , char * query , unsigned int max_query_length);
int StripVariableFromGETorPOSTString(const char * input,const char * var_id, char * var_val , unsigned int var_val_length);


int strToUpcase(char * strTarget , char * strSource , unsigned int strLength);

int CheckHTTPHeaderCategoryAllCaps(char * lineCAPS,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start);
int CheckHTTPHeaderCategory(char * line,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start);
int trim_last_empty_chars(char * input,unsigned int input_length);
int seek_non_blank_char(char * input,char * input_end);
int seek_blank_char(char * input,char * input_end);
unsigned int GetIntFromHTTPHeaderFieldPayload(char * request,unsigned int request_length);
char * GetNewStringFromHTTPHeaderFieldPayload(char * request,unsigned int request_length);


/**
* @brief Convert a string to base64 , required for the authorization tokens
* @ingroup tools
* @param Input string
* @param Input string length
* @param Output string
* @param Input Maximum output length
* @retval 1=Success,0=Failure*/
int encodeToBase64(char *src,unsigned s_len,char *dst,unsigned d_len);

/**
* @brief HTML characters should be converted to plain c byte chars after we get them , this poses some security threats since this might allow "weird" bytes to get set that
         in conjunction with an overflow somewhere else might trick the server into executing
* @ingroup tools,security
* @param Input string
* @param Enforce security that filters out possibly unwanted bytes..!! , bytes larger than 255 are always filtered since the sec_byte
          can be also triggered by %ZZ or any ascii value out of 0-F for ( see code ) ..!
* @retval 1=Success,0=Failure*/
int StripHTMLCharacters_Inplace(char * filename,int enable_security);

/**
* @brief Filenames may contain ///// with an arbitrary number of slashes , we convert them to a single slash
* @ingroup tools,security
* @param Input string
* @retval 1=Success,0=Failure*/
int ReducePathSlashes_Inplace(char * filename);


/**
* @brief Strip filename and security check it
* @ingroup security
* @param Pointer to string pointer to be analyzed
* @retval 1=Ok,0=Failed*/
int FilenameStripperOk(char * filename);

/**
* @brief A very basic http client for testing connections and maybe in the future make AmmarServers communicate with each other
* @ingroup tools
* @param Hostname to connect to
* @param Port to connect to
* @param Filename to download
* @param Maximum size of response to carry
* @retval Pointer to requested page,0=Failure*/
char * RequestHTTPWebPage(char * hostname,unsigned int port,char * filename,unsigned int max_content);

/**
* @brief Free C string and set it to 0
* @ingroup tools
* @param Pointer to string pointer to be freed
* @retval 1=Success,0=Failure*/
int freeString(char ** str);

/**
* @brief Enforce socket timeouts declared in server_configuration.h and configuration files to socket
* @ingroup tools
* @param Socket to change
* @retval 1=Success,0=Failure*/
int setSocketTimeouts(int clientSock);

/**
* @brief Tool that resolve a client socket to its IP , then uses it to try to clientList_GetClientId and returns the id number
* @ingroup tools
* @param An AmmarServer instance
* @param client socket
* @retval ClientID or ,0=Failure*/
clientID findOutClientIDOfPeer(struct AmmServer_Instance * instance , int clientSock);

#endif // HTTP_TOOLS_H_INCLUDED
