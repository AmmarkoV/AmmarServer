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

#define MAX_BINDING_PORT 65534

#define ENABLE_PASSWORD_PROTECTION 0
#define ENABLE_CHAT_BOX 0


#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define ADMIN_BINDING_PORT 8082
#define ENABLE_ADMIN_PAGE 0

//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented out or removed in stable release..
char admin_root[MAX_FILE_PATH]="admin_html/"; // <- change this to the directory that contains your content if you dont want to use the default admin_html dir..
char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

#define MAX_SCRIPT_RESPONSE_SIZE 40960
char * executeScript=0;


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
struct AmmServer_RH_Context gps={0};
struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context executeScriptRC={0};


void * prepare_chatbox_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  sprintf(rqst->content,"<html><head><title>A dead simple file based ChatBox</title></head><body><center><iframe src=\"chat.html\" width=700 height=500>This Browser does not support frames</iframe><br><hr><br>");


  char chatlog_path[MAX_FILE_PATH]={0};
  strcpy(chatlog_path,webserver_root);
  strcat(chatlog_path,"chat.html");

   unsigned int default_post_form = 1;

   if ( rqst->POST_request != 0 )
    {
      if ( strlen(rqst->POST_request)>0 )
       {
         char * username = (char *) malloc ( 256 * sizeof(char) );
         char * comment = (char *) malloc ( 1024 * sizeof(char) );
         if ((username!=0)&&(comment!=0))
          {
            if ( _POST(default_server,rqst,"user",username,256) )
             {
                if (! _POST(default_server,rqst,"comment",comment,1024) ) { fprintf(stderr,"Didn't find a comment \n"); }

                if ((AmmServer_StringIsHTMLSafe(username))&&(AmmServer_StringIsHTMLSafe(comment)))
                {
                 FILE * chatlog = fopen(chatlog_path,"a");
                 if (chatlog!=0)
                  {
                    fprintf(chatlog,"( %02d:%02d:%02d )", tm.tm_hour, tm.tm_min, tm.tm_sec);
                    fprintf(chatlog,"%s : %s <br>",username,comment);
                    fclose(chatlog);

                    strcat(rqst->content,"<form name=\"input\" action=\"chatbox.html\" method=\"post\">");
                    strcat(rqst->content,"  Username: <input type=\"text\" name=\"user\" readonly=\"readonly\" value =\"");
                    strcat(rqst->content,username);
                    strcat(rqst->content,"\" />Comment: <input type=\"text\" name=\"comment\" /><input type=\"submit\" value=\"Send\" /></form><br>\n");
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
      strcat(rqst->content,"<form name=\"input\" action=\"chatbox.html\" method=\"post\">");
      strcat(rqst->content,"Username: <input type=\"text\" name=\"user\" />");
      strcat(rqst->content,"Comment: <input type=\"text\" name=\"comment\" /><input type=\"submit\" value=\"Send\" /></form><br>\n");
     }

  strcat(rqst->content,"</center></body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  sprintf(rqst->content,"<html><head><title>Dynamic Content Enabled</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>",
                    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);
  strcat(rqst->content,"The string you see is updated dynamically every time you get a fresh copy of this file!<br><br>\n");
  strcat(rqst->content,"To include your own content see the <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/main.c#L46\">Dynamic content code label in ammarserver main.c</a><br>\n");
  strcat(rqst->content,"If you dont need dynamic content at all consider disabling it from ammServ.conf or by setting DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=0; in ");
  strcat(rqst->content,"<a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/file_caching.c\">file_caching.c</a> and recompiling.!</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_random_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  strcpy(rqst->content,"<html><head><title>Random Number Generator</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>");

  char hex[10]={0};
  unsigned int i=0;
  for (i=0; i<1024; i++)
    {
        sprintf(hex, "%x ", rand()%256 );
        strcat(rqst->content,hex);
    }

  strcat(rqst->content,"</body></html>");

  rqst->contentSize=strlen(rqst->content);
  return 0;
}



//This function prepares the content of  form context , ( content )
void * prepare_form_content_callback(struct AmmServer_DynamicRequest  * rqst)
{

  strcpy(rqst->content,"<html><body>");
  strcat(rqst->content,"<form name=\"input\" action=\"formtest.html\" method=\"get\">Username: <input type=\"text\" name=\"user\" />Comment: <input type=\"text\" name=\"comment\" /><input type=\"submit\" value=\"Submit\" /></form>");
  strcat(rqst->content,"<br><br><br><form name=\"input\" action=\"formtest.html\" method=\"post\">Username: <input type=\"text\" name=\"user\" /><input type=\"submit\" value=\"Submit\" />");
  strcat(rqst->content,"<input type=\"checkbox\" name=\"vehicle\" value=\"Bike\" /> I have a bike<br /><input type=\"checkbox\" name=\"vehicle\" value=\"Car\" /> I have a car &nbsp; ");
  strcat(rqst->content,"<input type=\"file\" name=\"testfile\" size=\"chars\"><br></form><br><br><br>");



   if ( rqst->POST_request != 0 )
    {
      if ( strlen(rqst->POST_request)>0 )
       {
         strcat(rqst->content,"<hr>POST REQUEST dynamically added here : <br><i>");
         strcat(rqst->content, rqst->POST_request);
         strcat(rqst->content,"</i><hr>");

         char * username = (char *) malloc ( 256 * sizeof(char) );
         if (username!=0)
          {
            if ( _POST(default_server,rqst,"user",username,256) )
             {
               strcat(rqst->content,"GOT A POST USERNAME !!!  : "); strcat(rqst->content,username); strcat(rqst->content," ! ! <br>");
             }
            free(username);
          }
       }
    }


  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         strcat(rqst->content,"<hr>GET REQUEST dynamically added here : <br><i>"); strcat(rqst->content, rqst->GET_request ); strcat(rqst->content,"</i><hr>");

         char * username = (char *) malloc ( 256 * sizeof(char) );
         if (username!=0)
          {
            if ( _GET(default_server,rqst,"user",username,256) )
             {
               strcat(rqst->content,"GOT A GET USERNAME !!!  : "); strcat(rqst->content,username); strcat(rqst->content," ! ! <br>");
             }
            free(username);
          }
       }
    }


  strcat(rqst->content,"</body></html>");


  rqst->contentSize=strlen(rqst->content);
  return 0;
}



//This function prepares the content of  form context , ( content )
void * prepare_gps_content_callback(struct AmmServer_DynamicRequest  * rqst)
{

  char latitude[128]={0};
  char longitude[128]={0};
  char message[256]={0};

 AmmServer_Warning("New GPS message");
 if ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         if ( _GET(default_server,rqst,"lat",latitude,128) )
             {
               fprintf(stderr,"Latitude : %s \n",latitude);
             }
         if ( _GET(default_server,rqst,"lon",longitude,128) )
             {
               fprintf(stderr,"Longitude : %s \n",longitude);
             }
         if ( _GET(default_server,rqst,"msg",message,256) )
             {
               fprintf(stderr,"Message : %s \n",message);
             }
       }
    }

  strcpy(rqst->content,"<html><body>Ack</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}





//This function prepares the content of  form context , ( content )
void * executeScriptFunction(struct AmmServer_DynamicRequest  * rqst)
{
 rqst->contentSize=0;
 if (executeScript==0) { AmmServer_Error("execute script called , but no execute script is declared \n"); return 0; }

 char * response = (char*) malloc(sizeof(char) * MAX_SCRIPT_RESPONSE_SIZE);

 if (response==0)
 {
    strcpy(rqst->content,"<html><body>Internal Error when executing command</body></html>");
    rqst->contentSize=strlen(rqst->content);
 } else
 {
    AmmServer_ExecuteCommandLine( executeScript , response , MAX_SCRIPT_RESPONSE_SIZE );
    sprintf(rqst->content,"<html><body><textarea name=\"commandline\" cols=\"80\" rows=\"24\">%s</textarea><br/><a href=\"javascript:location.reload();\">Rerun</a></body></html>",response);
    rqst->contentSize=strlen(rqst->content);

    free(response);
 }

  return 0;
}




//This function prepares the content of  form context , ( content )
void * request_override_callback(char * content)
{
  // char requestHeader;
  // struct HTTPHeader * request;
  // void * request_override_callback;

  //This does nothing for now :P
  return 0;
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  if (! AmmServer_AddResourceHandler(default_server,&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) )  { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&form,"/formtest.html",webserver_root,4096,0,&prepare_form_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { AmmServer_Warning("Failed adding form testing page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&random_chars,"/random.html",webserver_root,4096,0,&prepare_random_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding random testing page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&gps,"/gps.html",webserver_root,4096,0,&prepare_gps_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { AmmServer_Warning("Failed adding gps testing page\n"); }

  if (executeScript!=0)
  {
     if (! AmmServer_AddResourceHandler(default_server,&gps,"/execute.html",webserver_root,16000,0,&executeScriptFunction,DIFFERENT_PAGE_FOR_EACH_CLIENT) )
         { AmmServer_Warning("Failed adding execute page\n"); }
  }



  if (ENABLE_CHAT_BOX)
  {
   if (!AmmServer_AddResourceHandler(default_server,&chatbox,"/chatbox.html",webserver_root,4096,0,&prepare_chatbox_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) )
      { AmmServer_Warning("Failed adding chatbox page\n"); }

     char chatlog_path[MAX_FILE_PATH]={0};
     strcpy(chatlog_path,webserver_root);
     strcat(chatlog_path,"chat.html");
     AmmServer_EraseFile(chatlog_path);

     AmmServer_DoNOTCacheResourceHandler(default_server,&chatbox);
  }
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);
    AmmServer_RemoveResourceHandler(default_server,&form,1);
    AmmServer_RemoveResourceHandler(default_server,&random_chars,1);
    AmmServer_RemoveResourceHandler(default_server,&gps,1);
    if (ENABLE_CHAT_BOX) { AmmServer_RemoveResourceHandler(default_server,&chatbox,1); }

    if (executeScript!=0)
        {
          AmmServer_RemoveResourceHandler(default_server,&executeScriptRC,1);
          free(executeScript);
          executeScript=0;
        }

}
/*! Dynamic content code ..! END ------------------------*/




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
   //If we have a command line arguments we overwrite our buffers

   unsigned int i=0;
   for (i=0; i<argc; i++)
   {
    if ((strcmp(argv[i],"-e")==0)&&(argc>i+1))
      {
        if ( strlen(argv[i+1]) > MAX_RESOURCE ) { AmmServer_Error("-e argument is too long , will not allocate such a big chunk of memory"); return 1; } else
        {
          executeScript = (char*) malloc(sizeof(unsigned char) * strlen(argv[i+1]) );
          if (executeScript==0) { AmmServer_Error("-e argument could not be stored , could not allocate a chunk of memory for it"); return 1; }
        }

        strncpy(executeScript,argv[i+1] ,  strlen(argv[i+1]) );
        fprintf(stderr,"AmmarServer Will Execute script %s \n",executeScript);
      }
   }


    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;


    default_server = AmmServer_StartWithArgs(
                                             "ammarserver",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );

    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    if (ENABLE_ADMIN_PAGE)
     {
      admin_server = AmmServer_StartAdminInstance
        (
           bindIP,
           ADMIN_BINDING_PORT
         );
       if (!admin_server) { AmmServer_Warning("Could not create admin server carying on though...");  }
     }



    //If we want password protection ( variable defined in the start of this file ) we will have to set a username and a password
    //and then enable password protection
    if (ENABLE_PASSWORD_PROTECTION)
    {
      fprintf(stderr,"\nEnabling password protection\n");
      AmmServer_SetStrSettingValue(default_server,AMMSET_USERNAME_STR,"admin");
      AmmServer_SetStrSettingValue(default_server,AMMSET_PASSWORD_STR,"admin"); //these 2 calls should change BASE64PASSWORD in configuration.c to YWRtaW46YW1tYXI= (or something else)
      /* To avoid the rare race condition of logging only with username and keep a proper state ( i.e. when password hasn't been declared )
         It is best to enable password protection after correctly setting both username and password */
      AmmServer_SetIntSettingValue(default_server,AMMSET_PASSWORD_PROTECTION,1);
      //fprintf(stderr,"Debug ..  , Username = %s , Password = %s , Base64Representation = %s \n",default_server->USERNAME,default_server->PASSWORD,default_server->BASE64PASSWORD);
    }

    if (ENABLE_ADMIN_PAGE)
     {
      AmmServer_SetStrSettingValue(admin_server,AMMSET_USERNAME_STR,"admin");
      AmmServer_SetStrSettingValue(admin_server,AMMSET_PASSWORD_STR,"admin");
      AmmServer_SetIntSettingValue(admin_server,AMMSET_PASSWORD_PROTECTION,1);
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
