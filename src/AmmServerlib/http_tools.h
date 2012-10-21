#ifndef HTTP_TOOLS_H_INCLUDED
#define HTTP_TOOLS_H_INCLUDED

void error(char * msg);

char FileExistsAmmServ(char * filename);
char DirectoryExistsAmmServ( char* dirpath );

int GetContentType(char * filename,char * content_type);
int GetExtensionImage(char * filename, char * theimagepath,unsigned int theimagepath_length);

int FindIndexFile(char * webserver_root,char * directory,char * indexfile);

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

int StripHTMLCharacters_Inplace(char * filename);
int ReducePathSlashes_Inplace(char * filename);
int FilenameStripperOk(char * filename);

char * RequestHTTPWebPage(char * hostname,unsigned int port,char * filename,unsigned int max_content);

#endif // HTTP_TOOLS_H_INCLUDED
