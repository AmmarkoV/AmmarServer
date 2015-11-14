/*
AmmarServer , simple template executable

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2015

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



//AmmarServerStuff
char webserver_root[MAX_FILE_PATH]="/home/nao/public"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="/home/nao/public";
unsigned int port=8080;
char bindIP[MAX_IP_STRING_SIZE]={"0.0.0.0"};
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RH_Context control={0};
struct AmmServer_RH_Context termination={0};

int externalCommand=0;


//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&control,1);
    AmmServer_RemoveResourceHandler(default_server,&termination,1);
}

int closeAmmarServer()
{
    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 1;
}

//This function prepares the content of  stats context , ( stats.content )
void * terminate_callback(struct AmmServer_DynamicRequest  * rqst)
{
     fprintf(stderr,"TERMINATE Called\n");
     closeAmmarServer();
    exit(0);
}

//This function prepares the content of  stats context , ( stats.content )
void * control_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char movementRequested[128]={0};
  if ( _GET(default_server,rqst,"walk",movementRequested,128) )
  {
     fprintf(stderr,"Start Called\n");
     externalCommand=1;
  } else
  if ( _GET(default_server,rqst,"stop",movementRequested,128) )
  {
     fprintf(stderr,"Stop Called\n");
     externalCommand=2;
  } else
  if ( _GET(default_server,rqst,"jump",movementRequested,128) )
  {
    externalCommand=3;
 } else
  {
     fprintf(stderr,"Incorrect Command\n");
  }


  snprintf(rqst->content,rqst->MAXcontentSize,"<html><body><a href=\"control.html?walk=1\">Walk</a><br>\
                                               <a href=\"control.html?stop=1\">Stop</a>\
                                               <a href=\"control.html?jump=1\">Jump</a> </body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(default_server,&control,"/control.html",webserver_root,4096,0,(void*) &control_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&termination,"/terminate.html",webserver_root,4096,0,(void*) &terminate_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { AmmServer_Warning("Failed adding stats page\n"); }


}


int initAmmarServer()
{
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal((void*) &closeAmmarServer);
    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "SimpleTemplate",
                                              0,0 , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );


    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    init_dynamic_content();
    return 1;
 }




int main(int argc, char *argv[])
{
    initAmmarServer();
         while ( (AmmServer_Running(default_server))  )
           {
             //Do work here on any threads etc
             sleep(1);
           }
    closeAmmarServer();

    return 0;
}
