#ifndef AMMSERVERLIB_H_INCLUDED
#define AMMSERVERLIB_H_INCLUDED


int AmmServer_Start(char * ip,unsigned int port,char * web_root_path,char * templates_root_path);
int AmmServer_Stop();
int AmmServer_Running();

int AmmServer_AddResourceHandler(char * resource_name,char * content_memory,unsigned long * content_memory_size,void * prepare_content_callback);

#endif // AMMSERVERLIB_H_INCLUDED
