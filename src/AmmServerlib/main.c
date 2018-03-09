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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
// --------------------------------------------
#include "version.h"
// --------------------------------------------
#include "AmmServerlib.h"
#include "server_configuration.h"
#include "AString/AString.h"
// --------------------------------------------
#include "threads/serverAbstraction.h"
//#include "threads/threadedServer.h"
//#include "threads/prespawnedThreads.h"
// --------------------------------------------
#include "cache/file_caching.h"
#include "cache/dynamic_requests.h"
// --------------------------------------------
#include "scheduler/scheduler.h"
// --------------------------------------------
#include "tools/serverMonitor.h"
#include "tools/http_tools.h"
#include "tools/logs.h"
#include "tools/time_provider.h"
// --------------------------------------------
#include "header_analysis/get_data.h"
#include "header_analysis/post_data.h"
// --------------------------------------------
#include "templates/editor.h"
#include "templates/login.h"
// --------------------------------------------

//This is for calling back a client function after receiving
//a sigkill or other signal , after using AmmServer_RegisterTerminationSignal
void ( *TerminationCallback) (  )=0 ;

char * AmmServer_Version()
{
  return (char*) FULLVERSION_STRING;
}

int AmmServer_CheckIfHeaderBinaryAreTheSame(int headerSpec)
{
  if (AMMAR_SERVER_HTTP_HEADER_SPEC != headerSpec)
    {
      fprintf(stderr,RED "Please note that an inconsistency of binary and its header has been detected\n" NORMAL);
      return 0;
    }
  return 1;
}

int AmmServer_GetDateString(char * output , unsigned int maxOutput)
{
  return GetDateString(output,maxOutput,0,1,0,0,0,0,0,0,0);
}

int AmmServer_AppendToFile(const char * filename,const char * msg)
{
  return FileAppend(filename,msg);
}

void AmmServer_GeneralPrint( char * color,char * label,const char *format , va_list * arglist)
{
   unsigned int formatLength = 32+strlen(label)+strlen(format);
   char * coloredFormat= (char *) malloc( sizeof(char) * formatLength );

   if (coloredFormat!=0)
   {
     coloredFormat[0]=0;
     strcpy(coloredFormat,color);
     strcat(coloredFormat,label);
     strcat(coloredFormat,": ");
     strcat(coloredFormat,format);
     strcat(coloredFormat," \n ");
     strcat(coloredFormat,NORMAL );

     vfprintf(stderr,coloredFormat, *arglist );
     fflush(stderr);
     free(coloredFormat);
   } else
   {
      fprintf(stderr,RED "AmmServer_GeneralPrint failed to output %s, not enough memory..\n",format);
   }
 return;
}

void AmmServer_Warning( const char *format , ... )  { va_list arglist; va_start( arglist, format ); AmmServer_GeneralPrint(YELLOW,"Warning",format, &arglist );      va_end( arglist ); }
void AmmServer_Error( const char *format , ... )    { va_list arglist; va_start( arglist, format ); AmmServer_GeneralPrint(RED,"Error",format, &arglist );           va_end( arglist ); }
void AmmServer_Success( const char *format , ... )  { va_list arglist; va_start( arglist, format ); AmmServer_GeneralPrint(GREEN,"Success",format, &arglist );       va_end( arglist ); }
void AmmServer_Info( const char *format , ... )     { va_list arglist; va_start( arglist, format ); AmmServer_GeneralPrint(WHITE,"Info",format, &arglist );          va_end( arglist ); }
void AmmServer_Stub( const char *format , ... )     { va_list arglist; va_start( arglist, format ); AmmServer_GeneralPrint(RED,"Not Implemented",format, &arglist ); va_end( arglist ); }

int AmmServer_Stop(struct AmmServer_Instance * instance)
{
  warning("AmmServer_Stop started ..\n");
  if (!instance) { return 0; }

  if ( instance->webserverMonitorEnabled )
  {
    AmmServer_RemoveResourceHandler(instance,&instance->webserverMonitorPage,1);
  }

  ASRV_StopHTTPServer(instance);
  cache_Destroy(instance);

  if (instance->threads_pool!=0)    { free(instance->threads_pool); instance->threads_pool=0; }
  if (instance->prespawned_pool!=0) { free(instance->prespawned_pool); instance->prespawned_pool=0; }
  if (instance!=0)                  { free(instance); }

  warning("AmmServer_Stop completed ..\n");
  return 1;
}

struct AmmServer_Instance * AmmServer_Start( const char * name ,
                                             const char * ip,
                                             unsigned int port,
                                             const char * conf_file,
                                             const char * web_root_path,
                                             const char * templates_root_path
                                            )
{
  fprintf(stderr,"Binding AmmarServer v%s to %s:%u\n",FULLVERSION_STRING,ip,port);

  printDisclaimer();

  //log/ could be a global directory
  snprintf(AccessLog,MAX_FILE_PATH,"log/%s_access.log",name);
  snprintf(ErrorLog,MAX_FILE_PATH,"log/%s_error.log",name);
  fprintf(stderr,"Access logged @ %s , Errors logged @ %s \n ",AccessLog,ErrorLog);

  //Allocate and Clear instance..
  struct AmmServer_Instance * instance = (struct AmmServer_Instance *) malloc(sizeof(struct AmmServer_Instance));
  if (!instance) { fprintf(stderr,"AmmServer_Start failed to allocate a new instance \n"); } else
                 { memset(instance,0,sizeof(struct AmmServer_Instance)); }
  fprintf(stderr,"Initial AmmServer_Start instance pointing @ %p \n",instance);//Clear instance..!

  instance->threads_pool = (pthread_t *) malloc( sizeof(pthread_t) * MAX_CLIENT_THREADS);
  if (!instance->threads_pool) { fprintf(stderr,"AmmServer_Start failed to allocate %u records for a thread pool\n",MAX_CLIENT_THREADS);  } else
                               {  memset(instance->threads_pool,0,sizeof(pthread_t)*MAX_CLIENT_THREADS); }


  strncpy(instance->instanceName , name , MAX_INSTANCE_NAME_STRING); // TODO: check for MAX_INSTANCE_NAME_STRING



  fprintf(stderr,"Initial AmmServer_Start ( name %s ) thread pool pointing @ %p \n",instance->instanceName,instance->threads_pool);//Clear instance..!



   instance->settings.MAX_POST_TRANSACTION_SIZE = DEFAULT_MAX_HTTP_POST_REQUEST_HEADER;


  //LoadConfigurationFile happens before dropping root id so we are more sure that we will manage to read the configuration file..
  LoadConfigurationFile(instance,conf_file);

  //LoadConfigurationFile may set a binding port but if the parent call set a nonzero a port setting here it overrides configuration file....
  if (port!=0) { instance->settings.BINDING_PORT = port; }

  //This line explains configuration conflicts in a user understandable manner :p
  EmmitPossibleConfigurationWarnings(instance);


  cache_Initialize( instance,
                   /*These are the file cache settings , file caching is the mechanism that holds dynamic content and
                     speeds up file serving by not accessing the whole disk drive subsystem ..*/
                   MAX_SEPERATE_CACHE_ITEMS , /*Seperate items*/
                   MAX_CACHE_SIZE_IN_MB   , /*MB Limit for the WHOLE Cache*/
                   MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB    /*MB Max Size of Individual File*/
                  );

   if (ASRV_StartHTTPServer(instance,ip,instance->settings.BINDING_PORT,web_root_path,templates_root_path))
      {
          //All is well , we return a valid instance
            AccessLogAppend("127.0.0.1",0,"startup",1,0,"startup","ammarserver");
          return instance;
      } else
      {
          AmmServer_Stop(instance);
          return 0;
      }

  return 0;
}

struct AmmServer_Instance * AmmServer_StartWithArgs(const char * name ,
                                                    int argc,
                                                    char ** argv ,
                                                    const char * ip,
                                                    unsigned int port,
                                                    const char * conf_file,
                                                    const char * web_root_path,
                                                    const char * templates_root_path)
{
   //First prepare some buffers with default values for all the arguments
   char serverName[MAX_FILE_PATH]="default";
   char webserver_root[MAX_FILE_PATH]="public_html/";
   char templates_root[MAX_FILE_PATH]="public_html/templates/";
   char configuration_file[MAX_FILE_PATH]={0};
   char bindIP[MAX_IP_STRING_SIZE]="0.0.0.0";
   unsigned int bindPort=8080;

   //If we have arguments we change our buffers
   if (name!=0)           {  strncpy(serverName,name,MAX_FILE_PATH); }
   if (web_root_path!=0)  {  strncpy(webserver_root,web_root_path,MAX_FILE_PATH); }
   if (templates_root!=0) {  strncpy(templates_root,templates_root_path,MAX_FILE_PATH); }
   if (conf_file!=0)      {  strncpy(configuration_file,conf_file,MAX_FILE_PATH); }
   if (ip!=0)             {  strncpy(bindIP,ip,MAX_IP_STRING_SIZE); }
   if (port!=0)           {  bindPort=port; }

   //If we have a command line arguments we overwrite our buffers
  unsigned int argcUI = argc;
  if ( (argcUI>0) && (argv!=0) )
  {
   unsigned int i=0;
   for (i=0; i<argcUI; i++)
   {
    if ((strcmp(argv[i],"-bind")==0)&&(argcUI>i+1))        { strncpy(bindIP,argv[i+1],MAX_IP_STRING_SIZE);        fprintf(stderr,"Binding to %s \n",bindIP); } else
    if ((strcmp(argv[i],"-p")==0)&&(argcUI>i+1))           { bindPort = atoi(argv[i+1]);                          fprintf(stderr,"Binding to Port %u \n",bindPort); } else
    if ((strcmp(argv[i],"-port")==0)&&(argcUI>i+1))        { bindPort = atoi(argv[i+1]);                          fprintf(stderr,"Binding to Port %u \n",bindPort); } else
    if ((strcmp(argv[i],"-rootdir")==0)&&(argcUI>i+1))     { strncpy(webserver_root,argv[i+1],MAX_FILE_PATH);     fprintf(stderr,"Setting web server root directory to %s \n",webserver_root); } else
    if ((strcmp(argv[i],"-templatedir")==0)&&(argcUI>i+1)) { strncpy(templates_root,argv[i+1],MAX_FILE_PATH);     fprintf(stderr,"Setting web template directory to %s \n",templates_root); } else
    if (strcmp(argv[i],"-conf")==0)                        { strncpy(configuration_file,conf_file,MAX_FILE_PATH); fprintf(stderr,"Reading Configuration file %s \n",configuration_file); }
   }
  }

  return AmmServer_Start(name,bindIP,bindPort,configuration_file,webserver_root,templates_root);
}

int AmmServer_Running(struct AmmServer_Instance * instance)
{
  return ASRV_HTTPServerIsRunning(instance);
}

int AmmServer_DynamicRequestReturnFile(struct AmmServer_DynamicRequest  * rqst,const char * filename)
{
  if ( (rqst==0) || (rqst->content==0) || (filename==0) || ( strlen(filename)==0) )  { return 0; }
  rqst->contentContainsPathToFileToBeStreamed=1;
  snprintf(rqst->content,rqst->MAXcontentSize,"%s",filename);
  return 1;
}

void* AmmServer_DynamicRequestReturnMemoryHandler(struct AmmServer_DynamicRequest  * rqst,struct AmmServer_MemoryHandler * content)
{
  if (content==0) { return 0; }
  if (content->content==0) { return 0; }
  if (content->contentSize==0) { return 0; }

  if (rqst->MAXcontentSize <= content->contentSize )
  {
    AmmServer_Error("Not enough space to serve AmmServer_DynamicRequestReturnMemoryHandler ..");
    return 0;
  }

  memcpy(rqst->content,content->content,content->contentSize);
  rqst->contentSize = content->contentSize;

  return 0;
}

//This call , calls  callback every time a request hits the server..
//The outer layer of the server can do interesting things with it :P
//request_type is supposed to be GET , HEAD , POST , CONNECT , etc..
int AmmServer_AddRequestHandler(struct AmmServer_Instance * instance,struct AmmServer_RequestOverride_Context * RequestOverrideContext,const char * request_type,void * callback)
{
  if ( (instance==0)||(RequestOverrideContext==0)||(request_type==0)||(callback==0) ) { return 0; }
  strncpy( RequestOverrideContext->requestHeader , request_type , 64 /*limit declared on AmmServerlib.h*/) ;
  RequestOverrideContext->request=0;

  instance->clientRequestHandlerOverrideContext = RequestOverrideContext;
  RequestOverrideContext->request_override_callback = callback;

  AmmServer_Warning("AmmServer_AddRequestHandler could potentially be buggy\n");

  return 1;
}

int AmmServer_AddScheduler (
                            struct AmmServer_Instance * instance,
                            const char * resource_name ,
                            void * callback,
                            unsigned int delayMilliseconds,
                            unsigned int repetitions
                           )
{
  AmmServer_Stub("TODO add scheduler code ( %s ) .. \n",instance->instanceName);
  return schedulerAdd(
                      resource_name ,
                      callback,
                      delayMilliseconds,
                      repetitions
                     );
}

int AmmServer_AddResourceHandler
     ( struct AmmServer_Instance * instance,
       struct AmmServer_RH_Context * context,
       const char * resource_name ,
       //const char * web_root,
       unsigned int allocate_mem_bytes,
       unsigned int callback_every_x_msec,
       void * callback,
       unsigned int scenario
    )
{
   if ( context->requestContext.content!=0 )
    {
      AmmServer_Warning("Context in AmmServer_AddResourceHandler for %s appears to have an already initialized memory part\n",resource_name);
      AmmServer_Warning("Make sure that you are using a seperate context for each AmmServer_AddResourceHandler call you make..\n");
    }
   memset(context,0,sizeof(struct AmmServer_RH_Context));
   strncpy(context->resource_name,resource_name,MAX_RESOURCE);

   //Each New Resource Handler inherits the central webserver root, if we really know what we are doing  we can set up a different root dir per context
   //But this is almost never needed..
   strncpy(context->web_root_path,instance->webserver_root,MAX_FILE_PATH);
   context->requestContext.instance=instance; // Remember the instance that created this..
   context->requestContext.MAXcontentSize=allocate_mem_bytes;
   context->callback_every_x_msec=callback_every_x_msec;
   context->last_callback=0; //This is important because a random value here will screw up things with callback_every_x_msec..
   context->callback_cooldown=0;
   context->RH_Scenario = scenario;

   if((scenario&ENABLE_RECEIVING_FILES)        == ENABLE_RECEIVING_FILES         ) { context->enablePOST=1; instance->settings.ENABLE_POST=1; } else { context->enablePOST=0; }
   if((scenario&DIFFERENT_PAGE_FOR_EACH_CLIENT)== DIFFERENT_PAGE_FOR_EACH_CLIENT ) { context->needsDifferentPageForEachClient=1;              } else { context->needsDifferentPageForEachClient=0; }
   if((scenario&SAME_PAGE_FOR_ALL_CLIENTS)     == SAME_PAGE_FOR_ALL_CLIENTS      ) { context->needsSamePageForAllClients=1;                   } else { context->needsSamePageForAllClients=0; }

   if ( allocate_mem_bytes>0 )
    {
       context->requestContext.content = (char*) malloc( sizeof(char) * allocate_mem_bytes );
       if (context->requestContext.content==0) { AmmServer_Warning("Could not allocate space for request Context"); }
    }

   context->dynamicRequestCallbackFunction=callback;
   if (callback==0) { AmmServer_Warning("No callback passed for a new AmmServer_AddResourceHandler "); }


   int returnValue = cache_AddMemoryBlock(instance,context);

   if (!returnValue)
   {
     AmmServer_Error("Failed adding new resource handler\n Resource name `%s` will be unavailable\n",resource_name);
   }

  return returnValue;
}

/**
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
                                  Built-in and ready to use Dynamic Request Functionality calls..
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
**/

void * AmmServer_EditorCallback(struct AmmServer_DynamicRequest  * rqst)
{
  return editor_callback(rqst);
}

void * AmmServer_LoginCallback(struct AmmServer_DynamicRequest  * rqst)
{
  return login_callback(rqst);
}

int AmmServer_AddEditorResourceHandler(
                                       struct AmmServer_Instance * instance,
                                       struct AmmServer_RH_Context * context,
                                       const char * resource_name ,
                                       //const char * web_root ,
                                       void * callback
                                      )
{
 #warning "TODO: Also remove editor resource handler"
 int res = AmmServer_AddResourceHandler (  instance, context, resource_name , 16000, 0, callback, DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES );

  if (res) { AmmServer_DoNOTCacheResourceHandler(instance,context); }
 return res;
}

int AmmServer_EnableMonitor( struct AmmServer_Instance * instance)
{
  warning("Enabling Monitor..");
  instance->webserverMonitorEnabled=1;
  return AmmServer_AddResourceHandler
     ( instance,
       &instance->webserverMonitorPage,
       "/monitor.html",
       16000,
       10,
       &serveMonitorPage,
       SAME_PAGE_FOR_ALL_CLIENTS
    );

}

/**
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
**/
int AmmServer_PreCacheFile(struct AmmServer_Instance * instance,const char * filename)
{
  unsigned int index=0;
  return cache_AddFile(instance,filename,&index,0);
}

int AmmServer_DoNOTCacheResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
{
    char resource_name[MAX_FILE_PATH]={0};

    snprintf(resource_name,MAX_FILE_PATH,"%s%s",context->web_root_path,context->resource_name);

    if (! cache_AddDoNOTCacheRuleForResource(instance,resource_name) )
     {
       fprintf(stderr,"Could not set AmmServer_DoNOTCacheResourceHandler for resource %s\n",resource_name);
       return 0;
     }
    return 1;
}

int AmmServer_DoNOTCacheResource(struct AmmServer_Instance * instance,const char * resource_name)
{
    if (! cache_AddDoNOTCacheRuleForResource(instance,resource_name) )
     {
       fprintf(stderr,"Could not set AmmServer_DoNOTCacheResource for resource %s\n",resource_name);
       return 0;
     }
    return 1;
}

int AmmServer_RemoveResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem)
{
  return cache_RemoveContextAndResource(instance,context,free_mem);
}

int AmmServer_GetInfo(struct AmmServer_Instance * instance,unsigned int info_type)
{
  switch (info_type)
   {
     case AMMINF_ACTIVE_CLIENTS : return instance->CLIENT_THREADS_STARTED - instance->CLIENT_THREADS_STOPPED; break;
   };
  return 0;
}
/**
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
               POST / GET / FILE Accessors , this is one of the most basic and crucial functionality of AmmarServer..
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
**/

/// --------------------------------------------------------- POST ---------------------------------------------------------
int _POSTnum(struct AmmServer_DynamicRequest * rqst)
{
 return getNumberOfPOSTItems(rqst);
}

char * _POST(struct AmmServer_DynamicRequest * rqst,const char * name,unsigned int * valueLength)
{
 return  getPointerToPOSTItemValue(rqst,name,valueLength);
}

unsigned int _POSTuint(struct AmmServer_DynamicRequest * rqst,const char * name)
{
  unsigned int valueLength=0;
  const char * value = _POST(rqst,name,&valueLength);
  if (value!=0)
   { return  atoi(value); }
  return 0;
}

int _POSTexists(struct AmmServer_DynamicRequest * rqst,const char * name)
{
  unsigned int valueLength=0;
  return (_POST(rqst,name,&valueLength) != 0 );
}

int _POSTcmp(struct AmmServer_DynamicRequest * rqst,const char * name,const char * what2CompareTo)
{
  unsigned int valueLength=0;
  const char * value = _POST(rqst,name,&valueLength);
  return strcmp(value,what2CompareTo);
}

int _POSTcpy(struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize)
{
  unsigned int valueLength=0;
  const char * value = _POST(rqst,name,&valueLength);
  return _GENERIC_cpy(value,valueLength,destination,destinationSize);
}
/// -----------------------------------------------------------------------------------------------------------------------

/// --------------------------------------------------------- GET ---------------------------------------------------------
int _GETnum(struct AmmServer_DynamicRequest * rqst)
{
 return getNumberOfPOSTItems(rqst);
}

char * _GET(struct AmmServer_DynamicRequest * rqst,const char * name,unsigned int * valueLength)
{
 return  getPointerToGETItemValue(rqst,name,valueLength);
}

unsigned int _GETuint(struct AmmServer_DynamicRequest * rqst,const char * name)
{
  unsigned int valueLength=0;
  const char * value = _GET(rqst,name,&valueLength);
  if (value!=0)
   { return  atoi(value); }
  return 0;
}

int _GETexists(struct AmmServer_DynamicRequest * rqst,const char * name)
{
  unsigned int valueLength=0;
  return (_GET(rqst,name,&valueLength) != 0 );
}

int _GETcmp(struct AmmServer_DynamicRequest * rqst,const char * name,const char * what2CompareTo)
{
  unsigned int valueLength=0;
  const char * value = _GET(rqst,name,&valueLength);
  return strcmp(value,what2CompareTo);
}

int _GETcpy(struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize)
{
  unsigned int valueLength=0;
  const char * value = _GET(rqst,name,&valueLength);
  return _GENERIC_cpy(value,valueLength,destination,destinationSize);
}
/// -----------------------------------------------------------------------------------------------------------------------

/// --------------------------------------------------------- FILES ---------------------------------------------------------
const char * _FILES(struct AmmServer_DynamicRequest * rqst,const char * POSTName,enum TypesOfRequestFields POSTType,unsigned int * outputSize)
{
  switch (POSTType)
  {
    case NAME :       return POSTName; /*This makes no sense*/                        break;
    case VALUE :      return getPointerToPOSTItemValue(rqst,POSTName,outputSize);     break;
    case FILENAME :   return getPointerToPOSTItemFilename(rqst,POSTName,outputSize);  break;
    case TEMPNAME :                                                                   break;
    case TYPE :       return getPointerToPOSTItemType(rqst,POSTName,outputSize);      break;
    case SIZE :                                                                       break;
  };
 return 0;
}
/// -----------------------------------------------------------------------------------------------------------------------

/// --------------------------------------------------------- COOKIE ---------------------------------------------------------

int _COOKIE(struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize)
{
    AmmServer_Stub("Cookie access not coded in yet..!");
    return 0;
}
/// -----------------------------------------------------------------------------------------------------------------------
/**
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
**/

int AmmServer_SignalCountAsBadClientBehaviour(struct AmmServer_DynamicRequest * rqst)
{
   AmmServer_Stub("AmmServer_SignalCountAsBadClientBehaviour is a stub ..\n");
   return 0;
}

int AmmServer_SaveDynamicRequest(const char* filename , struct AmmServer_DynamicRequest * rqst)
{
    return saveDynamicRequest(filename,rqst->instance,rqst);
}

/*
The following calls are not implemented yet
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
     case AMMSET_PASSWORD_PROTECTION      :  instance->settings.PASSWORD_PROTECTION=set_value; return 1; break;
     case AMMSET_MAX_POST_TRANSACTION_SIZE     :  instance->settings.MAX_POST_TRANSACTION_SIZE=set_value; return 1; break;
     case AMMSET_RANDOMIZE_ETAG_BEGINNING :  return cache_RandomizeETAG(instance);  break;
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

int AmmServer_SetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,const char * set_value)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :  AssignStr(&instance->settings.USERNAME,set_value); return SetUsernameAndPassword(instance,instance->settings.USERNAME,instance->settings.PASSWORD); break;
     case AMMSET_PASSWORD_STR :  AssignStr(&instance->settings.PASSWORD,set_value); return SetUsernameAndPassword(instance,instance->settings.USERNAME,instance->settings.PASSWORD); break;
   };
  return 0;
}

int AmmServer_SelfCheck(struct AmmServer_Instance * instance)
{
  fprintf(stderr,"No Checks Implemented in this version , instance pointer is %p ..\n",instance);
  return 0;
}

void AmmServer_GlobalTerminationHandler(int signum)
{
        fprintf(stderr,"Terminating AmmarServer Instance after receiving signum %i .. \n",signum);
          GLOBAL_KILL_SERVER_SWITCH=1;
        //&
        if (TerminationCallback!=0) { TerminationCallback(); }
        fprintf(stderr,"done\n");
        exit(0);
}

int AmmServer_RegisterTerminationSignal(void * callback)
{
  TerminationCallback = callback;

  unsigned int failures=0;
  if (signal(SIGINT, AmmServer_GlobalTerminationHandler)  == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGINT!\n");  ++failures; }
  if (signal(SIGHUP, AmmServer_GlobalTerminationHandler)  == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGHUP!\n");  ++failures; }
  if (signal(SIGTERM, AmmServer_GlobalTerminationHandler) == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGTERM!\n"); ++failures; }
  if (signal(SIGKILL, AmmServer_GlobalTerminationHandler) == SIG_ERR) { AmmServer_Warning("AmmarServer cannot handle SIGKILL!\n"); ++failures; }
  return (failures==0);
}


/**
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
                                   LAST , SOME GENERIC TOOLS THAT ARE HANDY AND COMMON
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
    -----------------------------------------------------------------------------------------------------------------------------
**/

int AmmServer_ExecuteCommandLineNum(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize,unsigned int lineNumber)
{
 /* Open the command for reading. */
 FILE * fp = popen(command, "r");
 if (fp == 0 )
       {
         fprintf(stderr,"Failed to run command (%s) \n",command);
         return 0;
       }

 /* Read the output a line at a time - output it. */
  unsigned int i=0;
  while (fgets(what2GetBack, what2GetBackMaxSize , fp) != 0)
    {
        ++i;
        if (lineNumber==i) { break; }
    }
  /* close */
  pclose(fp);
  return 1;
}

int AmmServer_ExecuteCommandLine(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize)
{
  return AmmServer_ExecuteCommandLineNum(command,what2GetBack,what2GetBackMaxSize,1);
}

char * AmmServer_ReadFileToMemory(const char * filename,unsigned int *length )
{
  return astringReadFileToMemory(filename,length);
}

int AmmServer_WriteFileFromMemory(const char * filename,const char * memory , unsigned int memoryLength)
{
  return astringWriteFileFromMemory(filename,memory,memoryLength);
}

int AmmServer_ReplaceVariableInMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value)
{
  return astringInjectDataToMemoryHandler(mh,var,value);
}

int AmmServer_ReplaceAllVarsInMemoryHandler(struct AmmServer_MemoryHandler * mh ,unsigned int instances,const char * var,const char * value)
{
  return astringReplaceAllInstancesOfVarInMemoryFile(mh,instances,var,value);
}

int AmmServer_ReplaceAllVarsInDynamicRequest(struct AmmServer_DynamicRequest * dr ,unsigned int instances,const char * var,const char * value)
{

   struct AmmServer_MemoryHandler mh;
   mh.contentSize          = dr->MAXcontentSize;
   mh.contentCurrentLength = dr->contentSize;
   mh.content              = dr->content;

   int status=AmmServer_ReplaceAllVarsInMemoryHandler(&mh,instances,var,value);
     if (status)
     {
      dr->content = mh.content;
      dr->contentSize = mh.contentCurrentLength;
      dr->MAXcontentSize = mh.contentSize;
     }

   return status;
}

void AmmServer_ReplaceCharInString(char * input , char findChar , char replaceWith)
{
  char * cur = input;
  char * inputEnd = input+strlen(input);
  while ( cur < inputEnd )
  {
     if (*cur == findChar ) { *cur = replaceWith; }
     ++cur;
  }
  return ;
}

struct AmmServer_MemoryHandler * AmmServer_AllocateMemoryHandler(unsigned int initialBufferLength, unsigned int growStep)
{
 struct AmmServer_MemoryHandler * mh = ( struct AmmServer_MemoryHandler * ) malloc(sizeof(struct AmmServer_MemoryHandler));

 if (mh!=0)
 {
  unsigned long allocationSize = initialBufferLength * sizeof(char);
  mh->content = (char*) malloc(allocationSize);
  if (mh->content!=0)
    {
     mh->contentSize = initialBufferLength;
     mh->contentGrowthStep = growStep;
     mh->contentCurrentLength = initialBufferLength;
     return mh;
    } else
    {
     AmmServer_Error("Could not allocate the buffer of the allocated memory handler\n");
     safeFree(mh,allocationSize);
    }
 } else
 { AmmServer_Error("Could not allocate a memory handler of %u bytes length\n",initialBufferLength); }

 return 0;
}

struct AmmServer_MemoryHandler *  AmmServer_ReadFileToMemoryHandler(const char * filename)
{
  struct AmmServer_MemoryHandler * mh = ( struct AmmServer_MemoryHandler * ) malloc(sizeof(struct AmmServer_MemoryHandler));

  if (mh!=0)
  {
   mh->content = AmmServer_ReadFileToMemory(filename,&mh->contentSize);
   mh->contentCurrentLength = mh->contentSize;

   if (mh->content!=0)
   {
     return mh;
   } else
   {
     AmmServer_Error("AmmServer_ReadFileToMemory could not allocate memory\n");
     free(mh);
   }
  } else
  {  AmmServer_Error("AmmServer_ReadFileToMemoryHandler could not allocate memory to hold copy..");  }

  return 0;
}

struct AmmServer_MemoryHandler * AmmServer_CopyMemoryHandler(struct AmmServer_MemoryHandler * inpt)
{
  if (inpt==0)          { AmmServer_Warning("AmmServer_CopyMemoryHandler can't copy a null memory handler..");      return 0; }

  struct AmmServer_MemoryHandler * mh = ( struct AmmServer_MemoryHandler * ) malloc(sizeof(struct AmmServer_MemoryHandler));

  if (mh==0)            { AmmServer_Error("AmmServer_CopyMemoryHandler could not allocate memory to hold copy..");  return 0; }

  mh->contentCurrentLength = inpt->contentCurrentLength;
  mh->contentSize = inpt->contentSize ;

   if (inpt->content==0)
      {
        mh->content=0;
      }
       else
      {
        mh->content = (char *) malloc(sizeof(char) * inpt->contentSize );
        memcpy(mh->content,inpt->content,inpt->contentSize);
      }

   return mh;
}

int filterStringForHtmlInjection(char * buffer , unsigned int bufferSize)
{
  AmmServer_Warning("filterStringForHtmlInjection not implemented ( %s , %u ) ",buffer,bufferSize);
  return 0;
}

int AmmServer_FreeMemoryHandler(struct AmmServer_MemoryHandler ** mh)
{
  if (*mh==0) { return 0; }

  struct AmmServer_MemoryHandler * tmp = *mh;
  free(tmp->content);
  free(*mh);
  return 0;
}

int AmmServer_ConvertBufferToMemoryHandler(struct AmmServer_MemoryHandler * mh, unsigned char * buffer,unsigned int bufferLength)
{
  if (mh==0) { return 0; }
  mh->content = (char*) buffer;
  mh->contentSize = bufferLength;
  mh->contentCurrentLength = bufferLength;
  return 1;
}

const char * AmmServer_GetDirectoryFromPath(char * path)
{
 return dirname(path);
}

const char * AmmServer_GetBasenameFromPath(char * path)
{
 return basename(path);
}

const char * AmmServer_GetExtensionFromPath(char * path)
{
 char * bname = basename(path);
 const char *dot = strrchr(bname, '.');
 if(!dot || dot == bname) return "";
 return dot + 1;
}

int AmmServer_DirectoryExists(const char * filename) { return DirectoryExistsAmmServ(filename); }
int AmmServer_FileExists(const char * filename)      { return FileExistsAmmServ(filename);      }
int AmmServer_FileIsText(const char * filename)      { return CheckIfFileIsText(filename);      }
int AmmServer_FileIsAudio(const char * filename)     { return CheckIfFileIsAudio(filename);     }
int AmmServer_FileIsImage(const char * filename)     { return CheckIfFileIsImage(filename);     }
int AmmServer_FileIsVideo(const char * filename)     { return CheckIfFileIsVideo(filename);     }
int AmmServer_FileIsFlash(const char * filename)     { return CheckIfFileIsFlash(filename);     }

int AmmServer_EraseFile(const char * filename)
{
 FILE *fp = fopen(filename,"w");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 return 0;
}

unsigned int AmmServer_StringIsHTMLSafe( const char * str)
{
  unsigned int i=0;
  while(i<strlen(str)) { if ( ( str[i]<'!' ) || ( str[i]=='<' ) || ( str[i]=='>' ) ) { return 0;} ++i; }
  return 1;
}

unsigned int AmmServer_StringHasSafePath( const char * directory , const char * filenameUNSANITIZEDString)
{
  const char * str = filenameUNSANITIZEDString;
  AmmServer_Stub("TODO : AmmServer_StringHasSafePath better checking ( also use directory ).. https://www.owasp.org/index.php/Path_Traversal\n");

  unsigned int i=0;
  while(i<strlen(str)) { if (  ( str[i]<'!' ) || ( str[i]=='\\' ) || ( str[i]=='%' ) || ( str[i]=='/' ) ) { return 0;} ++i; }

  return 1;
}
