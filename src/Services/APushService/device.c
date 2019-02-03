#include <stdio.h>
#include <string.h>
#include "device.h"

#include "utilities.h"

#if __has_include("configuration.h") && __has_include(<stdint.h>)
 #include "configuration.h"  //If you don't have configuration.h then comment this line and uncomment the previous one
 #else
 #include "configurationDefault.h"
#endif

#include "../../CSVParser/csvparse.h"
#include "../../InputParser/InputParser_C.h"

#define USE_STATIC_DEVICE_SETTINGS 1

const char * deviceClassName[] =
{
    "Unknown"   	,
    "Thermometer" 	,
   //--------------------------------------------
    "NUMBER_OF_DEVICE_CLASSES"
};


int printDeviceList(struct deviceList * dl)
{
  return 0;
}


int readDeviceAuthorizationList(struct deviceList * dl,const char * filename)
{
   unsigned int dev=0;


 #if USE_STATIC_DEVICE_SETTINGS
   dl->device[dev].deviceNeedsPlot=0;
   dl->device[dev].deviceIsDead=0;
   dl->device[dev].deviceCommunicatingProperly=1;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_THERMOMETER;
   snprintf(dl->device[dev].deviceID,32,"0001");
   snprintf(dl->device[dev].devicePrivateKey,32,"xxxx");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"RD Sensor");
   snprintf(dl->device[dev].email,512,EMAIL_LIST_BIO);


   ++dev;
   dl->device[dev].deviceNeedsPlot=0;
   dl->device[dev].deviceIsDead=1;
   dl->device[dev].deviceCommunicatingProperly=0;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_THERMOMETER;
   snprintf(dl->device[dev].deviceID,32,"0002");
   snprintf(dl->device[dev].devicePrivateKey,32,"xxxx");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"PR Sensor");
   snprintf(dl->device[dev].email,512,EMAIL_LIST_BIO);


   ++dev;
   dl->device[dev].deviceNeedsPlot=0;
   dl->device[dev].deviceIsDead=1;
   dl->device[dev].deviceCommunicatingProperly=0;
   dl->device[dev].lastContact=0;
   dl->device[dev].deviceClass=DEVICE_UNKOWN;
   snprintf(dl->device[dev].deviceID,32,"DUMMY");
   snprintf(dl->device[dev].devicePrivateKey,32,"xxxx");
   snprintf(dl->device[dev].devicePublicKey,32,"xxxx");
   snprintf(dl->device[dev].deviceLabel,32,"Dummy Sensor");
   snprintf(dl->device[dev].email,512,"local");

   dl->numberOfDevices=dev+1;
   return 1;
#else
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  unsigned int lineCounter=0;
  unsigned int i;

  struct CSVParser *  csvParser = csvParserCreate("|",1);
  if (csvParser!=0)
  {
    if (csvParser_StartParsingFile(csvParser,"db/pushService.devices"))
    {
       unsigned int numberOfLines = csvParser_CountNumberOfLines(csvParser,"db/pushService.devices");
       fprintf(stderr,"Number of Lines %u \n",numberOfLines);

       while (csvParser_ParseNextLine(csvParser))
       {
         if (lineCounter==0)
         {
           //Labels..
         } else
         if (i<MAXIMUM_NUMBER_OF_ACCOUNTS)
         {
          accNumber = lineCounter;
          account = &acc->account[accNumber];

          for (i=0; i<csvParser_GetNumberOfFields(csvParser); i++)
          {
           if (csvParser_GetField(csvParser,i))
           {
             switch (i)
             {
               case 0: snprintf(account->accountUsername,32,"%s",csvParser_GetField(csvParser,i)); break;
               case 1: snprintf(account->accountPassword,32,"%s",csvParser_GetField(csvParser,i)); break;
               case 2: snprintf(account->email,512,"%s",csvParser_GetField(csvParser,i));          break;
               case 3: fprintf(stderr,"NoSerial %u/%u : %s\n",lineCounter,i,csvParser_GetField(csvParser,i));   break;
             }
             fprintf(stderr,"!!LINE %u/FIELD %u : %s\n",lineCounter,i,csvParser_GetField(csvParser,i));
           }
          }

         }

         ++lineCounter;
       }
      csvParser_StopParsingFile(csvParser);
      fprintf(stderr,"Successfully finished reading accounts ..\n");
      acc->numberOfAccounts=accNumber;
      csvParserDestroy(csvParser);
      return 1;
    } else
    { fprintf(stderr,"Could not open accounts csv file..\n"); }
    csvParserDestroy(csvParser);
  } else
  { fprintf(stderr,"Could not allocate accounts csvParser object..\n"); }
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
#endif


  return 0;
}


int saveDeviceState(struct deviceList * dl,const char * filename)
{
 FILE * fp = fopen(filename,"a");
 if (fp!=0)
 {
  unsigned int devID=0;
  for (devID=0; devID<dl->numberOfDevices; devID++)
  {





  }
    return 1;
 }

 fprintf(stderr,"Failed to saveDeviceState\n");
 return 0;
}










int getDeviceID(struct deviceList * dl,const char * serialNumber,deviceID * devID)
{
   unsigned int i=0;
   for (i=0; i<dl->numberOfDevices; i++)
   {
     if (strcmp(dl->device[i].deviceID,serialNumber)==0)
     {
       *devID=i;
       return 1;
     }
   }

   return 0;
}



int updateDeviceHeartbeat(struct deviceObject *device,char alarmed,float temperature,float humidity)
{
 if (device==0) { return 1; }
  device->lastContact = time(NULL);
  device->info.alarm=alarmed;
  device->info.sensors[0]=humidity;
  device->info.temperatures[0]=temperature;
  device->deviceCommunicatingProperly=1;
  device->deviceIsDead=0;
 return 1;
}


int checkForDeadDevices(struct deviceList *dl)
{
   unsigned int newlyDisconnectedDevicesFound=0;
   fprintf(stderr,"Checking for disconnected devices starting..\n");
   if (dl==0) { return 0; }

   time_t clock = time(NULL);
   struct tm * ptm = localtime( &clock );  //gmtime

   char message[512];


   unsigned int i=0;
   for (i=0; i<dl->numberOfDevices; i++)
   {
      struct deviceObject * device = &dl->device[i];
      if (device->lastContact==0)
      {
        //Don't bother with devices never actived..
      } else
      if (clock - device->lastContact > TIME_IN_SECONDS_TO_PRONOUNCE_A_DEVICE_LOST )
      {
          if ( (device->deviceCommunicatingProperly) && (!device->deviceIsDead) )
          {
            //We just lost this device..
            ++newlyDisconnectedDevicesFound;
            device->deviceCommunicatingProperly=0;

            snprintf(
                     message,512,"The sensor `%s` has been disconnected  @ %u/%u/%u %u:%u:%u Hopefully this is just a power black-out or temporary network problem.",
                     device->deviceLabel,
                     ptm->tm_mday,
                     1+ptm->tm_mon,
                     EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,
                     ptm->tm_hour,
                     ptm->tm_min,
                     ptm->tm_sec
                    );

            if ( sendEmail( device->email, "Sensor Disconnected", message ) )
               {
                device->lastEMailNotification=clock;
               }
          } else
          if ( (!device->deviceCommunicatingProperly) && (!device->deviceIsDead) )
          {
            //We have already lost the device, do not keep spamming
            if (clock - device->lastContact > TIME_IN_SECONDS_TO_PRONOUNCE_A_DEVICE_DEAD )
            {
              ++newlyDisconnectedDevicesFound;
              //If it is so much time
              device->deviceIsDead=1;

              snprintf(message,512,"The sensor `%s` has been disconnected for a very long time..! @ %u/%u/%u %u:%u:%u This could be a hardware problem.",
                    device->deviceLabel,ptm->tm_mday,1+ptm->tm_mon,EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

              if ( sendEmail( device->email, "Sensor Dead?", message ) )
               {
                device->lastEMailNotification=clock;
               }
            }
          } else
          {
            //DEAD DEVICE..
          }
      }//-------------------------------------------------------------
   }


   fprintf(stderr,"DeviceCheck: ");
   unsigned int alive=0,dead=0,disconnected=0;
   for (i=0; i<dl->numberOfDevices; i++)
   {
      struct deviceObject * device = &dl->device[i];
      if (device->lastContact==0)  { } else
      if (device->deviceCommunicatingProperly)    { ++alive; } else
      if (device->deviceIsDead)    { ++dead; }
   }

   fprintf(stderr,"%u alive/%u dead/%u disconnected/%u newly disconnected\n ",alive,dead,disconnected,newlyDisconnectedDevicesFound);
   return 1;
}


int isDeviceAutheticated(const char * deviceID, const char * devicePublicKey)
{
  return 1;
}
