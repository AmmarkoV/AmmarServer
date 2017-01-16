/*
V4L2ToHTTP , main executable

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
#include <pthread.h>
#include <signal.h>
#include "../../AmmServerlib/AmmServerlib.h"
#include "../../AmmCaptcha/AmmCaptcha.h"

#include "v4l2_acquisition_shared_library/V4L2Acquisition.h"

#define MAX_BINDING_PORT 65534
#define DEFAULT_BINDING_PORT 8081
#define MAX_INPUT_IP 256

#define ENABLE_PASSWORD_PROTECTION 1
#define SIMPLE_INDEX 1
#define WEB_PAGE_UPDATE_EVERY_MS 1000

unsigned int VIDEO_WIDTH=640 ,  jpg_width=640;
unsigned int VIDEO_HEIGHT=480 ,  jpg_height=480;
unsigned int FRAMERATE=10;

char webcam[MAX_FILE_PATH]="/dev/video0";
char webserver_root[MAX_FILE_PATH]="public_html/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";

pthread_mutex_t refresh_jpeg_lock;

struct AmmServer_Instance * v4l2_server=0;
struct AmmServer_RH_Context index_page= {0};
struct AmmServer_RH_Context jpeg_picture= {0};


void * prepare_index_page_callback(struct AmmServer_DynamicRequest * rqst)
{
  if (SIMPLE_INDEX)
    {
      strcpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"1\"><title>V4L2ToHTTP</title></head><body><br><br><center><img src=\"cam.jpg\"><br><h3><a href=\"index.html\">-- Manually Reload Page --</a></h3></center></body></html>");
    }
  else
    {
      strcpy(rqst->content,"<html>\n");
      strcat(rqst->content,"    <head>\n");
      strcat(rqst->content,"         <title>V4L2ToHTTP</title>\n");
      strcat(rqst->content,"         <script language=\"JavaScript\"><!--\n");

      strcat(rqst->content,"                var refreshDelayMilliSecs = ");

      char webupstr[128]={0};
      sprintf(webupstr,"%u\n",WEB_PAGE_UPDATE_EVERY_MS);
      strcat(rqst->content,webupstr);

      strcat(rqst->content,"                var newImage = new Image();\n");
      strcat(rqst->content,"                var number = 0;\n");
      strcat(rqst->content,"                newImage.src = \"cam.jpg?i=0\";\n");
      strcat(rqst->content,"                 function updateImage()\n");
      strcat(rqst->content,"                    {\n");
      strcat(rqst->content,"                           if(newImage.complete)\n");
      strcat(rqst->content,"                           {\n");
      strcat(rqst->content,"                                document.getElementById(\"LiveImage\").src = newImage.src;\n");
      strcat(rqst->content,"                                if(newImage.complete)\n");
      strcat(rqst->content,"                                 {\n");
      strcat(rqst->content,"                                      document.getElementById(\"LiveImage\").src = newImage.src;\n");
      strcat(rqst->content,"                                      newImage = new Image();\n");
      strcat(rqst->content,"                                      number++;\n");
      strcat(rqst->content,"                                     newImage.src = \"cam.jpg?i=\"+number\n"); //Later on make it cam.jpg?inc=\" + number;
      strcat(rqst->content,"                                   }\n");
      strcat(rqst->content,"                        }\n                     setTimeout('updateImage()',refreshDelayMilliSecs);\n");
      strcat(rqst->content,"                   }\n");
      strcat(rqst->content,"               //--></script>\n");
      strcat(rqst->content,"</head>\n");
      strcat(rqst->content,"<body  onLoad=\"setTimeout('updateImage()',refreshDelayMilliSecs)\">\n");
      strcat(rqst->content,"<br><br><center><img src=\"cam.jpg?i=0\" id=\"LiveImage\"><br>");
      strcat(rqst->content,"<h5>Active Clients : ");

      if (AmmServer_GetInfo(v4l2_server,AMMINF_ACTIVE_CLIENTS) < 10000 )
           {
             char viewers_str[150]={0};
             sprintf(viewers_str,"%u",AmmServer_GetInfo(v4l2_server,AMMINF_ACTIVE_CLIENTS));
             strcat(rqst->content,viewers_str);
           } else
           {
               strcat(rqst->content,"N/A");
           }

      strcat(rqst->content,"<br>\n");
      strcat(rqst->content,"<br><a href=\"index.html\">-- Manually Reload Page --</a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n");
      strcat(rqst->content,"<a href=\"https://github.com/AmmarkoV/V4L2ToHTTP\">-- Get V4L2ToHTTP --</a></h5></center></body></html>\n");
    }

  rqst->MAXcontentSize=strlen((char *)rqst->content);
  rqst->contentSize=rqst->MAXcontentSize;
  return 0;
}


void * prepare_camera_data_callback(struct AmmServer_DynamicRequest * rqst)
{

  pthread_mutex_lock (&refresh_jpeg_lock); // LOCK PROTECTED OPERATION -------------------------------------------

  fprintf(stderr,"Calling Camera callback \n");
  AmmCaptcha_getJPEGFileFromPixels(
                                    getV4L2ColorPixels(0),
                                    getV4L2ColorWidth(0),
                                    getV4L2ColorHeight(0),
                                    getV4L2ColorChannels(0),
                                    rqst->content,
                                    &rqst->contentSize
                                   );


  fprintf(stderr,"Calling Camera callback success new picture ( %u bytes long ) ready !\n",(unsigned int) rqst->contentSize);
  pthread_mutex_unlock (&refresh_jpeg_lock); // LOCK PROTECTED OPERATION -------------------------------------------
  return 0;
}



int open_camera(char * webcam_dev,unsigned int width,unsigned int height,unsigned int framerate)
{
  return createV4L2Device(0,webcam_dev,width,height,framerate);
}

int close_camera()
{
  return destroyV4L2Device(0);
}

void init_dynamic_pages()
{
  //int AmmServer_AddResourceHandler(struct AmmServer_RH_Context * context, char * resource_name , char * web_root, unsigned int allocate_mem_bytes,unsigned int callback_every_x_msec,void * callback);
  AmmServer_AddResourceHandler(v4l2_server,&index_page,(char *) "/index.html",webserver_root,4096,0,(void *) &prepare_index_page_callback,SAME_PAGE_FOR_ALL_CLIENTS);

  //Do not empty jpeg_picture struct since mallocs have already happened.. memset(&jpeg_picture,0,sizeof(struct AmmServer_RH_Context));

  AmmServer_AddResourceHandler(v4l2_server,&jpeg_picture,(char *) "/cam.jpg",webserver_root,jpg_width * jpg_height * 3, 250 /*Poll camera no sooner than once every x ms*/,(void *) &prepare_camera_data_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  jpeg_picture.requestContext.contentSize=jpg_width * jpg_height * 3;
  jpeg_picture.requestContext.MAXcontentSize=jpg_width * jpg_height * 3;

  prepare_camera_data_callback(0); //Do a callback to populate content..!

  AmmServer_DoNOTCacheResourceHandler(v4l2_server,&jpeg_picture);  // Cam.jpg will be constantly changing , do not send 304 NOT MODIFIED
}

void close_dynamic_pages()
{

}


void termination_handler (int signum)
     {
        fprintf(stderr,"Terminating V4L2ToHTTP.. ");
        close_dynamic_pages();
        AmmServer_Stop(v4l2_server);
        fprintf(stderr,"done\n");
        exit(0);
     }




int main(int argc, char *argv[])
{
  printf("V4L2ToHTTP starting up linked to Ammar Server v%s\n",AmmServer_Version());
  if (signal(SIGINT, termination_handler) == SIG_ERR)   printf("Cannot handle SIGINT!\n");
  if (signal(SIGHUP, termination_handler) == SIG_ERR)   printf("Cannot handle SIGHUP!\n");
  if (signal(SIGTERM, termination_handler) == SIG_ERR)  printf("Cannot handle SIGTERM!\n");
  if (signal(SIGKILL, termination_handler) == SIG_ERR)  printf("Cannot handle SIGKILL!\n");

  char bindIP[MAX_INPUT_IP];
  strcpy(bindIP,"0.0.0.0");

  unsigned int port=DEFAULT_BINDING_PORT;

  if ( argc <1 )
    {
      fprintf(stderr,"Something weird is happening , argument zero should be executable path :S \n");
      return 1;
    }
  else if ( argc <= 2 )
    {
      /*fprintf(stderr,"Please note that you may choose a different binding IP/port as command line parameters.. \"ammarserver 0.0.0.0 8080\"\n");*/
    }
  else
    {
      if (strlen(argv[1])>=MAX_INPUT_IP) { fprintf(stderr,"Console argument for binding IP is too long..!\n"); } else
                                         { strncpy(bindIP,argv[1],MAX_INPUT_IP); }
      port=atoi(argv[2]);
      if (port>=MAX_BINDING_PORT)
        {
          port=DEFAULT_BINDING_PORT;
        }
    }
  if (argc>=3)
    {
      strncpy(webcam,argv[3],MAX_FILE_PATH);
    }

  if ( open_camera(webcam,VIDEO_WIDTH,VIDEO_HEIGHT,FRAMERATE) )
    {
      //We were able to start camera so lets start everything else

   v4l2_server = AmmServer_Start("v4l2http",bindIP,port,0,webserver_root,templates_root);
   if (!v4l2_server) { fprintf(stderr,"Could not start webserver closing everything.."); close_camera(); exit(1); }

  if (ENABLE_PASSWORD_PROTECTION)
   {
     AmmServer_SetStrSettingValue(v4l2_server,AMMSET_USERNAME_STR,(char*) "admin");
     AmmServer_SetStrSettingValue(v4l2_server,AMMSET_PASSWORD_STR,(char*) "admin"); //these 2 calls should change BASE64PASSWORD to the proper hex value
    /* It is best to enable password protection after correctly setting both username and password
       to avoid the rare race condition of logging only with username ( i.e. when password hasn't been declared */
     AmmServer_SetIntSettingValue(v4l2_server,AMMSET_PASSWORD_PROTECTION,1);
   }

      init_dynamic_pages(); // Map index.html , cam.jpg to their content and callbacks

      while (AmmServer_Running(v4l2_server))
        {
          //Serving pages on background..!
          usleep(10000);
        }

      close_dynamic_pages(); // Free mappings of AmmServer
      close_camera();

      stopV4L2Module();
      AmmServer_Stop(v4l2_server);
    }


  fprintf(stderr,"Closing application\n");
  return 0;
}
