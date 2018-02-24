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
#include "xwd-1.0.5/XwdLib.h"
#include "../../AmmCaptcha/AmmCaptcha.h"

#define XWDLIB_BRIDGE 1
#define ALLOW_REMOTE_CONTROL 1


#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context indexPageContext={0};
struct AmmServer_RH_Context screenContext={0};
struct AmmServer_RH_Context commandContext={0};

char indexPagePath[128]="src/Services/MyRemoteDesktop/res/remotedesktop.html";
char * indexPage=0;
unsigned int indexPageLength=0;

unsigned int allowControl=0;


//This function prepares the content of  stats context , ( stats.content )
void * prepare_screen_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  fprintf(stderr,"Screen requested\n");
  unsigned char * pixels=(unsigned char *) malloc(sizeof(char)*1920*1080*3);
  unsigned int width=1920;
  unsigned int height=1080;

  if (pixels!=0)
  {
    #if XWDLIB_BRIDGE
     fprintf(stderr,"Trying to get screen using xwd , this might fail if done with concurrent threads..\n");
     getScreen(pixels,&width,&height);
    #endif // XWDLIB_BRIDGE

    fprintf(stderr,"Encoding it ..\n");
    rqst->contentSize=rqst->MAXcontentSize;
    AmmCaptcha_getJPEGFileFromPixels( (char *) pixels,width,height,3,rqst->content,&rqst->contentSize);
    fprintf(stderr,"Serving it ..\n");
   free(pixels);
  }
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,indexPage,indexPageLength);
  rqst->content[indexPageLength]=0;
  rqst->contentSize=indexPageLength;
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_command_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
 if (!allowControl)
 {
   strcpy(rqst->content,"<html>Disabled</html>");
   rqst->contentSize=strlen(rqst->content);
   return 0;
 }

 #if ALLOW_REMOTE_CONTROL
 unsigned int donothing=0;
 unsigned int x=0,y=0,doclick=0,do2xclick=0,dokey=0;
 char xCoord[128]={0};
 char yCoord[128]={0};
 char commandStr[128]={0};
  if ( _GETcpy(rqst,"x",xCoord,128) )  { x=atoi(xCoord); }
  if ( _GETcpy(rqst,"y",yCoord,128) )  { y=atoi(yCoord); }
  if ( _GETcpy(rqst,"k",commandStr,128) )  { fprintf(stderr,"Keystroke %s",commandStr);
                                                         dokey=atoi(commandStr); }
  if ( _GETcpy(rqst,"click",commandStr,128) )  { doclick=1;  }
  if ( _GETcpy(rqst,"dblclick",commandStr,128) )  { do2xclick=1;  }


 if ( (x>5) || (y>5) )
 {
  fprintf(stderr,"---------------------- MouseMovement \n");
  snprintf(commandStr,128,"xdotool mousemove --sync %u %u  ",x,y);
 } else
 if (dokey)
 {
  char keypressedFiltered[32]={0};
  char keyPressed = (char) dokey;
  if (keyPressed<200)
  {
   fprintf(stderr,"---------------------- Key (%c) \n",keyPressed);

   snprintf(keypressedFiltered,32,"%c",keyPressed);
   if (keyPressed==' ') {  snprintf(keypressedFiltered,32,"space"); } else
   if (keyPressed==' ') {  snprintf(keypressedFiltered,10,"Return"); } else
   if (keyPressed==' ') {  snprintf(keypressedFiltered,13,"Return"); }
   snprintf(commandStr,128,"xdotool key '%s'",keypressedFiltered);
  } else
  {
   donothing=1;
  }
 } else
 if (do2xclick)
 {
  fprintf(stderr,"---------------------- 2x Click \n");
  snprintf(commandStr,128,"xdotool click 1");
  int i=system(commandStr);
   if (i!=0) { AmmServer_Error("Could not execute %s\n",commandStr); }
  usleep(1000);
 } else
 if (doclick)
 {
  fprintf(stderr,"---------------------- Click \n");
  snprintf(commandStr,128,"xdotool click 1");
 } else
 {
  fprintf(stderr,"---------------------- Nothing \n");
  donothing=1;
 }

 if (!donothing)
 {
   int i=system(commandStr);
   if (i!=0) { AmmServer_Error("Could not execute %s\n",commandStr); }
 }

  strcpy(rqst->content,"<html>Ok</html>");
  rqst->contentSize=strlen(rqst->content);
  #else
   strcpy(rqst->content,"<html>Remote Control Disabled</html>");
   rqst->contentSize=strlen(rqst->content);
  #endif

  return 0;
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  indexPage=AmmServer_ReadFileToMemory(indexPagePath,&indexPageLength);
  if (indexPage==0) { AmmServer_Error("Could not find Index Page file %s ",indexPagePath); exit(0); }

  AmmServer_AddResourceHandler(default_server,&screenContext,"screen.jpg",512000,400,&prepare_screen_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&indexPageContext,"/index.html",4096,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&commandContext,"/cmd",4096,0,&prepare_command_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&screenContext,1);
    AmmServer_RemoveResourceHandler(default_server,&indexPageContext,1);
    AmmServer_RemoveResourceHandler(default_server,&commandContext,1);
}
/*! Dynamic content code ..! END ------------------------*/




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    #if XWDLIB_BRIDGE
       initXwdLib(argc,argv);
    #endif // XWDLIB_BRIDGE



  int i=0;
  for (i=0; i<argc; i++)
  {
    if (strcmp(argv[i],"--control")==0) {
                                          fprintf(stderr,"Allowing Control\n");
                                           allowControl=1;
                                        }
  }

    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "MyRemoteDesktop",
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

    #if XWDLIB_BRIDGE
       closeXwdLib();
    #endif // XWDLIB_BRIDGE


    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}
