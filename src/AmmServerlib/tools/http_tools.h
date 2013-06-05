#ifndef HTTP_TOOLS_H_INCLUDED
#define HTTP_TOOLS_H_INCLUDED


#include "../AmmServerlib.h"
#include "../cache/client_list.h"

typedef unsigned int contentType;

enum contentTypeEnumerator
{
    NO_FILETYPE=0,
    RESERVED, //This to prevent some functions that return 1 get used the wrong way :P
    TEXT,
    IMAGE,
    AUDIO,
    VIDEO,
    EXECUTABLE,
    FOLDER
};

unsigned int ServerThreads_DropRootUID();

char FileExistsAmmServ(char * filename);
char DirectoryExistsAmmServ( char* dirpath );

int GetExtentionType(char * theextension);
int GetContentType(char * filename,char * content_type);
int GetExtensionImage(char * filename, char * theimagepath,unsigned int theimagepath_length);

int FindIndexFile(struct AmmServer_Instance * instance,char * webserver_root,char * directory,char * indexfile);

int StripGETRequestQueryAndFragment(char * filename , char * query , unsigned int max_query_length);
int StripVariableFromGETorPOSTString(char * input,char * var_id, char * var_val , unsigned int var_val_length);

int GetDateString(char * output,char * label,unsigned int now,unsigned int dayofweek,unsigned int day,unsigned int month,unsigned int year,unsigned int hour,unsigned int minute,unsigned int second);

int CheckHTTPHeaderCategory(char * line,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start);
int trim_last_empty_chars(char * input,unsigned int input_length);
int seek_non_blank_char(char * input,char * input_end);
int seek_blank_char(char * input,char * input_end);
unsigned int GetIntFromHTTPHeaderFieldPayload(char * request,unsigned int request_length);
char * GetNewStringFromHTTPHeaderFieldPayload(char * request,unsigned int request_length);

int encodeToBase64(char *src,unsigned s_len,char *dst,unsigned d_len);

int StripHTMLCharacters_Inplace(char * filename,int enable_security);
int ReducePathSlashes_Inplace(char * filename);
int FilenameStripperOk(char * filename);

char * RequestHTTPWebPage(char * hostname,unsigned int port,char * filename,unsigned int max_content);

int freeString(char ** str);

int setSocketTimeouts(int clientSock);
clientID findOutClientIDOfPeer(struct AmmServer_Instance * instance , int clientSock);

#endif // HTTP_TOOLS_H_INCLUDED
