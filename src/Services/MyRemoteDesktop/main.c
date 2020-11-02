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


int DEFAULT_BINDING_PORT = 8090;  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context indexPageContext={0};
struct AmmServer_RH_Context screenContext={0};
struct AmmServer_RH_Context commandContext={0};
struct AmmServer_RH_Context backgroundContext={0};

char backgroundPath[128]="src/Services/MyRemoteDesktop/res/background.jpg";
struct AmmServer_MemoryHandler * backgroundImage=0;

char indexPagePath[128]="src/Services/MyRemoteDesktop/res/remotedesktop.html";
struct AmmServer_MemoryHandler * indexPage=0;
unsigned int indexPageLength=0;

float framesPerSecondRequested = 5.0;
unsigned int allowControl = 0;
unsigned int resolutionX = 3840;
unsigned int resolutionY = 1080;

int webXR=0;

int crop=0;
unsigned int cropX=0,cropY=0,cropWidth=800,cropHeight=600;

int cropImage(
               char * outputImage,
               unsigned int outputImageWidth, unsigned int outputImageHeight,
               char * inputImage,  unsigned int startX,  unsigned int startY,
               unsigned int inputImageWidth,  unsigned int inputImageHeight
             )
{
  char * inputPtr = inputImage + startX *3 + startY * inputImageWidth;
  char * outputPtr = outputImage;
  int x,y;
  for (y=startY; y<startY+outputImageHeight; y++)
  {
   memcpy(outputPtr,inputPtr,outputImageWidth*3);
   outputPtr += outputImageWidth*3;
   inputPtr += inputImageWidth*3;
  }
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_screen_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  fprintf(stderr,"Screen requested\n");
  unsigned int width=resolutionX;
  unsigned int height=resolutionY;
  unsigned char * pixels=(unsigned char *) malloc(sizeof(char)*width*height*3);

  if (pixels!=0)
  {
    #if XWDLIB_BRIDGE
     fprintf(stderr,"Trying to get screen using xwd , this might fail if done with concurrent threads..\n");
     fprintf(stderr,"If screen resolution is more than %ux%u then this might also fail..\n",width,height);
     getScreen(pixels,&width,&height);
    #endif // XWDLIB_BRIDGE

    if (crop)
    {
    unsigned char * cropPixels=(unsigned char *) malloc(sizeof(char)*cropWidth*cropHeight*3);
    if (cropPixels!=0)
    {
     cropImage(
                cropPixels,cropWidth,cropHeight,pixels,cropX,cropY,width,height
              );
    }
     fprintf(stderr,"Encoding cropped ..\n");
     rqst->contentSize=rqst->MAXcontentSize;
     AmmCaptcha_getJPEGFileFromPixels( (char *) cropPixels,cropWidth,cropHeight,3,rqst->content,&rqst->contentSize);
     fprintf(stderr,"Serving cropped ..\n");

    } else
    {
     fprintf(stderr,"Encoding it ..\n");
     rqst->contentSize=rqst->MAXcontentSize;
     AmmCaptcha_getJPEGFileFromPixels( (char *) pixels,width,height,3,rqst->content,&rqst->contentSize);
     fprintf(stderr,"Serving it ..\n");
    }

   free(pixels);
  }
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_background_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  AmmServer_DynamicRequestReturnMemoryHandler(rqst,backgroundImage);
  return 0;
}
//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  AmmServer_DynamicRequestReturnMemoryHandler(rqst,indexPage);
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
  indexPage=AmmServer_ReadFileToMemoryHandler(indexPagePath);
  if (indexPage==0) { AmmServer_Error("Could not find Index Page file %s ",indexPagePath); exit(0); }

  //Update framerate based on our configuration
  char fpsInStr[128]={0};
  unsigned int delayInMilliseconds =  (unsigned int) 1000/framesPerSecondRequested;
  snprintf(fpsInStr,128,"%u",delayInMilliseconds);
  AmmServer_ReplaceAllVarsInMemoryHandler(indexPage,1,"$FRAMERATE$",fpsInStr);


  backgroundImage=AmmServer_ReadFileToMemoryHandler(backgroundPath);
  if (backgroundImage==0) { AmmServer_Error("Could not find Background imagefile %s ",backgroundPath); exit(0); }
  AmmServer_AddResourceHandler(default_server,&backgroundContext,"background.jpg",512000,delayInMilliseconds,&prepare_background_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);


  AmmServer_AddResourceHandler(default_server,&screenContext,"screen.jpg",512000,delayInMilliseconds,&prepare_screen_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&indexPageContext,"/index.html",28096,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
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

  char result[512];
  AmmServer_ExecuteCommandLineNum("xwininfo -root | grep Width | cut -d ':' -f2",result,512,0);
  resolutionX=atoi(result);
  AmmServer_ExecuteCommandLineNum("xwininfo -root | grep Height | cut -d ':' -f2",result,512,0);
  resolutionY=atoi(result);
  fprintf(stderr,"Autodetected resolution  %u x %u\n", resolutionX,resolutionY );



  int i=0;
  for (i=0; i<argc; i++)
  {

    if (strcmp(argv[i],"--webxr")==0)    {
                                          fprintf(stderr,"Enabling WebXR\n");
                                          webXR=1;
                                          snprintf(indexPagePath,128,"src/Services/MyRemoteDesktop/res/remotedesktopXR.html");
                                         }
    if (strcmp(argv[i],"--crop")==0)    {
                                          fprintf(stderr,"Doing a crop of desktop\n");
                                          cropX=atoi(argv[i+1]);
                                          cropY=atoi(argv[i+2]);
                                          cropWidth=atoi(argv[i+3]);
                                          cropHeight=atoi(argv[i+4]);
                                          crop=1;
                                          fprintf(stderr,"to %u,%u -> %u x %u\n",cropX,cropY,cropWidth,cropHeight);
                                        }else
    if (strcmp(argv[i],"--control")==0) {
                                          fprintf(stderr,"Allowing Control\n");
                                           allowControl=1;
                                        } else
    if (strcmp(argv[i],"--fps")==0)     {
                                          fprintf(stderr,"Changing default framerate \n");
                                          framesPerSecondRequested=atof(argv[i+1]);
                                          if (framesPerSecondRequested==0.0) { framesPerSecondRequested=0.1;}
                                          fprintf(stderr,"to %0.2f\n", framesPerSecondRequested );

                                        } else
    if (strcmp(argv[i],"--size")==0)     {
                                          fprintf(stderr,"Changing default frame size\n");
                                          resolutionX=atoi(argv[i+1]);
                                          resolutionY=atoi(argv[i+2]);
                                          fprintf(stderr,"to %u x %u\n", resolutionX,resolutionY );
                                        }else
    if (strcmp(argv[i],"--port")==0)     {
                                          fprintf(stderr,"Changing default port to\n");
                                          DEFAULT_BINDING_PORT=atoi(argv[i+1]);
                                          fprintf(stderr,"to %u \n",DEFAULT_BINDING_PORT );
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
