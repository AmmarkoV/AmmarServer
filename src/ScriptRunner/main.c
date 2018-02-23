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

#define ENABLE_PASSWORD_PROTECTION 0
#define ENABLE_CHAT_BOX 0

#define MAX_COMMAND_SIZE 2048

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
struct AmmServer_RH_Context settings={0};
struct AmmServer_RH_Context stats={0};
struct AmmServer_RH_Context form={0};
struct AmmServer_RH_Context chatbox={0};
struct AmmServer_RH_Context base_image={0};
struct AmmServer_RH_Context top_image={0};
struct AmmServer_RH_Context random_chars={0};

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
void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  strcpy(rqst->content,"<html><head><title>Welcome to The Hobbit WebInterface</title>\
                                     <meta http-equiv=\"refresh\" content=\"0; url=controlpanel.html\" />\
                  <body><center><br><br><br>\
                   <h1>The incredibly minimal WebInterface for Hobbit</h1><br><br>\
                   <h3><a href=\"controlpanel.html\">Click Here for remote operation control panel</a></h3>\
                   <h3><a href=\"facilitator_panel.html\">Click Here for facilitator panel</a></h3>\
                   </body></html> ");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}



int getBackCommandLine(char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize)
{
 FILE *fp;
 /* Open the command for reading. */
 fp = popen(command, "r");
 if (fp == 0 )
       {
         fprintf(stderr,"Failed to run command (%s) \n",command);
         return 0;
       }

 /* Read the output a line at a time - output it. */
  unsigned int i=0;
  while (fgets(what2GetBack, what2GetBackMaxSize , fp) != 0)
    {
        ++i;
        //fprintf(stderr,"\n\nline %u = %s \n",i,output);
        break;
    }
  /* close */
  pclose(fp);
  return 1;
}



//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

/*
rostopic echo /battery_state
voltage: 26.0810012817
current: 3.8780002594
lifePercent: 41
lifeTime: -1
charging: False
powerSupplyPresent: False
*/

  char batteryState[MAX_COMMAND_SIZE]={0};
  getBackCommandLine("timeout 1 rostopic echo /battery_state -n 1 | grep lifePercent | cut -d ':' -f2", batteryState , MAX_COMMAND_SIZE );
  char chargingState[MAX_COMMAND_SIZE]={0};
  getBackCommandLine("timeout 1 rostopic echo /battery_state -n 1 | grep charging | cut -d ':' -f2", chargingState , MAX_COMMAND_SIZE );
  char mileageState[MAX_COMMAND_SIZE]={0};
  getBackCommandLine("timeout 1 rostopic echo /mileage -n 1 | grep data | cut -d ':' -f2", mileageState , MAX_COMMAND_SIZE );

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  snprintf(rqst->content,rqst->MAXcontentSize,
            "<html><head><body>Time is<br> %02d-%02d-%02d %02d:%02d:%02d\n <br>Battery is : %s<br>Charging : %s<br>Mileage : %s<br> <br></body></html>",
                    tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec,batteryState,chargingState,mileageState);

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_base_image(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int length=0;
  char * readContent = AmmServer_ReadFileToMemory("/opt/ros/hobbit_hydro/src/rgbd_acquisition/bin/frames/base/left0000.jpg",&length);

  if(readContent==0)
  {
     snprintf(rqst->content,rqst->MAXcontentSize,"<html><body><h1>Bad Image</h1></body></html>");
     rqst->contentSize=strlen(rqst->content);
  } else
  {
     if (rqst->MAXcontentSize<length)
     {
        length = rqst->MAXcontentSize;
        fprintf(stderr,"Error , not enough space to accomodate image \n");
     }
     memcpy(rqst->content,readContent,length);
     rqst->contentSize=length;
     free(readContent);
  }
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_top_image(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int length=0;
  char * readContent = AmmServer_ReadFileToMemory("/opt/ros/hobbit_hydro/src/rgbd_acquisition/bin/frames/top/left0000.jpg",&length);

  if(readContent==0)
  {
     snprintf(rqst->content,rqst->MAXcontentSize,"<html><body><h1>Bad Image</h1></body></html>");
     rqst->contentSize=strlen(rqst->content);
  } else
  {
     if (rqst->MAXcontentSize<length)
     {
        length = rqst->MAXcontentSize;
        fprintf(stderr,"Error , not enough space to accomodate image \n");
     }
     memcpy(rqst->content,readContent,length);
     rqst->contentSize=length;
     free(readContent);
  }
  return 0;
}


void joystickExecute(float x , float y )
{
   AmmServer_Warning("Joystick(%0.2f,%0.2f)\n",x,y);

   char commandToRun[MAX_COMMAND_SIZE]={0};
    snprintf(commandToRun,MAX_COMMAND_SIZE,
           "rostopic pub /joy sensor_msgs/Joy \"{ axes: [ %0.2f , %0.2f , 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 ] , buttons: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] }\" -1&"
            ,x,y
           );

    int i=system(commandToRun);
    if (i!=0) { AmmServer_Error("Command %s failed\n",commandToRun); } else
               { AmmServer_Success("Command %s success\n",commandToRun); }

}


/*
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
                     IF SOMEONE WANTS TO ADD SOMETHING JUST ADD TO THE FOLLOWING ( execute("command","param" )
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
*/
void execute(char * command,char * param)
{
  fprintf(stderr,"Execute(%s,%s) \n",command,param);
  int i=0;
  char commandToRun[MAX_COMMAND_SIZE]={0};



  if (strcmp(command,"node")==0)
  {
    if (strcmp(param,"niteTrigger")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /rgbd_acquisition/trigger_peopletracker\" "); } else
    if (strcmp(param,"nitePause")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /rgbd_acquisition/pause_peopletracker\" "); } else
    if (strcmp(param,"niteResume")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /rgbd_acquisition/resume_peopletracker\" "); } else
    if (strcmp(param,"nitePausePoint")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /rgbd_acquisition/pause_pointing_gesture_messages\" "); } else
    if (strcmp(param,"niteResumePoint")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /rgbd_acquisition/resume__pointing_gesture_messages\" "); } else

    if (strcmp(param,"forthSKPause")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /skeleton_detector/pause\" "); } else
    if (strcmp(param,"forthSKResume")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /skeleton_detector/resume\" "); } else

    if (strcmp(param,"emergencyTrigger")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /emergency_detector/trigger\" "); } else
    if (strcmp(param,"emergencyPause")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /emergency_detector/pause\" "); } else
    if (strcmp(param,"emergencyResume")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /emergency_detector/resume\" "); } else

    if (strcmp(param,"gesturePause")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /hand_gestures/pause\" "); } else
    if (strcmp(param,"gestureResume")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /hand_gestures/resume\" "); } else

    if (strcmp(param,"faceTrigger")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /face_detection/trigger\" "); } else
    if (strcmp(param,"facePause")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /face_detection/pause\" "); } else
    if (strcmp(param,"faceResume")==0) {  strcpy(commandToRun,"/bin/bash -c \"rosservice call /face_detection/resume\" "); } else
                                      { fprintf(stderr,"Unknown node command ( param %s ) \n", param); }

  } else

  if (strcmp(command,"camera")==0)
  {
    if (strcmp(param,"refresh")==0)
        { strcpy(commandToRun,"/bin/bash -c \"cd /opt/ros/hobbit_hydro/src/rgbd_acquisition/bin/frames/base/ && timeout 1 rosrun image_view image_saver image:=/basecam/rgb/image_raw\" && cd /opt/ros/hobbit_hydro/src/rgbd_acquisition/bin/frames/top/ && timeout 1 rosrun image_view image_saver image:=/headcam/rgb/image_raw "); }
  } else
  if (strcmp(command,"head")==0)
  {
    if (strcmp(param,"default")==0)  {
                                        execute((char*)"head",(char*)"center_center");
                                        return;
                                     } else
    if (strcmp(param,"up_right")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"up_right\" -1\" "); } else
    if (strcmp(param,"up_center")==0)    { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"up_center\" -1\" ");    } else
    if (strcmp(param,"up_left")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"up_left\" -1\" ");  } else
    if (strcmp(param,"center_right")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"center_right\" -1\" "); } else
    if (strcmp(param,"center_center")==0)    { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"center_center\" -1\" ");    } else
    if (strcmp(param,"center_left")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"center_left\" -1\" ");  } else
    if (strcmp(param,"down_right")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"down_right\" -1\" "); } else
    if (strcmp(param,"down_center")==0)    { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"down_center\" -1\" ");    } else
    if (strcmp(param,"down_left")==0)  { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/move std_msgs/String \"down_left\" -1\" ");  }
  }
   else
  if (strcmp(command,"emotion")==0)
  {
    //HAPPY VHAPPY LTIRED VTIRED CONCERNED SAD WONDERING NEUTRAL SLEEPING
    if (strcmp(param,"happy")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"HAPPY\" -1\" "); } else
    if (strcmp(param,"vhappy")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"VHAPPY\" -1\" "); } else
    if (strcmp(param,"ltired")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"LTIRED\" -1\" "); } else
    if (strcmp(param,"vtired")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"VTIRED\" -1\" "); } else
    if (strcmp(param,"concerned")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"CONCERNED\" -1\" "); } else
    if (strcmp(param,"sad")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"SAD\" -1\" "); } else
    if (strcmp(param,"wondering")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"WONDERING\" -1\" "); } else
    if (strcmp(param,"neutral")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"NEUTRAL\" -1\" "); } else
    if (strcmp(param,"sleeping")==0) { strcpy(commandToRun,"/bin/bash -c \"rostopic pub /head/emo std_msgs/String \"SLEEPING\" -1\" "); }
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
    if (strcmp(param,"reset")==0) { strcpy(commandToRun,"rosservice call /reset_motorstop service,mira_msgs::ResetMotorStop::Request"); } else
    if (strcmp(param,"360")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Turn 360'\" -1"); } else
    if (strcmp(param,"360ccw")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Turn 360'\" -1"); } else
    if (strcmp(param,"360cw")==0) { strcpy(commandToRun,"rostopic pub /DiscreteMotionCmd std_msgs/String \"data: 'Turn -360'\" -1"); } else
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
  if (strcmp(command,"move")==0)
  {
    if (strcmp(param,"charging")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_BRING' , params: [ {name: 'Name' , value: 'ΑΣΠΙΡΊΝΗ'} ] }\" -1  "); } else
    if (strcmp(param,"kitchen")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_BRING' , params: [ {name: 'Name' , value: 'ΑΣΠΙΡΊΝΗ'} ] }\" -1  "); } else
    if (strcmp(param,"livingroom")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_BRING' , params: [ {name: 'Name' , value: 'ΑΣΠΙΡΊΝΗ'} ] }\" -1  "); } else
    if (strcmp(param,"bedroom")==0) { strcpy(commandToRun,"rostopic pub /ActionSequence HobbitMsgs/Command \"{command: 'C_BRING' , params: [ {name: 'Name' , value: 'ΑΣΠΙΡΊΝΗ'} ] }\" -1  "); }
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
     char internalString[MAX_COMMAND_SIZE]={0};
     if (strcmp(param,"test")==0) {  strcpy(internalString,"Θα σας κάνω μια έκπληξη , θα σταματήσω να δουλεύω σε ένα τυχαίο σημείο.!"); } else
                                  {  strcpy(internalString,param); }


     replaceChar(internalString,'+',' ');

     //rostopic pub /ActionSequence HobbitMsgs/Command "{command: 'C_SPEAK' , params: [ name: 'INFO' , value: 'lobbit' ] }" -1
     snprintf(commandToRun,MAX_COMMAND_SIZE,"rostopic pub /ActionSequence hobbit_msgs/Command \"{command: 'C_SPEAK' , params: [ {name: 'INFO' , value: '%s'} ] }\" -1\n",internalString);
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


/*
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
                     IF SOMEONE WANTS TO ADD SOMETHING JUST ADD TO THE PREVIOUS FUNCTION  ( execute("command","param" )
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
      ===========================================================================================================================
*/



//This function prepares the content of  form context , ( content )
void * store_new_configuration_callback(struct AmmServer_DynamicRequest  * rqst)
{
  unsigned int successfullStore = 0;
  rqst->content[pageLength]=0; //Clear content

  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         if ( AmmServer_SaveDynamicRequest("hobbit_raw.ini",default_server,rqst) )
           {
             int i=system("tr \"\\&\" \"\\n\" < hobbit_raw.ini > hobbit.ini");
             if (i==0) {  successfullStore = 1; }
           }
       }
    }

if (successfullStore)
     {
         strcpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"3; url=controlpanel.html\" /></head>\
                  <body><center><br><br><br><h1>Settings stored..</h1><br><br><h3><a href=\"controlpanel.html\">You are beeing redirected to control panel</a></h3></body></html> ");
     } else
     {
         strcpy(rqst->content,"<html><body><center><br><br><br><h1>FAILED to store settings :( </h1><br><br><h3><a href=\"#\" onclick=\"javascript:window.history.go(-1)\">Click here to go back</a></h3></body></html> ");
     }

  rqst->contentSize=strlen(rqst->content);
  return 0;
}



//This function prepares the content of  form context , ( content )
void * prepare_form_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  if  ( rqst->GET_request != 0 )
    {
      if ( strlen(rqst->GET_request)>0 )
       {
         char * bufferCommand = (char *) malloc ( 256 * sizeof(char) );
         if (bufferCommand!=0)
          {
            if ( _GET(rqst,(char*)"node",bufferCommand,256) )  { execute((char*)"node",bufferCommand);  } else
            if ( _GET(rqst,(char*)"camera",bufferCommand,256) )  { execute((char*)"camera",bufferCommand);  } else
            if ( _GET(rqst,(char*)"head",bufferCommand,256) )  { execute((char*)"head",bufferCommand);  } else
            if ( _GET(rqst,(char*)"hand",bufferCommand,256) )  { execute((char*)"hand",bufferCommand);  } else
            if ( _GET(rqst,(char*)"emotion",bufferCommand,256) )  { execute((char*)"emotion",bufferCommand);  } else
            if ( _GET(rqst,(char*)"body",bufferCommand,256) )
                 {
                     if (strcmp(bufferCommand,"joystick")==0)
                         {
                             float x=0.0,y=0.0;
                             char xString[256]={0};
                             char yString[256]={0};

                             if ( _GET(rqst,(char*)"x",xString,256) ) { x=atof(xString); } else { AmmServer_Warning("Could not find X coord"); }
                             if ( _GET(rqst,(char*)"y",yString,256) ) { y=atof(yString); } else { AmmServer_Warning("Could not find Y coord"); }

                             joystickExecute(x,y);
                           //Parse joystick command
                         } else
                         {
                           execute((char*)"body",bufferCommand);
                         }
                 } else
            if ( _GET(rqst,(char*)"bring",bufferCommand,256) )  { execute((char*)"bring",bufferCommand);  } else
            if ( _GET(rqst,(char*)"move",bufferCommand,256) )  { execute((char*)"move",bufferCommand);  } else
            if ( _GET(rqst,(char*)"robot",bufferCommand,256) ) { execute((char*)"robot",bufferCommand); } else
            if ( _GET(rqst,(char*)"rtd",bufferCommand,256) ) { execute((char*)"rtd",bufferCommand); } else
            if ( _GET(rqst,(char*)"say",bufferCommand,256) )   { execute((char*)"say",bufferCommand);   } else
                                                                              { AmmServer_Warning("Could not find a recognizable command family to execute");  }


            free(bufferCommand);
          }

       }
    }

  if ( (page==0) || (pageLength==0) )
  {
     page=AmmServer_ReadFileToMemory((char*)"controlpanel.html",&pageLength);
  }
  strncpy(rqst->content,page,pageLength);
  rqst->content[pageLength]=0;
  rqst->contentSize=pageLength;

  //form.content_size=strlen(content);
  return 0;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
int  init_dynamic_content()
{
  fprintf(stderr,"Please note that control panel , is only refreshed once per startup for min resource consumption\n");
  page=AmmServer_ReadFileToMemory((char*)"controlpanel.html",&pageLength);
  if ( (page==0) || (pageLength==0) ) { fprintf(stderr,"Cannot initialize dynamic content\n"); return 0; }

  if (! AmmServer_AddResourceHandler(default_server,&form,(char*)"/controlpanel.html",webserver_root,pageLength+1,0,(void* ) &prepare_form_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding form testing page\n"); }
  AmmServer_DoNOTCacheResourceHandler(default_server,&form);

  if (! AmmServer_AddResourceHandler(default_server,&indexPage,(char*)"/index.html",webserver_root,4096,0,(void*) &prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding stats page\n"); }
  if (! AmmServer_AddResourceHandler(default_server,&stats,(char*) "/stats.html",webserver_root,4096,0,(void*) &prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { AmmServer_Warning("Failed adding stats page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&settings,(char*)"/settings.html",webserver_root,4096,0,(void* ) &store_new_configuration_callback,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding settings page\n"); }

  if (! AmmServer_AddResourceHandler(default_server,&base_image,(char*)"/base_image.jpg",webserver_root,640*480*3,0,(void* ) &prepare_base_image,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding prepare base_image page\n"); }
  //if (! AmmServer_AddResourceHandler(default_server,&top_image,(char*)"/top_image.jpg",webserver_root,640*480*3,0,(void* ) &prepare_top_image,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding prepare top_image page\n"); }
  return 1;
 }

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&indexPage,1);
    AmmServer_RemoveResourceHandler(default_server,&settings,1);
    AmmServer_RemoveResourceHandler(default_server,&form,1);
    AmmServer_RemoveResourceHandler(default_server,&base_image,1);
    AmmServer_RemoveResourceHandler(default_server,&top_image,1);

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


    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;


    default_server = AmmServer_StartWithArgs(
                                             "scriptrunner",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
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
    //stats.html and formtest.html should be availiable from now on..!
    if ( init_dynamic_content() )
        {
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
        }

    //Stop the server and clean state
    AmmServer_Stop(default_server);

    return 0;
}
