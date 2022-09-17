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

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define DYNAMIC_PAGES_MEMORY_COMMITED 4096
#define MAX_QUERY_SIZE 4096
#define MAX_IMAGES_CONCURRENTLY 5

#define IMAGE_DIRECTORY "workspace/outputs/txt2img-samples/samples/"
#define WEBSERVERROOT "public_html/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//------------------------------------------------
struct AmmServer_RH_Context indexPageContext= {0};
struct AmmServer_RH_Context imageContext= {0};
struct AmmServer_RH_Context form= {0};
//------------------------------------------------
struct AmmServer_MemoryHandler * indexPage=0;
struct AmmServer_MemoryHandler * loadingImage=0;
struct AmmServer_MemoryHandler * imageFile[MAX_IMAGES_CONCURRENTLY]={0};
//------------------------------------------------
struct AmmServer_Instance  * default_server=0;


void * prepare_image_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
    if (_GETexists(rqst,"i"))
    {
        int imageChannel = _GETuint(rqst,"i");
        if ( (imageChannel>=0) && (imageChannel<MAX_IMAGES_CONCURRENTLY) )
        {
         char filename[512]={0};
         snprintf(filename,512,"%s/%05u.png",IMAGE_DIRECTORY,imageChannel);
         imageFile[imageChannel] = AmmServer_ReadFileToMemoryHandler(filename);
         AmmServer_DynamicRequestReturnMemoryHandler(rqst,imageFile[imageChannel]);
         return 0;
        }
    }

    AmmServer_DynamicRequestReturnMemoryHandler(rqst,loadingImage);
    return 0;
}

void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
    AmmServer_DynamicRequestReturnMemoryHandler(rqst,indexPage);
    return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * generateImagesBasedOnQuery(struct AmmServer_DynamicRequest  * rqst)
{
    //The url , to Long , Short eetc conventions are shit.. :P I should really make them better :p
    memset(rqst->content,0,DYNAMIC_PAGES_MEMORY_COMMITED);

    if  (_GETexists(rqst,"query"))
    {
        char query[MAX_QUERY_SIZE]= {0};
        if ( _GETcpy(rqst,"query",query,MAX_QUERY_SIZE) )
        {
            fprintf(stderr,"\n\n\n\n\n\nQUERY : %s \n\n\n\n\n\n\n",query);

            char fullCommand[MAX_QUERY_SIZE+1024]={0};
            snprintf(fullCommand,MAX_QUERY_SIZE+1024,"~/workspace/tex2img.sh \"%s\"",query);
            int i=system(fullCommand);
            fprintf("Executed : %s \n",fullCommand);
            fprintf("Response : %u \n",i);
        }
    }

    strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\"></head><body>Refresh</body></html>",rqst->MAXcontentSize);

    rqst->contentSize=strlen(rqst->content);
    return 0;
}


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
    //--------------------------------------------------------------------------------------------------------------------------------------------
    AmmServer_AddResourceHandler(default_server,&indexPageContext,"/index.html",4096,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
    AmmServer_AddResourceHandler(default_server,&form,"/go",4096,0,&generateImagesBasedOnQuery,SAME_PAGE_FOR_ALL_CLIENTS);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    AmmServer_AddResourceHandler(default_server,&imageContext,"/image.png",1024000,0,&prepare_image_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    indexPage    = AmmServer_ReadFileToMemoryHandler("src/Services/ImageGeneration/generation.html");
    loadingImage = AmmServer_ReadFileToMemoryHandler("src/Services/ImageGeneration/1.png");
    if (indexPage==0)
    {
        AmmServer_Error("Could not find Index Page file");
        exit(0);
    }
    //--------------------------------------------------------------------------------------------------------------------------------------------

    for (int i=0; i<MAX_IMAGES_CONCURRENTLY; i++)
    {
        char filename[512]={0};
        snprintf(filename,512,"src/Services/ImageGeneration/%u.png",i);
        imageFile[i] = AmmServer_ReadFileToMemoryHandler(filename);
    }

    //fresh.txt will always be served fresh
    AmmServer_DoNOTCacheResource(default_server,"image.png");
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&form,1);
}
/*! Dynamic content code ..! END ------------------------*/





int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //If we have a command line arguments we overwrite our buffers

    unsigned int i=0;
    for (i=0; i<argc; i++)
    {
        if ((strcmp(argv[i],"--root")==0)&&(argc>i+1))
        {
            snprintf(webserver_root,MAX_FILE_PATH,"%s",argv[i+1]);
        }
    }

    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;


    default_server = AmmServer_StartWithArgs(
                         "imagegeneration",
                         argc,argv,  //The internal server will use the arguments to change settings
                         //If you don't want this look at the AmmServer_Start call
                         bindIP,
                         port,
                         0, /*This means we don't want a specific configuration file*/
                         webserver_root,
                         templates_root
                     );

    if (!default_server)
    {
        AmmServer_Error("Could not start server , shutting down everything..");
        exit(1);
    }


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
