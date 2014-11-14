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

char webserver_root[MAX_FILE_PATH]="public_html/cinemaPilot/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


/*! Dynamic content code ..! START!*/
/* A few words about dynamic content here..
   This is actually one of the key features on AmmarServer and maybe the reason that I started the whole project
   What I am trying to do here is serve content by directly linking the webserver to binary ( NOT Interpreted ) code
   in order to serve pages with the maximum possible efficiency and skipping all intermediate layers..

   PHP , Ruby , Python and all other "web-languages" are very nice and handy and to be honest I can do most of my work fine using PHP , MySQL and Apache
   However setting up , configuring and maintaining large projects with different database systems , separate configuration files for each of the sub parts
   and re deploying everything is a very tiresome affair.. Not to mention that despite the great work done by the apache  , PHP etc teams performance is wasted
   due to the interpreters of the various scripting languages used..

   Things can't get any faster than AmmarServer and the whole programming interface exposed to the programmer is ( imho ) very friendly and familiar to even inexperienced
   C developer..

   What follows is the decleration of some "Dynamic Content Resources" their Constructors/Destructors and their callback routines that fill them with the content to be served
   each time a client requests one of the pages..
*/

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};


struct AmmServer_RH_Context indexPage={0};
struct AmmServer_RH_Context remoteControl={0};
struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context stats={0};


//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  fprintf(stderr,"Trying to write dynamic request to %p , max size %lu \n",rqst->content , rqst->MAXcontentSize);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><title>Dynamic Content Enabled</title><meta http-equiv=\"refresh\" content=\"1\"></head>\
            <body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>\
            The string you see is updated dynamically every time you get a fresh copy of this file!<br><br>\n\
            To include your own content see the <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/main.c#L46\">\
            Dynamic content code label in ammarserver main.c</a><br>\n\
            If you dont need dynamic content at all consider disabling it from ammServ.conf or by setting DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=0; in \
            <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/file_caching.c\">file_caching.c</a> and recompiling.!</body></html>",
           tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of form context , ( content )
void * prepare_remoteControl_callback(struct AmmServer_DynamicRequest * rqst)
{
 char data[256]={0};
 AmmServer_Warning("New Control");
 if ( rqst->GET_request != 0 )
 {
   if ( strlen(rqst->GET_request)>0 )
    {
      if ( _GET(default_server,rqst,"play",data,128) )
       {
         AmmServer_Warning("Play pressed \n");
       }
      if ( _GET(default_server,rqst,"pause",data,128) )
       {
         AmmServer_Warning("Pause pressed\n");
       }
      if ( _GET(default_server,rqst,"previous",data,128) )
       {
         AmmServer_Warning("Previous\n");
       }
      if ( _GET(default_server,rqst,"next",data,128) )
       {
         AmmServer_Warning("Next\n");
       }
    }
 }
  strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"0; url=control.html\" ></head><body>Ack</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
return 0;
}

//This function prepares the content of  form context , ( content )
void * prepare_indexPage(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"0; url=cinema.html\"></head><body><a href=\"cinema.html\">Accessing</a></body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_random_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  strncpy(rqst->content,"<html><head><title>Random Number Generator</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>",rqst->MAXcontentSize);

  char hex[16+1]={0};
  unsigned int i=0;
  for (i=0; i<1024; i++)
    {
        snprintf(hex,16, "%x ", rand()%256 );
        strcat(rqst->content,hex);
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

  AmmServer_AddResourceHandler(default_server,&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&random_chars,"/random.html",webserver_root,4096,0,&prepare_random_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);

  AmmServer_AddResourceHandler(default_server,&remoteControl,"/remoteControl.html",webserver_root,4096,0,&prepare_remoteControl_callback,SAME_PAGE_FOR_ALL_CLIENTS);



  AmmServer_AddResourceHandler(default_server,&indexPage,"/index.html",webserver_root,4096,0,&prepare_indexPage,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);
    AmmServer_RemoveResourceHandler(default_server,&random_chars,1);
    AmmServer_RemoveResourceHandler(default_server,&indexPage,1);
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
                                             "CinemaPilot",
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
