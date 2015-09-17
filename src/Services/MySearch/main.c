/*
AmmarServer , simple template executable

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
#include <time.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context indexContext={0};
struct AmmServer_RH_Context searchContext={0};
struct AmmServer_RH_Context logoContext={0};
struct AmmServer_RH_Context faviconContext={0};


void * prepare_logo_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  if (!AmmServer_DynamicRequestReturnFile(rqst,"src/Services/MySearch/res/search.png") ) { AmmServer_Error("Could not return default thumbnail"); }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  if (!AmmServer_DynamicRequestReturnFile(rqst,"src/Services/MySearch/res/search.html") ) { AmmServer_Error("Could not return default thumbnail"); }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

void * favicon_callback(struct AmmServer_DynamicRequest  * rqst)
{
  if (!AmmServer_DynamicRequestReturnFile(rqst,"src/Services/MySearch/res/favicon.ico") )  { AmmServer_Error("Could not return favicon");  }
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * search_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char query[512]={0};
  if ( _GET(default_server,rqst,"q",query,512) )
              {
               snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><head><meta http-equiv=\"refresh\" content=\"5;URL='index.html'\" /></head><body>Search query `%s`</body></html>",query);
              } else
              {
               snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\" /></head><body>Search</body></html>");
              }

  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{

  if (! AmmServer_AddResourceHandler(default_server,&searchContext,"/search.html",webserver_root,4096,0,&search_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&indexContext,"/index.html",webserver_root,4096,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { AmmServer_Warning("Failed adding stats page\n"); }

   if (! AmmServer_AddResourceHandler(default_server,&logoContext,"/search.png",webserver_root,4096,0,&prepare_logo_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) )
     { AmmServer_Warning("Failed adding random testing page\n"); }

   if (! AmmServer_AddResourceHandler(default_server,&faviconContext,"/favicon.ico",webserver_root,4096,0,&favicon_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) )
     { AmmServer_Warning("Failed adding random testing page\n"); }


}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&indexContext,1);
    AmmServer_RemoveResourceHandler(default_server,&logoContext,1);
    AmmServer_RemoveResourceHandler(default_server,&faviconContext,1);
}




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "MySearch",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );


    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    //Create dynamic content allocations and associate context to the correct files
    init_dynamic_content();
    //stats.html and formtest.html should be availiable from now on..!

         while ( (AmmServer_Running(default_server))  )
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             //In other applications the programmer could use the main thread to do anything he likes..
             //The only caveat is that he would takeup more CPU time from the server and that he would have to poll
             //the AmmServer_Running() call once in a while to make sure everything is in order
             //usleep(60000);
             sleep(1);
           }

    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}
