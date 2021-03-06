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
#include "../../UserAccounts/userAccounts.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="src/Services/UserAccountsTester/res/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context loginProcess={0};
struct AmmServer_RH_Context loginCheck={0};


//This function prepares the content of  stats context , ( stats.content )
void * do_login_check_callback(struct AmmServer_DynamicRequest  * rqst)
{

  AmmServer_DynamicRequestReturnFile(rqst,"src/Services/UserAccountsTester/res/login.html");

  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * do_login_process_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char username[256];
  char password[256];

  if ( (_GETcpy(rqst ,(char*) "pass" , password , 256) ) && ( _GETcpy(rqst ,(char*) "user" , username , 256) ) )
   {
      strncpy(rqst->content,"<html><head><title>Random Number Generator</title></head><body>",rqst->MAXcontentSize);
      char msg[256];
      snprintf(msg,256," Username submitted is %s , Password submitted is %s \n",username,password);
      strcat(rqst->content,msg);
   } else
   {
     strncpy(rqst->content,"<html><head><title>Random Number Generator</title><meta http-equiv=\"refresh\" content=\"0 url=index.html\"></head><body>",rqst->MAXcontentSize);

   }
  strcat(rqst->content,"</body></html>");

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function could alter the content of the URI requested and then return 1
void request_override_callback(void * request)
{
  //struct AmmServer_RequestOverride_Context * rqstContext = (struct AmmServer_RequestOverride_Context *) request;
  return;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  AmmServer_AddResourceHandler(default_server,&loginCheck,"/index.html",4096,0,&do_login_check_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&loginProcess,"/performlogin.html",4096,0,&do_login_process_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&loginCheck,1);
    AmmServer_RemoveResourceHandler(default_server,&loginProcess,1);
}
/*! Dynamic content code ..! END ------------------------*/




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
                                             "UserAccountsTester",
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
