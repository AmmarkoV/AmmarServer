#ifndef HTTP_TOOLS_H_INCLUDED
#define HTTP_TOOLS_H_INCLUDED

void error(char * msg);

char FileExists(char * filename);

char DirectoryExists( char* dirpath );

int GetContentType(char * filename,char * content_type);

int FindIndexFile(char * webserver_root,char * directory,char * indexfile);


int ReducePathSlashes_Inplace(char * filename);
int FilenameStripperOk(char * filename);

#endif // HTTP_TOOLS_H_INCLUDED
