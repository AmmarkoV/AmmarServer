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
#include "AmmServerlib/AmmServerlib.h"

#define MAX_BINDING_PORT 65534
#define DEFAULT_BINDING_PORT 8080
#define MAX_INPUT_IP 256

char webserver_root[MAX_FILE_PATH]="public_html/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";


/*! Dynamic content code ..! START!*/
struct AmmServer_RH_Context stats={0};

void * prepare_content_callback()
{
  //fprintf(stderr,"CallbackCalled!\n");
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  sprintf(stats.content,"<html><head><title>Dynamic Content Enabled</title></head><body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>",
                    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);
  strcat(stats.content,"The string you see is updated dynamically every time you get a fresh copy of this file!<br><br>\n");
  strcat(stats.content,"To include your own content see the <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/main.c#L37\">Dynamic content code label in ammarserver main.c</a><br>\n");
  strcat(stats.content,"If you dont need dynamic content at all consider disabling it from ammServ.conf or by setting DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=0; in ");
  strcat(stats.content,"<a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/file_caching.c\">file_caching.c</a> and recompiling.!</body></html>");
  stats.content_size=strlen(stats.content);
  return 0;
}

void init_dynamic_content()
{
   memset(&stats,0,sizeof(struct AmmServer_RH_Context));
   strncpy(stats.web_root_path,webserver_root,MAX_FILE_PATH);
   strncpy(stats.resource_name,"/stats.html",MAX_RESOURCE);
   stats.MAX_content_size=4096;
   stats.prepare_content_callback=&prepare_content_callback;


   stats.content = (char*) malloc(sizeof(char) * stats.MAX_content_size);
   if (stats.content!=0)
     { //AmmServer_AddResourceHandlerOLD(webserver_root,"/stats.html",stats_buf,&stats_size,&prepare_content_callback);
       AmmServer_AddResourceHandler(&stats);
     }/*! Dynamic content Add Resource Handler..! */
}

void close_dynamic_content()
{
    if (stats.content !=0)
     {
       stats.MAX_content_size=0;
       free(stats.content );
       stats.content =0;
     }
}
/*! Dynamic content code ..! END ------------------------*/


int main(int argc, char *argv[])
{
    printf("Ammar Server starting up\n");

    char bindIP[MAX_INPUT_IP];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;


    if ( argc <1 ) { fprintf(stderr,"Something weird is happening , argument zero should be executable path :S \n"); return 1; } else
    if ( argc <= 2 ) { /*fprintf(stderr,"Please note that you may choose a different binding IP/port as command line parameters.. \"ammarserver 0.0.0.0 8080\"\n");*/ } else
     {
        if (strlen(argv[1])>=MAX_INPUT_IP) { fprintf(stderr,"Console argument for binding IP is too long..!\n"); } else
                                           { strncpy(bindIP,argv[1],MAX_INPUT_IP); }
        port=atoi(argv[2]);
        if (port>=MAX_BINDING_PORT) { port=DEFAULT_BINDING_PORT; }
     }
   if (argc>=3) { strncpy(webserver_root,argv[3],MAX_FILE_PATH); }
   if (argc>=4) { strncpy(templates_root,argv[4],MAX_FILE_PATH); }

    AmmServer_Start(bindIP,port,0,webserver_root,templates_root);

    //fprintf(stderr,"\nSetting password protection\n");
    //AmmServer_SetIntSettingValue(AMMSET_PASSWORD_PROTECTION,1);
    //AmmServer_SetStrSettingValue(AMMSET_PASSWORD_STR,"46YW1tYXI=");

    AmmServer_SetStrSettingValue(AMMSET_USERNAME_STR,"ammar");
    AmmServer_SetStrSettingValue(AMMSET_PASSWORD_STR,"ammar");
    AmmServer_SetStrSettingValue(AMMSET_PASSWORD_STR,"ammar");
    AmmServer_SetIntSettingValue(AMMSET_PASSWORD_PROTECTION,1); // It is best to enable password protection after correctly setting both username and password
    //to avoid the rare race condition of logging only with username ( i.e. when password hasn't been declared

    init_dynamic_content();

         while (AmmServer_Running())
           {
             usleep(10000);
           }

    close_dynamic_content();

    AmmServer_Stop();

    return 0;
}
