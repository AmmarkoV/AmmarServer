#ifndef HTTPRULES_H_INCLUDED
#define HTTPRULES_H_INCLUDED

void error(char * msg);
char FileExists(char * filename);
char DirectoryExists( char* dirpath );

int GetContentType(char * filename,char * content_type);

int FindIndexFile(char * webserver_root,char * directory,char * indexfile);

int FilenameStripperOk(char * filename);

#endif // HTTPRULES_H_INCLUDED
