#ifndef HTTP_TOOLS_H_INCLUDED
#define HTTP_TOOLS_H_INCLUDED

void error(char * msg);

char FileExists(char * filename);

char DirectoryExists( char* dirpath );

int GetContentType(char * filename,char * content_type);
int GetExtensionImage(char * filename, char * theimagepath,unsigned int theimagepath_length);

int FindIndexFile(char * webserver_root,char * directory,char * indexfile);

int StripGETRequestQueryAndFragment(char * filename , char * query , unsigned int max_query_length);

int CheckHTTPHeaderCategory(char * line,unsigned int line_length,char * potential_strCAPS,unsigned int * payload_start);
int trim_last_empty_chars(char * input,unsigned int input_length);

int StripHTMLCharacters_Inplace(char * filename);
int ReducePathSlashes_Inplace(char * filename);
int FilenameStripperOk(char * filename);

#endif // HTTP_TOOLS_H_INCLUDED
