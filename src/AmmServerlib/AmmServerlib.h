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

   unsigned int last_callback;
   unsigned int callback_every_x_msec;
   char callback_cooldown;
   void * prepare_content_callback;

   char web_root_path[MAX_FILE_PATH];
   char resource_name[MAX_RESOURCE];

   char * content;

   char * GET_request;
   unsigned int GET_request_length;

   char * POST_request;
   unsigned int POST_request_length;
};

enum AmmServInfos
{
    AMMINF_ACTIVE_CLIENTS=0,
    AMMINF_ACTIVE_THREADS
};


enum AmmServSettings
{
    AMMSET_PASSWORD_PROTECTION=0,
    AMMSET_TEST
};


enum AmmServStrSettings
{
    AMMSET_USERNAME_STR=0,
    AMMSET_PASSWORD_STR,
    AMMSET_TESTSTR
};


int AmmServer_Start(char * ip,unsigned int port,char * conf_file,char * web_root_path,char * templates_root_path);
int AmmServer_Stop();
int AmmServer_Running();

int AmmServer_AddResourceHandler(struct AmmServer_RH_Context * context, char * resource_name , char * web_root, unsigned int allocate_mem_bytes,unsigned int callback_every_x_msec,void * callback);
int AmmServer_RemoveResourceHandler(struct AmmServer_RH_Context * context,unsigned char free_mem);

int AmmServer_GetInfo(unsigned int info_type);

int AmmServer_GetIntSettingValue(unsigned int set_type);
int AmmServer_SetIntSettingValue(unsigned int set_type,int set_value);

int AmmServer_POSTArg(struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);
int AmmServer_GETArg(struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);
int AmmServer_FILES(struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);

int _POST(struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);
int _GET(struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);
int _FILES(struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);

int AmmServer_DoNOTCacheResource(char * resource_name);

char * AmmServer_GetStrSettingValue(unsigned int set_type);
int AmmServer_SetStrSettingValue(unsigned int set_type,char * set_value);


int AmmServer_SelfCheck();

#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
