/*
AmmarServer , main executable

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

#include "state.h"
#include "thread.h"
#include "board.h"
#include "postReceiver.h"

#define MAX_BINDING_PORT 65534

#define DEFAULT_BINDING_PORT 8082  // <--- Change this to 80 if you want to bind to the default http port..!
#define ADMIN_BINDING_PORT 8082

#define WEBSERVERROOT "public_html/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

#define MAX_SCRIPT_RESPONSE_SIZE 40960

struct AmmServer_RH_Context boardIndexView={0};
struct AmmServer_RH_Context threadIndexView={0};
struct AmmServer_RH_Context threadView={0};
struct AmmServer_RH_Context postReceiver={0};


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(default_server,&boardIndexView,"/index.html",webserver_root,4096,0,&prepareBoardIndexView,SAME_PAGE_FOR_ALL_CLIENTS) )
        { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&threadIndexView,"/threadIndexView.html",webserver_root,4096,0,&prepareThreadIndexView,SAME_PAGE_FOR_ALL_CLIENTS) )
        { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&threadView,"/threadView.html",webserver_root,4096,0,&prepareThreadView,SAME_PAGE_FOR_ALL_CLIENTS) )
        { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&postReceiver,"/postReceiver.html",webserver_root,4096,0,&processPostReceiver,SAME_PAGE_FOR_ALL_CLIENTS) )
        { AmmServer_Warning("Failed adding stats page\n"); }


  loadSite("data/settings.ini");
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    unloadSite();

    AmmServer_RemoveResourceHandler(default_server,&boardIndexView,1);
    AmmServer_RemoveResourceHandler(default_server,&threadIndexView,1);
    AmmServer_RemoveResourceHandler(default_server,&threadView,1);
    AmmServer_RemoveResourceHandler(default_server,&postReceiver,1);
}




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
   //If we have a command line arguments we overwrite our buffers

    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    //AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;


    default_server = AmmServer_StartWithArgs(
                                             "habchan",
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
