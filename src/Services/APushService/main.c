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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "device.h"

#include "../../AmmServerlib/AmmServerlib.h"

#define DEFAULT_BINDING_PORT 8087  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * server=0;
struct AmmServer_RH_Context pushDataCtx={0};
struct AmmServer_RH_Context alarmDataCtx={0};
struct AmmServer_RH_Context indexDataCtx={0};


//This function prepares the content of  random_chars context , ( random_chars.content )
void * index_data_callback(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><head><title>APushService</title></head><body>",rqst->MAXcontentSize);
  strcat(rqst->content,"<center><br><br><h2>You have reached APushService</h2><br>\
  Get the source code <a href=\"https://github.com/AmmarkoV/AmmarServer/tree/master/src/Services/APushService\">here</a>\
  <br></center></body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


int logTemperature(
                   const char * filename,
                   const char * deviceID,
                   const char * temperature,
                   const char * humidity
                  )
{
 FILE * fp = fopen(filename,"a");
 if (fp!=0)
 {
   fprintf(fp,"%s|%s|%s\n",deviceID,temperature,humidity);
   fclose(fp);
   return 1;
 }
 return 0;
}


int sendEmail(
               const char * receipient,
               const char * subject,
               const char * message
             )
{
  char messageBuffer[512]={0};
  char result[512]={0};

  snprintf(messageBuffer,512,"printf \"Subject:%s\n\n%s\" | ssmtp %s",subject,message,receipient);
  if (filterStringForShellInjection(messageBuffer,512))
  {
   if ( AmmServer_ExecuteCommandLine(messageBuffer,result,512) )
   {
     fprintf(stderr,"Successfully sent message to %s..\n",receipient);
     return 1;
   }
  }


 AmmServer_Error("Failed to send message to %s..\n",receipient);
 return 0;
}

//This function prepares the content of  random_chars context , ( random_chars.content )
void * push_alarm_callback(struct AmmServer_DynamicRequest  * rqst)
{
  int haveDeviceID=0;
  char deviceID[129]={0};
  if ( _GETcpy(rqst,"i",deviceID,128) )
              {
                fprintf(stderr,"Device %s alarm!\n",deviceID);
                haveDeviceID=1;
              }

  int haveDeviceKey=0;
  char devicePublicKey[32]={0};
  if ( _GETcpy(rqst,"k",devicePublicKey,128) )
              {
                haveDeviceKey=1;
              }



  char data[128]={0};
  if ( _GETcpy(rqst,"data",data,128) )
              {
                fprintf(stderr,"Data: %s \n",data);
              }

  if (
       (haveDeviceID)&&
       (haveDeviceKey)
     )
     {
      if (isDeviceAutheticated(deviceID,devicePublicKey))
       {
          if (
              sendEmail(
                          "ammarkov@gmail.com",
                           "APushService Alert",
                          data
                        )
              )
              {
                strncpy(rqst->content,"<html><body>OK</body></html>",rqst->MAXcontentSize);
                rqst->contentSize=strlen(rqst->content);
                return 0;
              }
       }
     }

  //FAILED state..
  strncpy(rqst->content,"<html><body>FAILED</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * push_data_callback(struct AmmServer_DynamicRequest  * rqst)
{
  int haveDeviceID=0;
  char deviceID[129]={0};
  if ( _GETcpy(rqst,"i",deviceID,128) )
              {
                fprintf(stderr,"Device %s connected\n",deviceID);
                haveDeviceID=1;
              }

  int haveDeviceKey=0;
  char devicePublicKey[32]={0};
  if ( _GETcpy(rqst,"k",devicePublicKey,128) )
              {
                haveDeviceKey=1;
              }


  char temperature[128]={0};
  if ( _GETcpy(rqst,"temperature",temperature,128) )
              {
                fprintf(stderr,"Temperature %s \n",temperature);
              }


  char humidity[128]={0};
  if ( _GETcpy(rqst,"humidity",humidity,128) )
              {
                fprintf(stderr,"Humidity %s \n",humidity);
              }


  if (
       (haveDeviceID)&&
       (haveDeviceKey)
     )
     {
      if (isDeviceAutheticated(deviceID,devicePublicKey))
       {
        if (
            logTemperature(
                           "temperature.log",
                           deviceID,
                           temperature,
                           humidity
                          )
           )
         {
          strncpy(rqst->content,"<html><body>OK</body></html>",rqst->MAXcontentSize);
          rqst->contentSize=strlen(rqst->content);
          return 0;
         }
       }
     }

  //FAILED state..
  strncpy(rqst->content,"<html><body>FAILED</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddResourceHandler(server,&pushDataCtx,"/push.html",4096,0,&push_data_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&alarmDataCtx,"/alarm.html",4096,0,&push_alarm_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&indexDataCtx,"/index.html",4096,0,&index_data_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(server,&pushDataCtx,1);
    AmmServer_RemoveResourceHandler(server,&alarmDataCtx,1);
    AmmServer_RemoveResourceHandler(server,&indexDataCtx,1);
}
/*! Dynamic content code ..! END ------------------------*/




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    server = AmmServer_StartWithArgs(
                                             "APushService",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );


    if (!server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    //Create dynamic content allocations and associate context to the correct files
    init_dynamic_content();
    //stats.html and formtest.html should be availiable from now on..!

    if (!readDeviceAuthorizationList("authorization.list"))
    {
      AmmServer_Error("Could not access authorization list..\n");
    }

         while ( (AmmServer_Running(server))  )
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
    AmmServer_Stop(server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}