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

#define MAX_BINDING_PORT 65534


#define DEFAULT_BINDING_PORT 8081  // <--- Change this to 80 if you want to bind to the default http port..!
#define ADMIN_BINDING_PORT 8082
#define ENABLE_ADMIN_PAGE 0

//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented out or removed in stable release..
char admin_root[MAX_FILE_PATH]="admin_html/"; // <- change this to the directory that contains your content if you dont want to use the default admin_html dir..
char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";



//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context gps={0};


int appendGPSMessage(char * filename , char  * from , char * message , char * latitude , char * longitude)
{
    FILE * fp = fopen(filename,"a+");
    if (fp!=0)
    {
        fprintf(fp,"gps(%s,%s,%s,%s)\n",from,latitude,longitude,message);
        fclose(fp);
        return 1;
    }
  return 0;
}


//This function prepares the content of  form context , ( content )
void * prepare_gps_content_callback(struct AmmServer_DynamicRequest  * rqst)
{

  char latitude[128]={0};
  char longitude[128]={0};
  char message[256]={0};
  char from[256]={0};

 AmmServer_Warning("New GPS message");
 if ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         if ( _GET(default_server,rqst,"lat",latitude,128) )
             {
               fprintf(stderr,"Latitude : %s \n",latitude);
             }
         if ( _GET(default_server,rqst,"lon",longitude,128) )
             {
               fprintf(stderr,"Longitude : %s \n",longitude);
             }
         if ( _GET(default_server,rqst,"msg",message,256) )
             {
               fprintf(stderr,"Message : %s \n",message);
             }
         if ( _GET(default_server,rqst,"from",from,256) )
             {
               fprintf(stderr,"From : %s \n",from);
             }
         if (!appendGPSMessage("gps.log", (char  *) from , (char *) message , (char *) latitude , (char *) longitude))
         {
            AmmServer_Error("Could not log new GPS message received");
         }
       }
    }

  strncpy(rqst->content,"<html><body>Ack</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  form context , ( content )
void * request_override_callback(char * content)
{
  // char requestHeader;
  // struct HTTPHeader * request;
  // void * request_override_callback;

  //This does nothing for now :P
  return 0;
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  if (! AmmServer_AddResourceHandler(default_server,&gps,"/gps.html",webserver_root,4096,0,&prepare_gps_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding gps testing page\n"); }

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&gps,1);
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
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;


    default_server = AmmServer_StartWithArgs(
                                             "geoposshare",
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
