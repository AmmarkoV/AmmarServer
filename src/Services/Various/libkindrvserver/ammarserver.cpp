
/***************************************************************************
 *  ammarserver.cpp - KinDrv example - simulate joystick movement
 *
 *  Created: Fri Oct 11 00:031:00 2013
 *  Copyright  2013  Bahram Maleki-Fard
 ****************************************************************************/

/*  This file is part of libkindrv.
 *
 *  libkindrv is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libkindrv is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libkindrv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libkindrv/kindrv.h>

#include <stdio.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "AmmarServer/src/AmmServerlib/AmmServerlib.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;

struct AmmServer_RH_Context indexPage={0};
struct AmmServer_RH_Context stats={0};

using namespace KinDrv;

JacoArm *arm;

int
goto_retract(JacoArm *arm)
{
  // this can only be achieved from HOME position. Otherwise the arm
  // will move to HOME. You'll probably need to uncomment the gripper movements
  // in order for this to work. Or even better, implement moving to HOME position,
  // which could be triggered before going to RETRACT ;)
  jaco_retract_mode_t mode = arm->get_status();
  switch( mode ) {
    case MODE_READY_TO_RETRACT:
      // is currently on the way to RETRACT. Need 2 button presses,
      // 1st moves towards HOME, 2nd brings it back to its way to RETRACT
      arm->push_joystick_button(2);
      arm->release_joystick();
      arm->push_joystick_button(2);
      break;

    case MODE_READY_STANDBY:
    case MODE_RETRACT_TO_READY:
      // just 1 button press needed
      arm->push_joystick_button(2);
      break;

    case MODE_NORMAL_TO_READY:
    case MODE_NORMAL:
    case MODE_NOINIT:
      printf("cannot go from NORMAL/NOINIT to RETRACT \n");
      return 0;
      break;

    case MODE_ERROR:
      printf("some error?! \n");
      return 0;
      break;

    case MODE_RETRACT_STANDBY:
      printf("nothing to do here \n");
      return 1;
      break;
  }

  while( mode != MODE_RETRACT_STANDBY ) {
    usleep(1000*10); // 10 ms
    mode = arm->get_status();
  }
  arm->release_joystick();

  return 1;
}


int
goto_home(JacoArm *arm)
{
  // going to HOME position is possible from all positions. Only problem is,
  // if there is some kinfo of error
  jaco_retract_mode_t mode = arm->get_status();
  switch( mode ) {
    case MODE_RETRACT_TO_READY:
      // is currently on the way to HOME. Need 2 button presses,
      // 1st moves towards RETRACT, 2nd brings it back to its way to HOME
      arm->push_joystick_button(2);
      arm->release_joystick();
      arm->push_joystick_button(2);
      break;

    case MODE_NORMAL_TO_READY:
    case MODE_READY_TO_RETRACT:
    case MODE_RETRACT_STANDBY:
    case MODE_NORMAL:
    case MODE_NOINIT:
      // just 1 button press needed
      arm->push_joystick_button(2);
      break;

    case MODE_ERROR:
      printf("some error?! \n");
      return 0;
      break;

    case MODE_READY_STANDBY:
      printf("nothing to do here \n");
      return 1;
      break;
  }

  while( mode != MODE_READY_STANDBY ) {
    usleep(1000*10); // 10 ms
    mode = arm->get_status();
    if( mode == MODE_READY_TO_RETRACT ) {
      arm->release_joystick();
      arm->push_joystick_button(2);
    }
  }
  arm->release_joystick();

  return 1;
}



int mainJACKO()
{





  printf("Move arm back to RETRACT position \n");
  if( !goto_retract(arm) ) {
    printf("Failed to go to RETRACT. Go to HOME first \n");
    if( goto_home(arm) ) {
      printf("Try RETRACT again\n");
      goto_retract(arm);
    } else {
      printf("Also failed going to HOME. Might be some serious problem...hmm \n");
    }
  }

  return 0;
}



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



//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  fprintf(stderr,"Trying to write dynamic request to %p , max size %lu \n",rqst->content , rqst->MAXcontentSize);

  char cmd[128]={0};   int haveCmd=0;
  AmmServer_Warning("New Command message");


   if ( _GETcpy(rqst,"cmd",cmd,128) )
             {
               jaco_joystick_axis_t axes = {0};
               haveCmd=1;
               // we want to start from home_position
               if (strcmp(cmd,"home")==0)
                  { goto_home(arm); } else
               if (strcmp(cmd,"forth")==0)
                  {
                    // need cartesian-control for joystick simulation.
                    // Angular-control is also possible, then you would control each joint!
                    arm->set_control_cart();
                    usleep(1e6);
                    printf("Sending joystick movements. We want the arm to: \n");
                    printf("* translate forth \n");
                    axes.trans_fb = -2.5;
                    arm->move_joystick_axis(axes);
                    usleep(2e6);
                    arm->release_joystick();
                  } else
               if (strcmp(cmd,"down")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* translate back \n");
                     axes.trans_fb = 2.5;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                     axes.trans_fb = 0.f;
                  } else
               if (strcmp(cmd,"left")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* translate left \n");
                     axes.trans_lr = 2.5f;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                  } else
               if (strcmp(cmd,"right")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* translate right \n");
                     axes.trans_lr = -2.5f;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                     axes.trans_lr = 0.f;
                  } else
               if (strcmp(cmd,"up")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* translate up \n");
                     axes.trans_rot = 2.5;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                  } else
               if (strcmp(cmd,"down")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* translate down \n");
                     axes.trans_rot = -2.5;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                  } else
               if (strcmp(cmd,"wforth")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* wrist forth \n");
                     axes.wrist_lr = 2.5;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                  } else
                if (strcmp(cmd,"wback")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* wrist back \n");
                     axes.wrist_lr = -2.5;
                     arm->move_joystick_axis(axes);
                     usleep(2e6);
                     arm->release_joystick();
                     axes.wrist_lr = 0.f;
                  } else
                if (strcmp(cmd,"ccw")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* rotate counter-clockwise \n");
                     axes.wrist_rot = 2.5;
                     arm->move_joystick_axis(axes);
                     usleep(3e6);
                     arm->release_joystick();
                  } else
                if (strcmp(cmd,"cw")==0)
                  {
                     // need cartesian-control for joystick simulation.
                     // Angular-control is also possible, then you would control each joint!
                     arm->set_control_cart();
                     usleep(1e6);
                     printf("Sending joystick movements. We want the arm to: \n");
                     printf("* rotate counter-clockwise \n");
                     axes.wrist_rot = -2.5;
                     arm->move_joystick_axis(axes);
                     usleep(3e6);
                     arm->release_joystick();
                  }




             }


  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  snprintf(rqst->content,rqst->MAXcontentSize, //<meta http-equiv=\"refresh\" content=\"1\">
           "<html><head><title>Kinova Control</title></head>\
            <body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>\
            <a href=\"control.html?cmd=home\">Home</a>\
            <a href=\"control.html?cmd=forth\">Forth</a>\
            <a href=\"control.html?cmd=down\">Down</a>\
            <a href=\"control.html?cmd=left\">Left</a>\
            <a href=\"control.html?cmd=right\">Right</a><br>\
            <a href=\"control.html?cmd=wforth\">Wrist Forth</a>\
            <a href=\"control.html?cmd=wdown\">Wrist Down</a>\
            <a href=\"control.html?cmd=ccw\">Wrist CCW</a>\
            <a href=\"control.html?cmd=cw\">Wrist CW</a>\
            </body></html>",
           tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  strncpy(rqst->content,"<html><head><title>Kinova</title</head><body>TEST</body></html>",rqst->MAXcontentSize);

  rqst->contentSize=strlen(rqst->content);
  return 0;
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddResourceHandler(default_server,&stats,"/control.html",16000,0,(void*) &prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&indexPage,"/index.html",4096,0,(void*) &prepare_index_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);
}
/*! Dynamic content code ..! END ------------------------*/


int initializeArm()
{
 printf("KinDrv example for controlling the arm \n");

  // explicitly initialize a libusb context; optional
  KinDrv::init_usb();



  printf("Create a JacoArm \n");
  try {
    arm = new JacoArm();
    printf("Successfully connected to arm! \n");
  } catch( KinDrvException &e ) {
    printf("error %i: %s \n", e.error(), e.what());
    return 0;
  }

  printf("Gaining API control over the arm \n");
  arm->start_api_ctrl();



  //check if we need to initialize arm
  jaco_retract_mode_t mode = arm->get_status();
  printf("Arm is currently in state: %i \n", mode);
  if( mode == MODE_NOINIT ) {
    //push the "HOME/RETRACT" button until arm is initialized
    jaco_joystick_button_t buttons = {0};
    buttons[2] = 1;
    arm->push_joystick_button(buttons);

    while( mode == MODE_NOINIT ) {
      usleep(1000*10); // 10 ms
      mode = arm->get_status();
    }

    arm->release_joystick();
  }
  printf("Arm is initialized now, state: %i \n", mode);

}


int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal((void*) &close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "Kinova",
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

    initializeArm();



    // we want to start from home_position
    goto_home(arm);

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

    // explicitly close libusb context (only needed if explicitly openede before)
    KinDrv::close_usb();
    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}
