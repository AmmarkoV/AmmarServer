#ifndef AMMSERVERLIB_H_INCLUDED
#define AMMSERVERLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "configuration.h"

struct AmmServer_RH_Context
{
   unsigned long MAX_content_size;
   unsigned long content_size;
   unsigned long GET_request_size;
   unsigned long POST_request_size;

   void * prepare_content_callback;

   char web_root_path[MAX_FILE_PATH];
   char resource_name[MAX_RESOURCE];

   char * content;
   char * GET_request;
   char * POST_request;
};

enum AmmServInfos
{
    AMMINF_ACTIVE_CLIENTS=0,
    AMMINF_ACTIVE_THREADS
};

int AmmServer_Start(char * ip,unsigned int port,char * web_root_path,char * templates_root_path);
int AmmServer_Stop();
int AmmServer_Running();

int AmmServer_AddResourceHandler(struct AmmServer_RH_Context * context);

int AmmServer_GetInfo(unsigned int info_type);

int AmmServer_GetIntSettingValue(unsigned int set_type);
int AmmServer_SetIntSettingValue(unsigned int set_type);

int AmmServer_SelfCheck();

#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
