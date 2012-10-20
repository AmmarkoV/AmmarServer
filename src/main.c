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

#define ENABLE_CHAT_BOX 0

//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented or removed in stable release..
char webserver_root[MAX_FILE_PATH]="public_html/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";


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
struct AmmServer_RH_Context stats={0};
struct AmmServer_RH_Context form={0};
struct AmmServer_RH_Context chatbox={0};



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



void * prepare_chatbox_content_callback(unsigned int associated_vars)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  sprintf(chatbox.content,"<html><head><title>A dead simple file based ChatBox</title></head><body><center><iframe src=\"chat.html\" width=700 height=500>This Browser does not support frames</iframe><br><hr><br>");


  char chatlog_path[MAX_FILE_PATH]={0};
  strcpy(chatlog_path,webserver_root);
  strcat(chatlog_path,"chat.html");

   unsigned int default_post_form = 1;

   if ( chatbox.POST_request != 0 )
    {
      if ( strlen(chatbox.POST_request)>0 )
       {
         char * username = (char *) malloc ( 256 * sizeof(char) );
         char * comment = (char *) malloc ( 1024 * sizeof(char) );
         if ((username!=0)&&(comment!=0))
          {
            if ( _POST(&chatbox,"user",username,256) )
             {
                if (! _POST(&chatbox,"comment",comment,1024) ) { fprintf(stderr,"Didn't find a comment \n"); }

                if ((StringIsHTMLSafe(username))&&(StringIsHTMLSafe(comment)))
                {
                 FILE * chatlog = fopen(chatlog_path,"a");
                 if (chatlog!=0)
                  {
                    fprintf(chatlog,"( %02d:%02d:%02d )", tm.tm_hour, tm.tm_min, tm.tm_sec);
                    fprintf(chatlog,"%s : %s <br>",username,comment);
                    fclose(chatlog);

                    strcat(chatbox.content,"<form name=\"input\" action=\"chatbox.html\" method=\"post\">");
                    strcat(chatbox.content,"  Username: <input type=\"text\" name=\"user\" readonly=\"readonly\" value =\"");
                    strcat(chatbox.content,username);
                    strcat(chatbox.content,"\" />Comment: <input type=\"text\" name=\"comment\" /><input type=\"submit\" value=\"Send\" /></form><br>\n");
                    default_post_form=0;
                  }
               }
           }
          }
         if (username!=0) { free(username); }
         if (comment!=0) { free(comment);  }
       }
    }


  if (default_post_form)
    {
      strcat(chatbox.content,"<form name=\"input\" action=\"chatbox.html\" method=\"post\">");
      strcat(chatbox.content,"Username: <input type=\"text\" name=\"user\" />");
      strcat(chatbox.content,"Comment: <input type=\"text\" name=\"comment\" /><input type=\"submit\" value=\"Send\" /></form><br>\n");
     }

  strcat(chatbox.content,"</center></body></html>");
  chatbox.content_size=strlen(chatbox.content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(unsigned int associated_vars)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  fprintf(stderr,"Associated Var with stats callback is %u \n",associated_vars);

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



//This function prepares the content of  form context , ( form.content )
void * prepare_form_content_callback(unsigned int associated_vars)
{

  strcpy(form.content,"<html><body>");
  strcat(form.content,"<form name=\"input\" action=\"formtest.html\" method=\"get\">Username: <input type=\"text\" name=\"user\" />Comment: <input type=\"text\" name=\"comment\" /><input type=\"submit\" value=\"Submit\" /></form>");
  strcat(form.content,"<br><br><br><form name=\"input\" action=\"formtest.html\" method=\"post\">Username: <input type=\"text\" name=\"user\" /><input type=\"submit\" value=\"Submit\" />");
  strcat(form.content,"<input type=\"checkbox\" name=\"vehicle\" value=\"Bike\" /> I have a bike<br /><input type=\"checkbox\" name=\"vehicle\" value=\"Car\" /> I have a car &nbsp; ");
  strcat(form.content,"<input type=\"file\" name=\"testfile\" size=\"chars\"><br></form><br><br><br>");


  fprintf(stderr,"Associated Var with form callback is %u \n",associated_vars);

   if ( form.POST_request != 0 )
    {
      if ( strlen(form.POST_request)>0 )
       {
         strcat(form.content,"<hr>POST REQUEST dynamically added here : <br><i>"); strcat(form.content, form.POST_request); strcat(form.content,"</i><hr>");

         char * username = (char *) malloc ( 256 * sizeof(char) );
         if (username!=0)
          {
            if ( _POST(&form,"user",username,256) )
             {
               strcat(form.content,"GOT A POST USERNAME !!!  : "); strcat(form.content,username); strcat(form.content," ! ! <br>");
             }
            free(username);
          }
       }
    }


  if  ( form.GET_request != 0 )
    {
      if ( strlen(form.GET_request)>0 )
       {
         strcat(form.content,"<hr>GET REQUEST dynamically added here : <br><i>"); strcat(form.content, form.GET_request ); strcat(form.content,"</i><hr>");

         char * username = (char *) malloc ( 256 * sizeof(char) );
         if (username!=0)
          {
            if ( _GET(&form,"user",username,256) )
             {
               strcat(form.content,"GOT A GET USERNAME !!!  : "); strcat(form.content,username); strcat(form.content," ! ! <br>");
             }
            free(username);
          }
       }
    }


  strcat(form.content,"</body></html>");


  form.content_size=strlen(form.content);
  return 0;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback) )
     { fprintf(stderr,"Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(&form,"/formtest.html",webserver_root,4096,0,&prepare_form_content_callback) )
     { fprintf(stderr,"Failed adding form testing page\n"); }

  if (ENABLE_CHAT_BOX)
  {
   if (! AmmServer_AddResourceHandler(&chatbox,"/chatbox.html",webserver_root,4096,0,&prepare_chatbox_content_callback) )
      { fprintf(stderr,"Failed adding chatbox page\n"); }

     char chatlog_path[MAX_FILE_PATH]={0};
     strcpy(chatlog_path,webserver_root);
     strcat(chatlog_path,"chat.html");
     EraseFile(chatlog_path);

     AmmServer_DoNOTCacheResource(chatlog_path); // Chat Html will be changing all the time , so we don't want to cache it..!
  }
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(&stats,1);
    AmmServer_RemoveResourceHandler(&form,1);
    if (ENABLE_CHAT_BOX) { AmmServer_RemoveResourceHandler(&chatbox,1); }
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


    //If we want password protection ( variable defined in the start of this file ) we will have to set a username and a password
    //and then enable password protection
    if (ENABLE_PASSWORD_PROTECTION)
    {
      fprintf(stderr,"\nEnabling password protection\n");
      AmmServer_SetStrSettingValue(AMMSET_USERNAME_STR,"admin");
      AmmServer_SetStrSettingValue(AMMSET_PASSWORD_STR,"admin"); //these 2 calls should change BASE64PASSWORD in configuration.c to YWRtaW46YW1tYXI= (or something else)
      /* To avoid the rare race condition of logging only with username and keep a proper state ( i.e. when password hasn't been declared )
         It is best to enable password protection after correctly setting both username and password */
      AmmServer_SetIntSettingValue(AMMSET_PASSWORD_PROTECTION,1);
    }

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
