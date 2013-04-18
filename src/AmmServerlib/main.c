/*
AmmarServer , HTTP Server Library

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "version.h"
#include "AmmServerlib.h"
#include "server_threads.h"
#include "cache/file_caching.h"
#include "version.h"
#include "tools/http_tools.h"

char * AmmServer_Version()
{
  return FULLVERSION_STRING;
}


int AmmServer_Stop(struct AmmServer_Instance * instance)
{
  if (!instance) { return 0; }
  StopHTTPServer(instance);
  DestroyCache(instance);

  if (instance->threads_pool!=0) { free(instance->threads_pool); instance->threads_pool=0; }
  if (instance->prespawned_pool!=0) { free(instance->prespawned_pool); instance->prespawned_pool=0; }
  if (instance!=0) { free(instance); }
  return 1;
}

struct AmmServer_Instance * AmmServer_Start(char * ip,unsigned int port,char * conf_file,char * web_root_path,char * templates_root_path)
{
  fprintf(stderr,"Binding AmmarServer v%s to %s:%u\n",FULLVERSION_STRING,ip,port);


  fprintf(stderr,"\n\nDISCLAIMER : \n");
  fprintf(stderr,"Please note that this server version is not thoroughly\n");
  fprintf(stderr," pen-tested so it is not meant for production deployment..\n");

  fprintf(stderr,"Bug reports and feedback are very welcome.. \n");
  fprintf(stderr,"via https://github.com/AmmarkoV/AmmarServer/issues\n\n");


  //Allocate and Clear instance..
  struct AmmServer_Instance * instance = (struct AmmServer_Instance *) malloc(sizeof(struct AmmServer_Instance));
  if (!instance) { fprintf(stderr,"AmmServer_Start failed to allocate a new instance \n"); } else
                 { memset(instance,0,sizeof(struct AmmServer_Instance)); }
  fprintf(stderr,"Initial AmmServer_Start instance pointing @ %p \n",instance);//Clear instance..!

  instance->threads_pool = (pthread_t *) malloc( sizeof(pthread_t) * MAX_CLIENT_THREADS);
  if (!instance->threads_pool) { fprintf(stderr,"AmmServer_Start failed to allocate %u records for a thread pool\n",MAX_CLIENT_THREADS);  } else
                               {  memset(instance->threads_pool,0,sizeof(pthread_t)*MAX_CLIENT_THREADS); }
  fprintf(stderr,"Initial AmmServer_Start thread pool pointing @ %p \n",instance->threads_pool);//Clear instance..!

  instance->prespawned_pool = (void *) malloc( sizeof(struct PreSpawnedThread) * MAX_CLIENT_PRESPAWNED_THREADS);
  if (!instance->prespawned_pool) { fprintf(stderr,"AmmServer_Start failed to allocate %u records for a prespawned thread pool\n",MAX_CLIENT_PRESPAWNED_THREADS);  }else
                                  {  memset(instance->prespawned_pool,0,sizeof(pthread_t)*MAX_CLIENT_PRESPAWNED_THREADS);  }



  //LoadConfigurationFile happens before dropping root id so we are more sure that we will manage to read the configuration file..
  LoadConfigurationFile(instance,conf_file);

  //LoadConfigurationFile may set a binding port but if the parent call set a nonzero a port setting here it overrides configuration file....
  if (port!=0) { instance->settings.BINDING_PORT = port; }

  //This line explains configuration conflicts in a user understandable manner :p
  EmmitPossibleConfigurationWarnings();


  InitializeCache( instance,
                   /*These are the file cache settings , file caching is the mechanism that holds dynamic content and
                     speeds up file serving by not accessing the whole disk drive subsystem ..*/
                   MAX_SEPERATE_CACHE_ITEMS , /*Seperate items*/
                   MAX_CACHE_SIZE_IN_MB   , /*MB Limit for the WHOLE Cache*/
                   MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB    /*MB Max Size of Individual File*/
                  );

   if (StartHTTPServer(instance,ip,instance->settings.BINDING_PORT,web_root_path,templates_root_path))
      {
          //All is well , we return a valid instance
          return instance;
      } else
      {
          AmmServer_Stop(instance);
          return 0;
      }

  return 0;
}


int AmmServer_Running(struct AmmServer_Instance * instance)
{
  return HTTPServerIsRunning(instance);
}



//This call , calls  callback every time a request hits the server..
//The outer layer of the server can do interesting things with it :P
//request_type is supposed to be GET , HEAD , POST , CONNECT , etc..
int AmmServer_AddRequestHandler(struct AmmServer_Instance * instance,struct AmmServer_RequestOverride_Context * context,char * request_type,void * callback)
{
  if ( (context==0)||(request_type==0)||(callback==0) ) { return 0; }
  strncpy( context->requestHeader , request_type , 64 /*limit declared on AmmServerlib.h*/) ;
  context->request=0;


  // void * request_override_callback;
  //TODO add the callback somewhere that makes sense!

  return 1;
}


int AmmServer_AddResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context, char * resource_name , char * web_root, unsigned int allocate_mem_bytes,unsigned int callback_every_x_msec,void * callback,unsigned int scenario)
{
   if ( context->content!=0 ) { fprintf(stderr,"Context in AmmServer_AddResourceHandler for %s appears to have an already initialized memory part\n",resource_name); }
   memset(context,0,sizeof(struct AmmServer_RH_Context));
   strncpy(context->web_root_path,web_root,MAX_FILE_PATH);
   strncpy(context->resource_name,resource_name,MAX_RESOURCE);
   context->MAX_content_size=allocate_mem_bytes;
   context->prepare_content_callback=callback;
   context->callback_every_x_msec=callback_every_x_msec;
   context->last_callback=0; //This is important because a random value here will screw up things with callback_every_x_msec..
   context->callback_cooldown=0;
   context->RH_Scenario = scenario;

   if ( allocate_mem_bytes>0 )
    {
       context->content = (char*) malloc( sizeof(char) * allocate_mem_bytes );
    }
  return AddDirectResource_As_CacheItem(instance,context);
}


int AmmServer_PreCacheFile(struct AmmServer_Instance * instance,char * filename)
{
  unsigned int index=0;
  return AddFile_As_CacheItem(instance,filename,&index,0);
}


int AmmServer_DoNOTCacheResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
{
    char resource_name[MAX_FILE_PATH]={0};
    strcpy(resource_name,context->web_root_path);
    strcat(resource_name,context->resource_name);

    if (! AddDoNOTCache_CacheItem(instance,resource_name) )
     {
       fprintf(stderr,"Could not set AmmServer_DoNOTCacheResourceHandler for resource %s\n",resource_name);
       return 0;
     }
    return 1;
}



int AmmServer_DoNOTCacheResource(struct AmmServer_Instance * instance,char * resource_name)
{
    if (! AddDoNOTCache_CacheItem(instance,resource_name) )
     {
       fprintf(stderr,"Could not set AmmServer_DoNOTCacheResource for resource %s\n",resource_name);
       return 0;
     }
    return 1;
}


int AmmServer_RemoveResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem)
{
  return RemoveDirectResource_CacheItem(instance,context,free_mem);
}



int AmmServer_GetInfo(struct AmmServer_Instance * instance,unsigned int info_type)
{
  switch (info_type)
   {
     case AMMINF_ACTIVE_CLIENTS : return instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED; break;
   };
  return 0;
}


int AmmServer_POSTArg(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
  if  (  ( context->POST_request !=0 ) && ( context->POST_request_length !=0 ) &&  ( var_id_IN !=0 ) &&  ( var_value_OUT !=0 ) && ( max_var_value_OUT !=0 )  )
   {
     return StripVariableFromGETorPOSTString(context->POST_request,var_id_IN,var_value_OUT,max_var_value_OUT);
   } else
   { fprintf(stderr,"AmmServer_POSTArg failed , called with incorrect parameters..\n"); }
  return 0;
}

int AmmServer_GETArg(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
  if  (  ( context->GET_request !=0 ) && ( context->GET_request_length !=0 ) &&  ( var_id_IN !=0 ) &&  ( var_value_OUT !=0 ) && ( max_var_value_OUT !=0 )  )
   {
     return StripVariableFromGETorPOSTString(context->GET_request,var_id_IN,var_value_OUT,max_var_value_OUT);
   } else
   { fprintf(stderr,"AmmServer_GETArg failed , called with incorrect parameters..\n"); }
  return 0;
}

int AmmServer_FILES(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
  fprintf(stderr,"AmmServer_FILES failed , called with incorrect parameters..\n");
  return 0;
}

/*User friendly aliases of the above calls.. :P */

int _POST(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
    return AmmServer_POSTArg(instance,context,var_id_IN,var_value_OUT,max_var_value_OUT);
}

int _GET(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
    return AmmServer_GETArg(instance,context,var_id_IN,var_value_OUT,max_var_value_OUT);
}

int _FILES(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT)
{
    return AmmServer_FILES(instance,context,var_id_IN,var_value_OUT,max_var_value_OUT);
}


/*
The following calls are not implemented yet

   ||
  \||/
   \/
*/

int AmmServer_GetIntSettingValue(struct AmmServer_Instance * instance,unsigned int set_type)
{
  switch (set_type)
   {
     case AMMSET_PASSWORD_PROTECTION : return instance->settings.PASSWORD_PROTECTION; break;
   };
  return 0;
}

int AmmServer_SetIntSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,int set_value)
{
  switch (set_type)
   {
     case AMMSET_PASSWORD_PROTECTION :  instance->settings.PASSWORD_PROTECTION=set_value; return 1; break;
   };
  return 0;
}


char * AmmServer_GetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :    return instance->settings.USERNAME; break;
     case AMMSET_PASSWORD_STR :    return instance->settings.PASSWORD; break;
   };
  return 0;
}

int AmmServer_SetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,char * set_value)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :  AssignStr(&instance->settings.USERNAME,set_value); return SetUsernameAndPassword(instance,instance->settings.USERNAME,instance->settings.PASSWORD); break;
     case AMMSET_PASSWORD_STR :  AssignStr(&instance->settings.PASSWORD,set_value); return SetUsernameAndPassword(instance,instance->settings.USERNAME,instance->settings.PASSWORD); break;
   };
  return 0;
}


struct AmmServer_Instance *  AmmServer_StartAdminInstance(char * ip,unsigned int port)
{
  return 0;
}



int AmmServer_SelfCheck(struct AmmServer_Instance * instance)
{
  fprintf(stderr,"No Checks Implemented in this version , instance pointer is %p ..\n",instance);
  return 0;
}





char * AmmServer_ReadFileToMemory(char * filename,unsigned int *length )
{
  *length = 0;
  FILE * pFile=0;

  char * buffer;
  size_t result;

  pFile = fopen ( filename , "rb" );
  if (pFile==0) { fprintf(stderr,"Could not read file %s \n",filename); return 0; }

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  long lSize = ftell (pFile);
  rewind (pFile);

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == 0 ) { fprintf(stderr,"Could not allocate enough memory for file %s ",filename); fclose(pFile); return 0; }

  // copy the file into the buffer:
  result = fread (buffer,1,lSize,pFile);
  if (result != lSize) { fprintf(stderr,"Could not read the whole file onto memory %s ",filename); fclose(pFile); return 0; }

  /* the whole file is now loaded in the memory buffer. */

  // terminate
  fclose (pFile);


  *length = lSize;
  return buffer;
}



void AmmServer_GlobalTerminationHandler(int signum)
{
        fprintf(stderr,"Terminating AmmarServer.. \n");
        close_dynamic_content();
          GLOBAL_KILL_SERVER_SWITCH=1;
        fprintf(stderr,"done\n");
        exit(0);
}


int AmmServer_RegisterTerminationSignal()
{
  unsigned int failures=0;
  if (signal(SIGINT, AmmServer_GlobalTerminationHandler) == SIG_ERR)  { printf("AmmarServer cannot handle SIGINT!\n");  ++failures; }
  if (signal(SIGHUP, AmmServer_GlobalTerminationHandler) == SIG_ERR)  { printf("AmmarServer cannot handle SIGHUP!\n");  ++failures; }
  if (signal(SIGTERM, AmmServer_GlobalTerminationHandler) == SIG_ERR) { printf("AmmarServer cannot handle SIGTERM!\n"); ++failures; }
  if (signal(SIGKILL, AmmServer_GlobalTerminationHandler) == SIG_ERR) { printf("AmmarServer cannot handle SIGKILL!\n"); ++failures; }
  return (failures==0);
}





