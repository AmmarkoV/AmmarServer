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
#include <signal.h>
#include "../AmmServerlib/AmmServerlib.h"

#define MAX_BINDING_PORT 65534
#define MAX_INPUT_IP 256

#define ENABLE_PASSWORD_PROTECTION 0
#define ENABLE_CHAT_BOX 0


#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define ADMIN_BINDING_PORT 8082
#define ENABLE_ADMIN_PAGE 0

//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented out or removed in stable release..
char admin_root[MAX_FILE_PATH]="admin_html/"; // <- change this to the directory that contains your content if you dont want to use the default admin_html dir..
char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";
char * page=0;
unsigned int pageLength=0;

/*! Dynamic content code ..! START!*/
/* A few words about dynamic content here..
   This is actually one of the key features on AmmarServer and maybe the reason that I started the whole project
   What I am trying to do here is serve content by directly linking the webserver to binary ( NOT Interpreted ) code
   in order to serve pages with the maximum possible efficiency and skipping all intermidiate layers..

   PHP , Ruby , Python and all other "web-languages" are very nice and handy and to be honest I can do most of my work fine using PHP , MySQL and Apache
   However setting up , configuring and maintaining large projects with different database systems , seperate configuration files for each of the sub parts
   and re deploying everything is a very tiresome affair.. Not to mention that despite the great work done by the apache  , php etc teams performance is wasted
   due to the interpreters of the various scripting languages used..

   Things can't get any faster than AmmarServer and the whole programming interface exposed to the programmer is ( imho ) very friendly and familiar to even inexperienced
   C developer..

   What follows is the decleration of some "Dynamic Content Resources" their Constructors/Destructors and their callback routines that fill them with the content to be served
   each time a client requests one of the pages..

   One can test them by opening http://127.0.0.1:8081/stats.html for a dynamic time page and http://127.0.0.1:8081/formtest.html for form testing..

*/

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_Instance  * admin_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};

struct AmmServer_RH_Context stats={0};
struct AmmServer_RH_Context form={0};
struct AmmServer_RH_Context chatbox={0};
struct AmmServer_RH_Context random_chars={0};


char FileExistsTest(char * filename)
{
 FILE *fp = fopen(filename,"r");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 fprintf(stderr,"FileExists(%s) returns 0\n",filename);
 return 0;
}

char EraseFile(char * filename)
{
 FILE *fp = fopen(filename,"w");
 if( fp ) { /* exists */ fclose(fp); return 1; }
 return 0;
}


unsigned int StringIsHTMLSafe(char * str)
{
  unsigned int i=0;
  while(i<strlen(str)) { if ( ( str[i]<'!' ) || ( str[i]=='<' ) || ( str[i]=='>' ) ) { return 0;} ++i; }
  return 1;
}




//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(char * content)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  sprintf(content,"<html><head><title>Dynamic Content Enabled</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>",
                    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);
  strcat(content,"The string you see is updated dynamically every time you get a fresh copy of this file!<br><br>\n");
  strcat(content,"To include your own content see the <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/main.c#L37\">Dynamic content code label in ammarserver main.c</a><br>\n");
  strcat(content,"If you dont need dynamic content at all consider disabling it from ammServ.conf or by setting DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=0; in ");
  strcat(content,"<a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/file_caching.c\">file_caching.c</a> and recompiling.!</body></html>");
  stats.content_size=strlen(content);
  return 0;
}

void execute(char * command,char * param)
{
  fprintf(stderr,"Execute(%s,%s) \n",command,param);
  int i=0;
  char commandToRun[1024]={0};
  if (strcmp(command,"head")==0)
  {
    if (strcmp(param,"right")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"right\" -1\" "); }
    if (strcmp(param,"down")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"down\" -1\" ");  }
    if (strcmp(param,"left")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"left\" -1\" ");  }
    if (strcmp(param,"up")==0)    { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"up\" -1\" ");    }
  }

  if ( strlen(commandToRun)!=0 )
   {
     i=system(commandToRun);
     if (i!=0) { fprintf(stderr,"Command %s failed\n",commandToRun); } else
               { fprintf(stderr,"Command %s success\n",commandToRun); }
   } else
   {
     fprintf(stderr,"Execute with unknown command or parameter\n");
   }

  return ;
}



//This function prepares the content of  form context , ( content )
void * prepare_form_content_callback(char * content)
{
  //fprintf(stderr,"Sending back %s\n" , page );
  strncpy(content,page,pageLength);
  content[pageLength]=0;
  //strcat(content,"\0");
  form.content_size=pageLength;


  char * headCommand = 0;

  if  ( form.GET_request != 0 )
    {
      if ( strlen(form.GET_request)>0 )
       {
         char * headCommand = (char *) malloc ( 256 * sizeof(char) );
         if (headCommand!=0)
          {
            if ( _GET(default_server,&form,"head",headCommand,256) )
             {
               printf("%s <- head command \n",headCommand);
               execute("head",headCommand);
             }
            free(headCommand);
          }
       }
    }




  form.content_size=strlen(content);
  return 0;
}


//This function prepares the content of  form context , ( content )
void * request_override_callback(char * content)
{
  // char requestHeader;
  // struct HTTPRequest * request;
  // void * request_override_callback;

  //This does nothing for now :P
  return 0;
}


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  if (! AmmServer_AddResourceHandler(default_server,&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { fprintf(stderr,"Failed adding stats page\n"); }


  page=AmmServer_ReadFileToMemory("src/ScriptRunner/page.html",&pageLength);
  fprintf(stderr,"page %s , pageLength %u \n",page , pageLength);
  if (! AmmServer_AddResourceHandler(default_server,&form,"/formtest.html",webserver_root,pageLength+1,0,&prepare_form_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
     { fprintf(stderr,"Failed adding form testing page\n"); }

 }

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);
    AmmServer_RemoveResourceHandler(default_server,&form,1);
}
/*! Dynamic content code ..! END ------------------------*/



void termination_handler (int signum)
     {
        fprintf(stderr,"Terminating AmmarServer.. \n");
        close_dynamic_content();
        AmmServer_Stop(default_server);
        fprintf(stderr,"done\n");
        exit(0);
     }



int main(int argc, char *argv[])
{
    printf("Ammar Server %s starting up..\n",AmmServer_Version());

    if (signal(SIGINT, termination_handler) == SIG_ERR)   printf("Cannot handle SIGINT!\n");
    if (signal(SIGHUP, termination_handler) == SIG_ERR)   printf("Cannot handle SIGHUP!\n");
    if (signal(SIGTERM, termination_handler) == SIG_ERR)  printf("Cannot handle SIGTERM!\n");
    if (signal(SIGKILL, termination_handler) == SIG_ERR)  printf("Cannot handle SIGKILL!\n");


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
    default_server = AmmServer_Start
        (
           bindIP,
           port,
           0, /*This means we don't want a specific configuration file*/
           webserver_root,
           templates_root
         );

    if (ENABLE_ADMIN_PAGE)
     {
      admin_server = AmmServer_StartAdminInstance
        (
           bindIP,
           ADMIN_BINDING_PORT
         );
       if (!admin_server) { fprintf(stderr,"Could not create admin server carying on though...");  }
     }

    if (!default_server) { fprintf(stderr,"Closing everything.."); exit(1); }



    //Create dynamic content allocations and associate context to the correct files
    init_dynamic_content();
    //stats.html and formtest.html should be availiable from now on..!


         while ( (AmmServer_Running(default_server))  )
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
    AmmServer_Stop(default_server);

    return 0;
}
