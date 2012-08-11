#ifndef AMMSERVERLIB_H_INCLUDED
#define AMMSERVERLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

int AmmServer_Start(char * ip,unsigned int port,char * web_root_path,char * templates_root_path);
int AmmServer_Stop();
int AmmServer_Running();

int AmmServer_AddResourceHandler(char * web_root_path,char * resource_name,char * content_memory,unsigned long * content_memory_size,void * prepare_content_callback);


#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
