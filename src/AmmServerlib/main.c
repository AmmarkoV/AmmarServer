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
#include "version.h"
#include "AmmServerlib.h"
#include "server_threads.h"
#include "file_caching.h"



int AmmServer_Start(char * ip,unsigned int port,char * conf_file,char * web_root_path,char * templates_root_path)
{
  fprintf(stderr,"Binding AmmarServer v%s to %s:%u\n",FULLVERSION_STRING,ip,port);
  fprintf(stderr,"\n\nDISCLAIMER : \n");
  fprintf(stderr,"Please note that this server version is not thoroughly\n");
  fprintf(stderr," pen-tested so it is not meant for production deployment..\n");

  fprintf(stderr,"Bug reports and feedback are very welcome.. \n");
  fprintf(stderr,"via https://github.com/AmmarkoV/AmmarServer/issues\n\n");

  fprintf(stderr,"TODO: TOP PRIORITY -> Implement POST requests , and couple them to dynamic content ..\n");
  fprintf(stderr,"TODO: Implement download resume capabilities ( range head request ) ..\n");
  fprintf(stderr,"TODO: require the Host: header from HTTP 1.1 clients\n");
  fprintf(stderr,"TODO: accept absolute URL's in a request\n");
  fprintf(stderr,"TODO: accept requests with chunked data\n");
  fprintf(stderr,"TODO: use the \"100 Continue\" response appropriately\n");
  fprintf(stderr,"TODO: handle requests with If-Modified-Since: or If-Unmodified-Since: headers\n");
  fprintf(stderr,"TODO: Add configuration file ammServ.conf parsing..\n");
  fprintf(stderr,"TODO: Add detailed input header parsing\n");
  fprintf(stderr,"TODO: Improve directory listings ( add filesizes , dates etc ) \n");
  fprintf(stderr,"TODO: Improve implemented file caching mechanism ( add string comparison to make code hash collision free ) \n");
  fprintf(stderr,"TODO: Improve dynamic content handling ( coming from programs statically linked to the webserver ) ..\n");
  fprintf(stderr,"TODO: Add apache like logging capabilities\n");
  fprintf(stderr,"TODO: Implement gzip gunzip file compression , especially in cache for txt,html low entropy files\n");

  LoadConfigurationFile(conf_file);

  InitializeCache(
                   /*These are the file cache settings , file caching is the mechanism that holds dynamic content and
                     speeds up file serving by not accessing the whole disk drive subsystem ..*/
                   2000 , /*Seperate items*/
                   64   , /*MB Limit for the WHOLE Cache*/
                   3    , /*MB Max Size of Individual File*/

                   /*These are the POST/GET variable cache settings , POST and GET variables are stored in a seperate cache
                     in order for dynamic content pages to be able to ..*/
                   100  , /*Seperate GET/POST client entries */
                   10   , /*MB Limit for the WHOLE GET/POST entry memory allocation*/
                   1     /*MB Max Size of an Individual GET/POST entry memory allocation*/
                  );
  return StartHTTPServer(ip,port,web_root_path,templates_root_path);
}

int AmmServer_Stop()
{
  DestroyCache();
  return StopHTTPServer();
}

int AmmServer_Running()
{
  return HTTPServerIsRunning();
}

int AmmServer_AddResourceHandler(struct AmmServer_RH_Context * context, char * resource_name , char * web_root, unsigned int allocate_mem_bytes,unsigned int callback_every_x_msec,void * callback)
{
   if ( context->content!=0 ) { fprintf(stderr,"Context in AmmServer_AddResourceHandler for %s appears to have an already initialized memory part\n",resource_name); }
   memset(context,0,sizeof(struct AmmServer_RH_Context));
   strncpy(context->web_root_path,web_root,MAX_FILE_PATH);
   strncpy(context->resource_name,resource_name,MAX_RESOURCE);
   context->MAX_content_size=allocate_mem_bytes;
   context->prepare_content_callback=callback;

   //TODO : callback_every_x_msec is ignored for now , it should make the query the callback function no sooner than x_msec

   if ( allocate_mem_bytes>0 )
    {
       context->content = (char*) malloc( sizeof(char) * allocate_mem_bytes );
    }
  return AddDirectResourceToCache(context);
}


int AmmServer_Get_GETArg(struct AmmServer_RH_Context * context,char * id,char * value,unsigned int max_value)
{
  return 0;
}

int AmmServer_Get_POSTArg(struct AmmServer_RH_Context * context,char * id,char * value,unsigned int max_value)
{
  return 0;
}

int AmmServer_RemoveResourceHandler(struct AmmServer_RH_Context * context,unsigned char free_mem)
{
  return RemoveDirectResourceToCache(context,free_mem);
}



int AmmServer_GetInfo(unsigned int info_type)
{
  switch (info_type)
   {
     case AMMINF_ACTIVE_CLIENTS : return ACTIVE_CLIENT_THREADS; break;
   };
  return 0;
}


/*
The following calls are not implemented yet

   ||
  \||/
   \/
*/

int AmmServer_GetIntSettingValue(unsigned int set_type)
{
  switch (set_type)
   {
     case AMMSET_PASSWORD_PROTECTION : return PASSWORD_PROTECTION; break;
   };
  return 0;
}

int AmmServer_SetIntSettingValue(unsigned int set_type,int set_value)
{
  switch (set_type)
   {
     case AMMSET_PASSWORD_PROTECTION :  PASSWORD_PROTECTION=set_value; return 1; break;
   };
  return 0;
}


char * AmmServer_GetStrSettingValue(unsigned int set_type)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :    return USERNAME; break;
     case AMMSET_PASSWORD_STR :    return PASSWORD; break;
   };
  return 0;
}

int AmmServer_SetStrSettingValue(unsigned int set_type,char * set_value)
{
  switch (set_type)
   {
     case AMMSET_USERNAME_STR :  AssignStr(&USERNAME,set_value); return SetUsernameAndPassword(USERNAME,PASSWORD); break;
     case AMMSET_PASSWORD_STR :  AssignStr(&PASSWORD,set_value); return SetUsernameAndPassword(USERNAME,PASSWORD); break;
   };
  return 0;
}



int AmmServer_SelfCheck()
{
  fprintf(stderr,"No Checks Implemented in this version , everything should be ok ..\n");
  return 0;
}
