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
#include "../AmmServerlib/AmmServerlib.h"

#define MAX_BINDING_PORT 65534
#define MAX_INPUT_IP 256


#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented or removed in stable release..
char templates_root[MAX_FILE_PATH]="public_html/templates/";



struct AmmServer_RH_Context create_url={0};
struct AmmServer_RH_Context goto_url={0};



//This function prepares the content of  the url creator context
void * serve_create_url_page(unsigned int associated_vars)
{

  strcpy(create_url.content,"<html><head><title>Welcome to MyURL</title></head><body><br><br><br><br><br><br><br><br><br><br><center><table border=5><tr><td><center><br><h2>Welcome to MyURL</h2><br>");

  strcat(create_url.content,"<form name=\"input\" action=\"go.html\" method=\"get\">Long URL : <input type=\"text\" name=\"url\" /> Name: <input type=\"text\" name=\"name\" value=\"automatic generation\" /><input type=\"submit\" value=\"Submit\" /></form>");

  strcat(create_url.content,"</center><br><br></td></tr></table></center></body></html>");
  create_url.content_size=strlen(create_url.content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_goto_url_page(unsigned int associated_vars)
{
  strcat(goto_url.content,"<html><body>This is the serve go URL page</body></html>");
  goto_url.content_size=strlen(goto_url.content);
  return 0;
}


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(&create_url,"/index.html",webserver_root,4096,0,&serve_create_url_page) ) { fprintf(stderr,"Failed adding create page\n"); }
  AmmServer_DoNOTCacheResource("public_html/index.html"); // Chat Html will be changing all the time , so we don't want to cache it..!

  if (! AmmServer_AddResourceHandler(&goto_url,"/go.html",webserver_root,4096,0,&serve_goto_url_page) ) { fprintf(stderr,"Failed adding form testing page\n"); }
  AmmServer_DoNOTCacheResource("public_html/go.html"); // Chat Html will be changing all the time , so we don't want to cache it..!

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(&create_url,1);
    AmmServer_RemoveResourceHandler(&goto_url,1);
}
/*! Dynamic content code ..! END ------------------------*/


int main(int argc, char *argv[])
{
    printf("Ammar Server starting up\n");

    char bindIP[MAX_INPUT_IP];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;


    if ( argc <1 ) { fprintf(stderr,"Something weird is happening , argument zero should be executable path :S \n"); return 1; } else
    if ( argc <= 2 ) {  } else
     {
        if (strlen(argv[1])>=MAX_INPUT_IP) { fprintf(stderr,"Console argument for binding IP is too long..!\n"); } else
                                           { strncpy(bindIP,argv[1],MAX_INPUT_IP); }
        port=atoi(argv[2]);
        if (port>=MAX_BINDING_PORT) { port=DEFAULT_BINDING_PORT; }
     }
   if (argc>=3) { strncpy(webserver_root,argv[3],MAX_FILE_PATH); }
   if (argc>=4) { strncpy(templates_root,argv[4],MAX_FILE_PATH); }

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    AmmServer_Start(bindIP,port,0,webserver_root,templates_root);

    //Create dynamic content allocations and associate context to the correct files
    init_dynamic_content();
    //stats.html and formtest.html should be availiable from now on..!

         while (AmmServer_Running())
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             //In other applications the programmer could use the main thread to do anything he likes..
             //The only caveat is that he would takeup more CPU time from the server and that he would have to poll
             //the AmmServer_Running() call once in a while to make sure everything is in order
             usleep(10000);
           }

    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop();

    return 0;
}
