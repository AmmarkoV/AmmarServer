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

struct AmmServer_RH_Context indexPage={0};
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

void replaceChar(char * input , char findChar , char replaceWith)
{
  char * cur = input;
  char * inputEnd = input+strlen(input);
  while ( cur < inputEnd )
  {
     if (*cur == findChar ) { *cur = replaceWith; }
     ++cur;
  }
  return ;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_index_content_callback(char * content)
{
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  strcpy(content,"<html><head><title>Welcome</title>\
                  <body><center><br><br><br>\
                   <h1>The incredibly minimal WebInterface for Hobbit</h1><br><br>\
                   <h3><a href=\"controlpanel.html\">Click Here to open Control Panel</a></h3>\
                   <h3><a href=\"stats.html\">Click Here for stats (not ready yet)</a></h3>\
                   </body></html> ");
  indexPage.content_size=strlen(content);
  return 0;
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


void joystickExecute(float x , float y )
{
   AmmServer_Warning("Joystick(%0.2f,%0.2f)\n",x,y);

   char commandToRun[1024]={0};
    sprintf(commandToRun,
           "rostopic pub /joy sensor_msgs/Joy \"{ axes: [ %0.2f , %0.2f , 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 ] , buttons: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] }\" -1&"
            ,x,y
           );

    int i=system(commandToRun);
    if (i!=0) { AmmServer_Error("Command %s failed\n",commandToRun); } else
               { AmmServer_Success("Command %s success\n",commandToRun); }

}


void execute(char * command,char * param)
{
  fprintf(stderr,"Execute(%s,%s) \n",command,param);
  int i=0;
  char commandToRun[1024]={0};

  if (strcmp(command,"head")==0)
  {
    if (strcmp(param,"default")==0)  {
                                        execute("head","center");
                                        execute("head","middle");
                                        return;
                                     } else
    if (strcmp(param,"right")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"right\" -1\" "); } else
    if (strcmp(param,"center")==0)    { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"center\" -1\" ");    } else
    if (strcmp(param,"left")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"left\" -1\" ");  } else
    if (strcmp(param,"up")==0)    { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"up\" -1\" ");    } else
    if (strcmp(param,"middle")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"middle\" -1\" ");  } else
    if (strcmp(param,"down")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /HeadMove std_msgs/String \"down\" -1\" ");  }
  }
   else
  if (strcmp(command,"rtd")==0)
  {
    if (strcmp(param,"home")==0) 		{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseHome.py"); 			} else
    if (strcmp(param,"almosthome")==0) 		{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseAlmostHome.py");		} else
    if (strcmp(param,"armatside")==0) 		{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseArmAtSide.py"); 		} else
    if (strcmp(param,"cfaftergrasp")==0)	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseCFAfterGrasp.py"); 		} else
    if (strcmp(param,"lowerlearn")==0) 		{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseLearn.py");			} else
    if (strcmp(param,"tablepregrasp")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseTPregrasp.py"); 		} else
    if (strcmp(param,"tablefinalgrasp")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseTFinalGrasp.py"); 		} else
    if (strcmp(param,"trayprerelease")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PosePreReleaseInTray.py"); 	} else
    if (strcmp(param,"trayrelease")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseReleaseInTray.py"); 		} else
    if (strcmp(param,"clearfloorpregrasp")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseCFPregrasp.py"); 		} else
    if (strcmp(param,"clearfloorfinalgrasp")==0){ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_POSES/RTD_PoseCFFinalGrasp.py"); 		} else
    if (strcmp(param,"opengripper")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_COMMANDS/RTD_OpenGripper.py"); 		} else
    if (strcmp(param,"closegripper")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_COMMANDS/RTD_CloseGripper.py"); 		} else
    if (strcmp(param,"disableallaxis")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_COMMANDS/RTD_DisableAllAxis.py"); 		} else
    if (strcmp(param,"graspfromtable")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_SCENARIOS/RTD_GraspFromTable.sh"); 		} else
    if (strcmp(param,"graspfromfloor")==0) 	{ strcpy(commandToRun,"/home/hobbit/hobbit/ActionSequencer/src/RTD_SCENARIOS/RTD_GraspFromFloor.sh"); 		}
  }
   else
  if (strcmp(command,"hand")==0)
  {
    if (strcmp(param,"calibrate")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_ARM_REFERENCE'\" -1  "); }
  }
   else
  if (strcmp(command,"body")==0)
  {
    if (strcmp(param,"right")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Turn -30'\" -1"); } else
    if (strcmp(param,"left")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Turn 30'\" -1"); } else
    if (strcmp(param,"forward")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Move 0.30'\" -1"); } else
    if (strcmp(param,"back")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Move -0.30'\" -1"); }else
    if (strcmp(param,"stop")==0) {  joystickExecute(0.0,0.0);
                                    strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Stop'\" -1"); }
  }
   else
  if (strcmp(command,"bring")==0)
  {
    if (strcmp(param,"aspirin")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_BRING' , params: [ {name: 'Name' , value: 'ΑΣΠΙΡΊΝΗ'} ] }\" -1  "); } else
    if (strcmp(param,"sugar")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_BRING' , params: [ {name: 'Name' , value: 'ΖΆΧΑΡΗ'} ] }\" -1  "); }
  }
   else
  if (strcmp(command,"robot")==0)
  { ///bin/bash -c \"

    if (strcmp(param,"yes")==0) { strcpy(commandToRun,"rostopic pub /Event HobbitMsgs/Event \"event: 'G_YES'\" -1  "); } else
    if (strcmp(param,"no")==0) { strcpy(commandToRun,"rostopic pub /Event HobbitMsgs/Event \"event: 'G_NO'\" -1  "); } else
    if (strcmp(param,"stop")==0) { strcpy(commandToRun,"rostopic pub /Command HobbitMsgs/Command \"command: 'C_STOP'\" -1  "); } else
    if (strcmp(param,"reward")==0) { strcpy(commandToRun,"rostopic pub /Command HobbitMsgs/Command \"command: 'C_REWARD'\" -1  "); } else
    if (strcmp(param,"cancel")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_SPEAK' , params: [ {name: 'CANCEL' , value: ''} ] }\" -1\n"); } else

    if (strcmp(param,"call")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'E_CALLHOBBIT'\" -1  "); } else
    if (strcmp(param,"settings")==0) { strcpy(commandToRun,"rostopic pub /Command HobbitMsgs/Command \"command: 'F_SETTINGS'\" -1  "); } else
    if (strcmp(param,"closemic")==0) { strcpy(commandToRun,"rostopic pub /Command HobbitMsgs/Command \"command: 'F_ASR_OFF'\" -1  "); } else
    if (strcmp(param,"openmic")==0) { strcpy(commandToRun,"rostopic pub /Command HobbitMsgs/Command \"command: 'F_ASR_ON'\" -1  "); } else


    if (strcmp(param,"hobbit")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_WAKEUP'\" -1  "); } else
    if (strcmp(param,"wake")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_WAKEUP'\" -1  "); } else
    if (strcmp(param,"sleep")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_SLEEP'\" -1  "); } else
    if (strcmp(param,"clearfloor")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_CLEARFLOOR'\" -1  "); } else
    if (strcmp(param,"bringobject")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_BRING'\" -1  "); } else

           //name: Name     value: ΑΣΠΙΡΊΝΗ


    if (strcmp(param,"learnobject")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'C_LEARN'\" -1  "); } else
    if (strcmp(param,"helpme")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"command: 'G_FALL'\" -1  "); }

  }
   else
  if (strcmp(command,"say")==0)
  {
     char internalString[1024]={0};
     if (strcmp(param,"test")==0) {  strcpy(internalString,"Θα σας κάνω μια έκπληξη , θα σταματήσω να δουλεύω σε ένα τυχαίο σημείο.!"); } else
                                  {  strcpy(internalString,param); }


     replaceChar(internalString,'+',' ');

     //rostopic pub /ActionSequence HobbitMsgs/Command "{command: 'C_SPEAK' , params: [ name: 'INFO' , value: 'lobbit' ] }" -1
     sprintf(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_SPEAK' , params: [ {name: 'INFO' , value: '%s'} ] }\" -1\n",internalString);
  }

  if ( strlen(commandToRun)!=0 )
   {
     i=system(commandToRun);
     if (i!=0) { AmmServer_Error("Command %s failed\n",commandToRun); } else
               { AmmServer_Success("Command %s success\n",commandToRun); }
   } else
   {
     fprintf(stderr,"Execute with unknown command or parameter\n");
   }

  return ;
}



//This function prepares the content of  form context , ( content )
void * prepare_form_content_callback(char * content)
{
  strncpy(content,page,pageLength);
  content[pageLength]=0;
  form.content_size=pageLength;


  if  ( form.GET_request != 0 )
    {
      if ( strlen(form.GET_request)>0 )
       {
         char * bufferCommand = (char *) malloc ( 256 * sizeof(char) );
         if (bufferCommand!=0)
          {
            if ( _GET(default_server,&form,"head",bufferCommand,256) )  { execute("head",bufferCommand);  } else
            if ( _GET(default_server,&form,"hand",bufferCommand,256) )  { execute("hand",bufferCommand);  } else
            if ( _GET(default_server,&form,"body",bufferCommand,256) )
                 {
                     if (strcmp(bufferCommand,"joystick")==0)
                         {
                             float x=0.0,y=0.0;
                             char xString[256]={0};
                             char yString[256]={0};

                             if ( _GET(default_server,&form,"x",xString,256) ) { x=atof(xString); } else { AmmServer_Warning("Could not find X coord"); }
                             if ( _GET(default_server,&form,"y",yString,256) ) { y=atof(yString); } else { AmmServer_Warning("Could not find Y coord"); }

                             joystickExecute(x,y);
                           //Parse joystick command
                         } else
                         {
                           execute("body",bufferCommand);
                         }
                 } else
            if ( _GET(default_server,&form,"bring",bufferCommand,256) )  { execute("bring",bufferCommand);  } else
            if ( _GET(default_server,&form,"robot",bufferCommand,256) ) { execute("robot",bufferCommand); } else
            if ( _GET(default_server,&form,"rtd",bufferCommand,256) ) { execute("rtd",bufferCommand); } else
            if ( _GET(default_server,&form,"say",bufferCommand,256) )   { execute("say",bufferCommand);   }

            free(bufferCommand);
          }

       }
    }


  //form.content_size=strlen(content);
  return 0;
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(default_server,&indexPage,"/index.html",webserver_root,4096,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding stats page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&stats,"/stats.html",webserver_root,4096,0,&prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding stats page\n"); }

  page=AmmServer_ReadFileToMemory("src/ScriptRunner/controlpanel.html",&pageLength);
  if (! AmmServer_AddResourceHandler(default_server,&form,"/controlpanel.html",webserver_root,pageLength+1,0,&prepare_form_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding form testing page\n"); }

  AmmServer_DoNOTCacheResourceHandler(default_server,&form);
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
             //usleep(120000);
             sleep(1);
           }


    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(default_server);

    return 0;
}
