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

#include "serialIO.h"
#include "state.h"

#define MAX_BINDING_PORT 65534


#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define ADMIN_BINDING_PORT 8082
#define ENABLE_ADMIN_PAGE 0

#define WEBSERVERROOT "src/Services/AmmBus/public_html/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT;
char templates_root[MAX_FILE_PATH]=WEBSERVERROOT;
int deviceID = 0;


char deviceSerialFilePath[MAX_FILE_PATH]="/dev/ttyACM0";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;

struct AmmServer_RH_Context cmdRH={0};



//This function prepares the content of  form context , ( content )
void * callback_command(struct AmmServer_DynamicRequest  * rqst)
{

  char dev[128]={0};   int haveDev=0;
  int state=0;
  char time[256]={0};  int haveTime=0;

 AmmServer_Warning("New Command message");



         if ( _GETcpy(rqst,"activate",dev,128) )
             {
               haveDev=1;
               state=1;
               fprintf(stderr,"Device : %s , State : %u \n",dev,state);
             }

         if ( _GETcpy(rqst,"deactivate",dev,128) )
             {
               haveDev=1;
               state=0;
               fprintf(stderr,"Device : %s , State : %u \n",dev,state);
             }


  if ( (haveDev)  )
  {
    if (strcmp(dev,"all")==0)
    {
     if (!setAmmBusStateAll(deviceID,state))
     {
       AmmServer_Warning("AmmBus: Failed to set a state for all devices");
     }
    } else
    if (!setAmmBusState(deviceID,dev,state))
    {
      AmmServer_Warning("AmmBus: Incorrect device to set a state for (%s)",dev);
    }
  }


  strncpy(rqst->content,"<html><body>Ack</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}




//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddResourceHandler(default_server,&cmdRH,"/commander.html",4096,0,&callback_command,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&cmdRH,1);
}
/*! Dynamic content code ..! END ------------------------*/




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
   //If we have a command line arguments we overwrite our buffers

   unsigned int i=0;
   for (i=0; i<argc; i++)
   {
     if (strstr(argv[i],"/dev/")!=0)
     {
       snprintf(deviceSerialFilePath,126,"%s",argv[i]);
     } else
     if (strcmp(argv[i],"--config")==0)
     {
       fprintf(stderr,"Config file : %s \n",argv[i+1]);
     }
   }
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;


    deviceID = serialport_init(deviceSerialFilePath,9600);

    default_server = AmmServer_StartWithArgs(
                                             "ammbus",
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


    serialport_close(deviceID);
    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");

    return 0;

return 0;
}
