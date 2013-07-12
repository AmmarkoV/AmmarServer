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


char FileExistsTest(char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 fprintf(stderr,"FileExists(%s) returns 0\n",filename);
 return 0;
}

char EraseFile(char * filename)
{
 FILE *fp = fopen(filename,"w");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 return 0;
}


unsigned int StringIsHTMLSafe(char * str)
{
  unsigned int i=0;
  while(i<strlen(str)) { if ( ( str[i]<'!' ) || ( str[i]=='<' ) || ( str[i]=='>' ) ) { return 0;} ++i; }
  return 1;
}




//This function prepares the content of  form context , ( content )
void * prepare_gps_content_callback(struct AmmServer_DynamicRequest  * rqst)
{

  char latitude[128]={0};
  char longitude[128]={0};
  char message[256]={0};

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
       }
    }

  strcpy(rqst->content,"<html><body>Ack</body></html>");
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
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);


    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;


    if ( argc <1 ) { AmmServer_Warning("Something weird is happening , argument zero should be executable path :S \n"); return 1; } else
    if ( argc <= 2 ) {  } else
     {
        if (strlen(argv[1])>=MAX_IP_STRING_SIZE) { AmmServer_Warning("Console argument for binding IP is too long..!\n"); } else
                                           { strncpy(bindIP,argv[1],MAX_IP_STRING_SIZE); }
        port=atoi(argv[2]);
        if (port>=MAX_BINDING_PORT) { port=DEFAULT_BINDING_PORT; }
     }
   if (argc>=3) { strncpy(webserver_root,argv[3],MAX_FILE_PATH); }
   if (argc>=4) { strncpy(templates_root,argv[4],MAX_FILE_PATH); }

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_Start
        (
           "ammarserver",
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
