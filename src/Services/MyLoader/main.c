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

#define DEFAULT_BINDING_PORT 8085

char webserver_root[MAX_FILE_PATH]="src/Services/MyLoader/res/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};
struct AmmServer_RH_Context uploadProcessor={0};
struct AmmServer_RH_Context testProcessor={0};
struct AmmServer_RH_Context indexProcessor={0};

struct AmmServer_RH_Context stats={0};

struct AmmServer_MemoryHandler * indexPage=0;


//This function prepares the content of  stats context , ( stats.content )
void * prepare_index_callback(struct AmmServer_DynamicRequest  * rqst)
{
  struct AmmServer_MemoryHandler * videoMH = AmmServer_CopyMemoryHandler(indexPage);


  unsigned int linkID=1+rand()%6;

  char bannerLink[28];
  snprintf(bannerLink,28,"banner_%u",linkID);

  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$BANNERLINK$",bannerLink);
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$TOTAL_SHARED_DATA$", "0");
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$TOTAL_UPLOAD_SIZE$", "0");

  memcpy (rqst->content , videoMH->content , videoMH->contentCurrentLength );
  rqst->contentSize=videoMH->contentCurrentLength ;

  AmmServer_FreeMemoryHandler(&videoMH);
  return 0;
}

//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

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

//This function could alter the content of the URI requested and then return 1
void request_override_callback(void * request)
{
  struct AmmServer_RequestOverride_Context * rqstContext = (struct AmmServer_RequestOverride_Context *) request;
  struct HTTPHeader * rqst = rqstContext->request;
  AmmServer_Warning("With URI : %s \n Filtered URI : %s \n GET Request : %s \n",rqst->resource,rqst->verified_local_resource, rqst->GETquery);

  if (strcmp("/favicon.ico",rqst->resource)==0 ) { return; /*Client requested favicon.ico , no resolving to do */ } else
  if (strcmp("/error.html",rqst->resource)==0 )  { return; /*Client requested error.html , no resolving to do */  } else
  if (strcmp("/upload.html",rqst->resource)==0 )  { return; /*Client requested error.html , no resolving to do */  } else
  if (strcmp("/",rqst->resource)==0 ) {  return; /*Client requested index.html , no resolving to do */  } else
  if (strcmp("/random.html",rqst->resource)==0 )  { return; /*Client requested index.html , no resolving to do */  }

  return;
}



//This function prepares the content of  stats context , ( stats.content )
void * processUploadCallback(struct AmmServer_DynamicRequest  * rqst)
{
  //AmmServer_WriteFileFromMemory("test.bin",rqst->POST_request,rqst->POST_request_length);
  AmmServer_POSTArgToFile (default_server,rqst,0,"test.bin");
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  snprintf(rqst->content,rqst->MAXcontentSize,"<html>\
                           <head>\
                             <title>Dynamic Content Enabled</title>\
                           </head>\
                           <body>Uploaded test.bin<br>\
                           </body></html>");


  rqst->contentSize=strlen(rqst->content);
  return 0;
}



//This function prepares the content of  stats context , ( stats.content )
void * internalTestCallback(struct AmmServer_DynamicRequest  * rqst)
{
  //char command[512]={"curl \"http://127.0.0.1:8085/upload.html\" -F myfile=@\"public_html/image.png\" "};

  AmmServer_WriteFileFromMemory("test.jpg",rqst->POST_request,rqst->POST_request_length);
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  snprintf(rqst->content,rqst->MAXcontentSize,"<html>\
                           <head>\
                             <title>Dynamic Content Enabled</title>\
                           </head>\
                           <body>Uploaded<br>\
                           </body></html>");


  rqst->contentSize=strlen(rqst->content);
  return 0;
}




//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_SetIntSettingValue(default_server,AMMSET_MAX_POST_TRANSACTION_SIZE,32/*MB*/*1024*1024);
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  if (! AmmServer_AddResourceHandler(default_server,&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&uploadProcessor,"/upload.html",webserver_root,4096,0,&processUploadCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES) )
     { AmmServer_Warning("Failed adding upload processor page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&testProcessor,"/test.html",webserver_root,4096,0,&internalTestCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT) )
     { AmmServer_Warning("Failed adding upload processor page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&indexProcessor,"/index.html",webserver_root,4096,0,&prepare_index_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) )
     { AmmServer_Warning("Failed adding upload processor page\n"); }


  fprintf(stderr,"Reading master index file..  ");
  indexPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyLoader/res/index.html");
  fprintf(stderr,"current length %u , size is %u \n",indexPage->contentCurrentLength , indexPage->contentSize);

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);
    AmmServer_RemoveResourceHandler(default_server,&uploadProcessor,1);
    AmmServer_RemoveResourceHandler(default_server,&testProcessor,1);
    AmmServer_FreeMemoryHandler(&indexPage);
}




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

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "myloader",
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
