#ifndef AMMSERVERLIB_H_INCLUDED
#define AMMSERVERLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


struct AmmServer_RH_Context
{
   unsigned long content_memory_size;
   unsigned long GET_request_size;
   unsigned long POST_request_size;

   void * prepare_content_callback;

   char * web_root_path;
   char * resource_name;

   char * content_memory;
   char * GET_request;
   char * POST_request;
};

int AmmServer_Start(char * ip,unsigned int port,char * web_root_path,char * templates_root_path);
int AmmServer_Stop();
int AmmServer_Running();

int AmmServer_AddResourceHandlerOLD(char * web_root_path,char * resource_name,char * content_memory,unsigned long * content_memory_size,void * prepare_content_callback);
int AmmServer_AddResourceHandler(struct AmmServer_RH_Context * context);


#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
