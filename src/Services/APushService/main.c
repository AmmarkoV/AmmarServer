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
#include "account.h"

#include "../../AmmServerlib/AmmServerlib.h"

#include "utilities.h"
#include "temperatureSensor.h"

#define DEFAULT_BINDING_PORT 8087  // <--- Change this to 80 if you want to bind to the default http port..!

//todo: Add TinyAES for encryption..
//https://github.com/kokke/tiny-AES-c


char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";
char logFile[]="temperatures.log";

struct AmmServer_MemoryHandler * accountPage=0;


//The decleration of some dynamic content resources..
struct AmmServer_Instance  * server=0;
struct AmmServer_RH_Context accountDataCtx={0};
struct AmmServer_RH_Context pushDataCtx={0};
struct AmmServer_RH_Context alarmDataCtx={0};
struct AmmServer_RH_Context testDataCtx={0};
struct AmmServer_RH_Context indexDataCtx={0};
struct AmmServer_RH_Context monitorImageCtx={0};



struct deviceList devices;
struct accountList accounts;


int authenticateDevice(struct AmmServer_DynamicRequest  * rqst,deviceID * devID)
{
 int haveDeviceSerial=0,haveDeviceKey=0;
  char deviceSerial[129]={0};
  char devicePublicKey[32]={0};

  if ( _GETcpy(rqst,"s",deviceSerial,128) )        { fprintf(stderr,"Device %s connected\n",deviceSerial); haveDeviceSerial=1; }
  if ( _GETcpy(rqst,"k",devicePublicKey,128) )     { haveDeviceKey=1; }

  if ( (haveDeviceSerial)||(haveDeviceKey) )
  {
    if ( getDeviceID(&devices,deviceSerial,devID) )
    {
      return 1;
    }
  }
 return 0;
}


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


//This function prepares the content of  random_chars context , ( random_chars.content )
void * viewAccountDevicesCallback(struct AmmServer_DynamicRequest  * rqst)
{
   struct AmmServer_MemoryHandler * response = AmmServer_CopyMemoryHandler(accountPage);

   AmmServer_ReplaceAllVarsInMemoryHandler(response,2,"xxxACCOUNTxxx","SAMPLE_ACCOUNT");


    char placeHolder[32];
    char color[32];
    char dateString[512];
    char deviceSummary[512];

    unsigned int i=0;
    for (i=0; i<devices.numberOfDevices; i++)
    {
      if (i%2==0) { snprintf(color,32,"#EEEEEE"); } else
                  { snprintf(color,32,"#FEFEFE"); }

      snprintf(placeHolder,32,"xxxDEVICE_%uxxx",i);

      if (devices.device[i].lastContact==0)
      {
       snprintf(dateString,512," <center>-</center> ");
      } else
      {
       struct tm * ptm = localtime(&devices.device[i].lastContact); //gmtime
       snprintf(dateString,512,"%u/%u/%u %02u:%02u:%02u",ptm->tm_mday,1+ptm->tm_mon,EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
      }

      snprintf(
             deviceSummary,
             512,
             "<tr bgcolor=\"%s\">\
                <td>%s</td>\
                <td>%s</td>\
                <td>%s</td>\
                <td>%s</td>\
                <td>%0.2f&deg;C</td>\
                <td>%0.2f%%</td>\
                <td><center><a href=\"#\">&copy;</a></center></td>\
               </tr>"
               ,
               color,
               devices.device[i].deviceLabel,
               devices.device[i].deviceID,
               deviceClassName[(int) devices.device[i].deviceClass],
               dateString,
               devices.device[i].info.temperatures[0],
               devices.device[i].info.sensors[0]
             );

      AmmServer_ReplaceAllVarsInMemoryHandler(response,1,placeHolder,deviceSummary);
    }


    for (i=devices.numberOfDevices; i<11; i++)
    {
      snprintf(placeHolder,32,"xxxDEVICE_%uxxx",i);
      snprintf(color,32,"                              ");
      AmmServer_ReplaceAllVarsInMemoryHandler(response,1,placeHolder,color);
    }


   //--------------------------------------------------------
   AmmServer_DynamicRequestReturnMemoryHandler(rqst,response);
   AmmServer_FreeMemoryHandler(&response);
  return 0;
}


///---------------------------------------------------------------------------------------------------------------------------------
//This function prepares the content of  random_chars context , ( random_chars.content )
void * generalTestCallback(struct AmmServer_DynamicRequest  * rqst)
{
  deviceID devID=0;
    if ( authenticateDevice(rqst,&devID) )
    {
       switch(devices.device[devID].deviceClass)
       {
         case DEVICE_UNKOWN       :
         break;
         case DEVICE_THERMOMETER  :
            if (temperatureSensorTestCallback(&devices.device[devID],rqst) ) { return 0; }
         break;
       }
    }

   //If we reached this point then we respond with a failure message
    generalFailureResponseToRequest(rqst);
   //---------------------------------------------------------------

 return 0;
}

///---------------------------------------------------------------------------------------------------------------------------------
//This function prepares the content of  random_chars context , ( random_chars.content )
void * generalAlarmCallback(struct AmmServer_DynamicRequest  * rqst)
{
  deviceID devID=0;
  if ( authenticateDevice(rqst,&devID) )
    {
       switch(devices.device[devID].deviceClass)
       {
         case DEVICE_UNKOWN       :
         break;
         case DEVICE_THERMOMETER  :
            if ( temperatureSensorAlarmCallback(&devices.device[devID],rqst) ) { return 0; }
         break;
       }
    }

   //If we reached this point then we respond with a failure message
    generalFailureResponseToRequest(rqst);
   //---------------------------------------------------------------

  return 0;
}

///---------------------------------------------------------------------------------------------------------------------------------
//This function prepares the content of  random_chars context , ( random_chars.content )
void * generalHeartBeatCallback(struct AmmServer_DynamicRequest  * rqst)
{
  deviceID devID=0;
  if ( authenticateDevice(rqst,&devID) )
    {
       switch(devices.device[devID].deviceClass)
       {
         case DEVICE_UNKOWN       :
         break;
         case DEVICE_THERMOMETER  :
            if ( temperatureSensorHeartBeatCallback(&devices.device[devID],rqst) ) { return 0; }
         break;
       }
    }

   //If we reached this point then we respond with a failure message
    generalFailureResponseToRequest(rqst);
   //---------------------------------------------------------------

  return 0;
}





///---------------------------------------------------------------------------------------------------------------------------------
//This function prepares the content of  random_chars context , ( random_chars.content )
void * generalMonitorImageCallback(struct AmmServer_DynamicRequest  * rqst)
{
  deviceID devID=0;
  if ( authenticateDevice(rqst,&devID) )
    {
       switch(devices.device[devID].deviceClass)
       {
         case DEVICE_UNKOWN       :
         break;
         case DEVICE_THERMOMETER  :
            if ( temperatureSensorPlotImageCallback(&devices.device[devID],rqst) ) { return 0; }
         break;
       }
    }

   //If we reached this point then we respond with a failure message
    generalFailureResponseToRequest(rqst);
   //---------------------------------------------------------------

  return 0;
}





///---------------------------------------------------------------------------------------------------------------------------------
//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddResourceHandler(server,&accountDataCtx,"/account.html",46000,0,&viewAccountDevicesCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&pushDataCtx,"/push.html",4096,0,&generalHeartBeatCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&alarmDataCtx,"/alarm.html",4096,0,&generalAlarmCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&testDataCtx,"/test.html",4096,0,&generalTestCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&monitorImageCtx,"/monitor.png",460000,0,&generalMonitorImageCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(server,&indexDataCtx,"/index.html",4096,0,&index_data_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);


  fprintf(stderr,"Reading account view file..\n");
  accountPage=AmmServer_ReadFileToMemoryHandler("src/Services/APushService/res/account.html");
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    fprintf(stderr,"Termination request received..\n");
    saveDeviceState(&devices,"db/pushService.state");

    AmmServer_RemoveResourceHandler(server,&accountDataCtx,1);
    AmmServer_RemoveResourceHandler(server,&pushDataCtx,1);
    AmmServer_RemoveResourceHandler(server,&alarmDataCtx,1);
    AmmServer_RemoveResourceHandler(server,&testDataCtx,1);
    AmmServer_RemoveResourceHandler(server,&monitorImageCtx,1);
    AmmServer_RemoveResourceHandler(server,&indexDataCtx,1);


    fprintf(stderr,"goodbye..\n");
}
/*! Dynamic content code ..! END ------------------------*/
///---------------------------------------------------------------------------------------------------------------------------------




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

    if (!readDeviceAuthorizationList(&devices,"db/pushServiceDevices.list"))
    {
      AmmServer_Error("Could not access device list..\n");
    }

    if (!readAccountList(&accounts,"db/pushServiceAccounts.list"))
    {
      AmmServer_Error("Could not access account list..\n");
    }

     while ( (AmmServer_Running(server))  )
           {
             sleep(160);
             checkForDeadDevices(&devices);
           }

    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}
