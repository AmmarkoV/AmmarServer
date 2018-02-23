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

#include "framebuffer.h"

struct imageStorage storage[12]={0};



#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="src/Services/WebFramebuffer/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..

char uploads_root[MAX_FILE_PATH]="uploads/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";

unsigned int hits=0;

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;

struct AmmServer_RH_Context indexContext={0};
struct AmmServer_RH_Context updateContext={0};
struct AmmServer_RH_Context uploadContext={0};
struct AmmServer_RH_Context frameContext={0};

void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  snprintf(
    rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html>\
    <head>\
    <script>\
    function refreshFeed()\
    {\
      var randomnumber=Math.floor(Math.random()*100000);\
      document.getElementById(\"vfi\").style.visibility='visible';\
      document.getElementById(\"vfi\").src=\"framebuffer.jpg?t=\"+randomnumber;\
    }\
     setInterval(refreshFeed,100);\
    </script>\
    </head>\
    <body><img  id=\"vfi\" src=\"framebuffer.jpg?t=%u\"></body></html>",
    rand()%1000000,
    rand()%1000000
    );
  rqst->contentSize=strlen(rqst->content);
  return 0;
}



void * prepare_update_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  snprintf(
    rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\
<html>\
 <head>\
  <meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">\
 </head>\
 <body>\
 <form enctype=\"multipart/form-data\" action=\"upload.html\" method=\"POST\">\
  <input name=\"rawresponse\" value=\"NO\" type=\"hidden\">\
  <table>\
   <tr>\
    <td> Name: </td>\
    <td> <input type=\"text\" name=\"name\" value=\"frame\"> </td>\
   </tr>\
   <tr>\
    <td> Width: </td>\
    <td> <input type=\"text\" name=\"width\" value=\"640\"> </td>\
   </tr>\
   <tr>\
    <td> Height: </td>\
    <td> <input type=\"text\" name=\"height\" value=\"480\"> </td>\
   </tr>\
   <tr>\
    <td> Depth: </td>\
    <td> <input type=\"text\" name=\"depth\" value=\"24\"> </td>\
   </tr>\
   <tr>\
    <td> Frame Number: </td>\
    <td> <input type=\"text\" name=\"framenumber\" value=\"0\"> </td>\
   </tr>\
  \
   <tr>\
    <td colspan=2>\
     File to upload:\
     <input name=\"uploadedfile\" type=\"file\">\
     <input value=\"Upload File\" name=\"submit\" type=\"submit\">\
    </td>\
   </tr>\
 </table>\
 </form>\
 </body>\
</html>"
    );
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


void * prepare_upload_content_callback(struct AmmServer_DynamicRequest  * rqst)
{

 /*
-----------------------------83668502413335709751714991384
Content-Disposition: form-data; name="name"

ammar
-----------------------------83668502413335709751714991384
Content-Disposition: form-data; name="width"

640
-----------------------------83668502413335709751714991384
Content-Disposition: form-data; name="height"

480
-----------------------------83668502413335709751714991384
Content-Disposition: form-data; name="depth"

24
-----------------------------83668502413335709751714991384
Content-Disposition: form-data; name="framenumber"

1
-----------------------------83668502413335709751714991384
Content-Disposition: form-data; name="uploadedfile"; filename="67cdbd08fe5021534c4d52a59b046105.jpg"
*/

   unsigned int numberOfPOSTItems =  _POSTNum(rqst);
   fprintf(stderr,"We received %u POST items\n",numberOfPOSTItems);

   //We are looking for the POST form elements with name="Something"
   unsigned int nameSize=0;
   char *  name = _POST(rqst,"name",&nameSize);
   if (name!=0) { fprintf(stderr,"name=%s\n",name);}

   unsigned int widthSize=0;
   char *  width = _POST(rqst,"width",&widthSize);
   if (width!=0) { fprintf(stderr,"width=%s\n",width);}

   unsigned int heightSize=0;
   char *  height = _POST(rqst,"height",&heightSize);
   if (height!=0) { fprintf(stderr,"height=%s\n",height);}

   unsigned int depthSize=0;
   char *  depth = _POST(rqst,"depth",&depthSize);
   if (depth!=0) { fprintf(stderr,"depth=%s\n",depth);}

   unsigned int framenumberSize=0;
   char *  framenumber = _POST(rqst,"framenumber",&framenumberSize);
   if (framenumber!=0) { fprintf(stderr,"framenumber=%s\n",framenumber);}

   unsigned int uploadedfileSize=0;
   char *  uploadedfile = _POST(rqst,"uploadedfile",&uploadedfileSize);

   fprintf(stderr,"We received all pointers\n");


   char uploadedFileUNSANITIZEDPath[513]={0};
   AmmServer_POSTNameOfFile (rqst,0,uploadedFileUNSANITIZEDPath,512);
   AmmServer_Warning("Unsanitized filename is %s \n",uploadedFileUNSANITIZEDPath);

   unsigned int filePointerLength=0;
   char * data = AmmServer_POSTArgGetPointer(rqst,0,&filePointerLength);


   if ( storeImage(storage,0,data,filePointerLength) )
   {
    AmmServer_Success("Pushed new frame..");
    snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><body>Success : %u size</body></html>",filePointerLength);
   } else
   {
    snprintf(rqst->content,rqst->MAXcontentSize,"<!DOCTYPE html>\n<html><body>Failed</body></html>");
   }


  rqst->contentSize=strlen(rqst->content);
  return 0;
}



void * prepare_frame_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  ++hits;

  if ( (storage[0].data==0) || (storage[0].dataSize==0) )
  {
    //No data to serve..
    AmmServer_Warning("Returning default..");
    AmmServer_DynamicRequestReturnFile(rqst,"src/Services/WebFramebuffer/framebuffer.jpg");
    return 0;
  } else
  {
   if (rqst->MAXcontentSize<storage[0].dataSize)
   {
    AmmServer_Warning("Cannot fit..!");
    AmmServer_DynamicRequestReturnFile(rqst,"src/Services/WebFramebuffer/framebuffer.jpg");
   } else
   {
    AmmServer_Success("Returning new frame..");
    memcpy(rqst->content,storage[0].data,storage[0].dataSize);
    rqst->contentSize=storage[0].dataSize;
   }
  }


  return 0;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddResourceHandler(default_server,&indexContext ,"/index.html" ,4096,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&updateContext,"/update.html",4096,0,&prepare_update_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&uploadContext,"/upload.html",4096,0,&prepare_upload_content_callback,SAME_PAGE_FOR_ALL_CLIENTS|ENABLE_RECEIVING_FILES);
  AmmServer_DoNOTCacheResourceHandler(default_server,&uploadContext);

  AmmServer_AddResourceHandler(default_server,&frameContext ,"/framebuffer.jpg",1128000,0,&prepare_frame_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&indexContext,1);
    AmmServer_RemoveResourceHandler(default_server,&frameContext,1);
}




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
                                             "webframebuffer",
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
