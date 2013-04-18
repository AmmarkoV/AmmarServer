#ifndef HTTP_TOOLS_H_INCLUDED
#define HTTP_TOOLS_H_INCLUDED


#include "../AmmServerlib.h"



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

enum FileType
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

void error(char * msg);
void warning(char * msg);

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

#endif // HTTP_TOOLS_H_INCLUDED
