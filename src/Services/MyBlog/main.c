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
#include "database.h"
#include "index.h"

#define TEST_INDEX_GENERATION_ONLY 1
#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context stats={0};




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

  unsigned char*  buf = prepare_index_prototype("src/Services/MyBlog/res/index.html",&myblog);

  if (! AmmServer_AddResourceHandler(default_server,&stats,"/index.html",webserver_root,4096,0,&prepare_index,SAME_PAGE_FOR_ALL_CLIENTS) )
     { AmmServer_Warning("Failed adding index page\n"); }

   if (! AmmServer_AddResourceHandler(default_server,&random_chars,"/random.html",webserver_root,4096,0,&prepare_random_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) )
     { AmmServer_Warning("Failed adding random testing page\n"); }

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);

    destroy_index_prototype();
}




int main(int argc, char *argv[])
{

#if TEST_INDEX_GENERATION_ONLY
  fprintf(stderr,"Testing index generation..\n");
  unsigned char*  buf = prepare_index_prototype("src/Services/MyBlog/res/index.html",&myblog);
  FILE *fp =fopen("test.html","w");
  if (fp!=0)
  {
     fprintf(fp,"%s",buf);
     fclose(fp);
  }
  fprintf(stderr,"Program just generates a test.html file and stops for now , it is not ready yet :) \n");
  exit (0);
#endif // TEST_INDEX_GENERATION_ONLY



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
                                             "MyBlog",
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
