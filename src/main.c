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
#define ENABLE_PASSWORD_PROTECTION 0


char webserver_root[MAX_FILE_PATH]="public_html/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";


/*! Dynamic content code ..! START!*/
struct AmmServer_RH_Context stats={0};
struct AmmServer_RH_Context form={0};

void * prepare_stats_content_callback()
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



void * prepare_form_content_callback()
{
  strcpy(form.content,"<html><body>");
  strcat(form.content,"<form name=\"input\" action=\"formtest.html\" method=\"get\">Username: <input type=\"text\" name=\"user\" /><input type=\"submit\" value=\"Submit\" /></form>");
  strcat(form.content,"<br><br><br><form name=\"input\" action=\"formtest.html\" method=\"post\">Username: <input type=\"text\" name=\"user\" /><input type=\"submit\" value=\"Submit\" />");
  strcat(form.content,"<input type=\"checkbox\" name=\"vehicle\" value=\"Bike\" /> I have a bike<br /><input type=\"checkbox\" name=\"vehicle\" value=\"Car\" /> I have a car");
  strcat(form.content,"</form></body></html>");
  form.content_size=strlen(form.content);
  return 0;
}


void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback) )
     { fprintf(stderr,"Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(&form,"/formtest.html",webserver_root,4096,0,&prepare_form_content_callback) )
     { fprintf(stderr,"Failed adding form testing page\n"); }
}

void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(&stats,1);
    AmmServer_RemoveResourceHandler(&form,1);
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


    if (ENABLE_PASSWORD_PROTECTION)
    {
      fprintf(stderr,"\nSetting password protection\n");
      AmmServer_SetStrSettingValue(AMMSET_USERNAME_STR,"admin");
      AmmServer_SetStrSettingValue(AMMSET_PASSWORD_STR,"admin"); //these 2 calls should change BASE64PASSWORD in configuration.c to YWRtaW46YW1tYXI= (or something else)
      /* To avoid the rare race condition of logging only with username and keep a proper state ( i.e. when password hasn't been declared )
         It is best to enable password protection after correctly setting both username and password */
      AmmServer_SetIntSettingValue(AMMSET_PASSWORD_PROTECTION,1);
    }

    init_dynamic_content();

         while (AmmServer_Running())
           {
             usleep(10000);
           }

    close_dynamic_content();

    AmmServer_Stop();

    return 0;
}
