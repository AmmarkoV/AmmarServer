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
#include <sys/stat.h>
#include "../../AmmServerlib/AmmServerlib.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
#define DYNAMIC_PAGES_MEMORY_COMMITED 4096
#define MAX_QUERY_SIZE 4096
#define MAX_IMAGES_CONCURRENTLY 4

#define IMAGE_DIRECTORY "/home/user/workspace/outputs/txt2img-samples/samples/"
#define WEBSERVERROOT "public_html/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

int DEACTIVATE_EMAIL =1;

struct AmmServer_Instance  * default_server=0;
//------------------------------------------------
struct AmmServer_RH_Context indexPageContext = {0};
struct AmmServer_RH_Context loadingGIFContext = {0};
struct AmmServer_RH_Context loadingPNGContext = {0};
struct AmmServer_RH_Context logoPNGContext = {0};
struct AmmServer_RH_Context imageContext = {0};
struct AmmServer_RH_Context form = {0};
struct AmmServer_RH_Context sendMailContext = {0};
struct AmmServer_RH_Context uploadProcessor = {0};
//------------------------------------------------
struct AmmServer_MemoryHandler * indexPage=0;
struct AmmServer_MemoryHandler * loadingGif=0;
struct AmmServer_MemoryHandler * loadingImage=0;
struct AmmServer_MemoryHandler * logoImage=0;
struct AmmServer_MemoryHandler * imageFile[MAX_IMAGES_CONCURRENTLY]={0};
//------------------------------------------------

const unsigned long maximumDiskUsageAllowed=3.5 /*GB*/ *1024 *1024 *1024;
const unsigned int bufferPageSize=32 /*KB*/ *1024;
unsigned int maxUploadFileSizeAllowedMB=4; /*MB*/
char uploads_root[MAX_FILE_PATH]="uploads/";
unsigned int uploadsFilesSize=0;
unsigned int uploadsDataSize=0;

//------------------------------------------------

void * prepare_index_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
    AmmServer_DynamicRequestReturnMemoryHandler(rqst,indexPage);
    return 0;
}


void filterQuery(char * query)
{
    int len = strlen(query);

    for (int i=0; i<len; i++)
    {
       if ( (query[i]=='.')  || (query[i]==',') ) { } else
       if ( (query[i]=='(')  || (query[i]==')') ) { } else
       if ( (query[i]>='a')  && (query[i]<='z') ) { } else
       if ( (query[i]>='A')  && (query[i]<='Z') ) { } else
       if ( (query[i]>='0')  && (query[i]<='9') ) { } else
          {
            query[i]=' ';
          }
    }
}


unsigned long getUploadsSizeLive()
{
  if (uploadsFilesSize==0)
  {
    char command[4096]={0};
    char sizeOfUploadsString[257]={0};

    snprintf(command,4096,"du -sb %s%s | cut -f1",webserver_root,uploads_root);
    if ( AmmServer_ExecuteCommandLine(command,sizeOfUploadsString,256) )
    {
      uploadsFilesSize = atoi(sizeOfUploadsString);
      fprintf(stderr,"getUploadsSizeLive raw result = %s ( %u ) \n",sizeOfUploadsString,uploadsFilesSize);
    }
  }
 return uploadsFilesSize;
}


char * getBackRandomFileDigits(unsigned int numberOfDigits)
{
 char * response= (char *) malloc(sizeof(char)* (numberOfDigits+1));

 unsigned int i=0,range='z'-'a';
 for (i=0; i<numberOfDigits; i++)
 {
   response[i]='a'+rand()%range;
 }
response[numberOfDigits]=0;
return response;
}

void * processUploadCallback(struct AmmServer_DynamicRequest  * rqst)
{
   AmmServer_Warning("processUploadCallback called\n");
  //md5sum file.jpg | cut -d' ' -f1

  if (getUploadsSizeLive() >= maximumDiskUsageAllowed)
  {
    AmmServer_Error("SERVER IS FULL\n");
    return  prepare_index_content_callback(rqst);
  }

  char * storeID = getBackRandomFileDigits(32);

  if (storeID!=0)
  {
   unsigned int fSize=0;
   char uploadedFileUNSANITIZEDPath[4096]={0};
   snprintf(uploadedFileUNSANITIZEDPath,4095,"%s",_FILES(rqst,"uploadedfile",FILENAME,&fSize));

   AmmServer_Warning("Unsanitized filename is %s \n",uploadedFileUNSANITIZEDPath);

   if (AmmServer_StringHasSafePath(uploads_root,uploadedFileUNSANITIZEDPath))
   {
    char * uploadedFilePath = uploadedFileUNSANITIZEDPath;
    char finalPath[4096]={0};
    snprintf(finalPath,4095,"%s/%s/%s-%s",webserver_root,uploads_root,storeID,uploadedFilePath);

   const char * f = _FILES(rqst,"uploadedfile",VALUE,&fSize);
   AmmServer_WriteFileFromMemory(finalPath,f,fSize);


   char fullCommand[MAX_QUERY_SIZE+1024]={0};

   snprintf(fullCommand,MAX_QUERY_SIZE+1024,"convert \"%s\" -resize 512x512^ -gravity Center -extent 512x512 /home/user/workspace/upimage.jpg",finalPath);
   int i=system(fullCommand);
   fprintf(stderr,"Executed : %s \n",fullCommand);
   fprintf(stderr,"Response : %u \n",i);


   unsigned int query3Size=0;
   const char * query3 = _FILES(rqst,"query3",VALUE,&query3Size);
   //if (i==0)
     {
      //if  (_GETexists(rqst,"query3"))
       {
        char query[MAX_QUERY_SIZE]= {0};
        snprintf(query,MAX_QUERY_SIZE,"%s",query3);
        //if ( _GETcpy(rqst,"query3",query,MAX_QUERY_SIZE) )
        {
             filterQuery(query);
             snprintf(fullCommand,MAX_QUERY_SIZE+1024,"/home/user/workspace/img2imgOnlyOnce.sh \"%s\"",query);
             i=system(fullCommand);
             fprintf(stderr,"Executed : %s \n",fullCommand);
             fprintf(stderr,"Response : %u \n",i);

            fprintf(stderr,"\n\n\n\n\n\nQUERY ASCII: %s \n\n\n\n\n\n\n",query);
             if (i!=0)
                     {

                     }
        }
       }
      }

    //This is slightly bigger ( plus the header but almost correct )
    uploadsFilesSize+=fSize;

    //snprintf(finalPath,512,"%s/%s/%s.raw",webserver_root,uploads_root,storeID);
    //AmmServer_WriteFileFromMemory(finalPath,rqst->POST_request,rqst->POST_request_length);

    //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
    snprintf(finalPath,4095,"%s-%s",storeID,uploadedFilePath);
    return prepare_index_content_callback(rqst);
   }
   free(storeID);
  } else
  {
    return prepare_index_content_callback(rqst);
  }
  return 0;
}


int sendEmail(
               const char * username,
               const char * password,
               //const char * receipient,
               const char * subject,
               const char * message
             )
{
   char messageBuffer[4096]={0};
   char result[1024]={0};
   snprintf(messageBuffer,4096,"printf \"Subject:%s\n\n%s\n\" | \
            (cat - && uuencode %s/%05u.png image1.png) | \
            (cat - && uuencode %s/%05u.png image2.png) | \
            (cat - && uuencode %s/%05u.png image3.png) | \
            (cat - && uuencode %s/%05u.png image4.png) | \
            ssmtp -v %s@%s",
            subject,message,
            IMAGE_DIRECTORY,0,
            IMAGE_DIRECTORY,1,
            IMAGE_DIRECTORY,2,
            IMAGE_DIRECTORY,3,
            username,
            password
            );
   if (1)//filterStringForShellInjection(messageBuffer,512))
   {
    if ( AmmServer_ExecuteCommandLine(messageBuffer,result,512) )
    {
     fprintf(stderr,"Successfully sent message to %s@%s..\n",username,password);
     return 1;
    }
   }
//sudo apt install sharutils ssmtp
//printf "Subject:FORTH-ICS-CVRL Your AI Generated Images\n\nΣας ευχαριστούμε που μας επισκευθήκατε. \nΣας επισυνάπτουμε τις εικόνες που δημιουργήσατε με την βοήθεια της Τεχνητής Νοημοσύνης. \nFORTH-ICS-CVRL - 2022\n \n \n \n" | (cat - && uuencode 00982.png image1.png) | (cat - && uuencode 00983.png image2.png) | (cat - && uuencode 00984.png image3.png) | (cat - && uuencode 00985.png image4.png) | ssmtp -v ammarkov@gmail.com

 AmmServer_Error("Failed to send message to %s@%s..\n",username,password);
 return 0;
}

void * logoPNGContent(struct AmmServer_DynamicRequest  * rqst)
{
    AmmServer_DynamicRequestReturnMemoryHandler(rqst,logoImage);
    return 0;
}

void * loadingPNGContent(struct AmmServer_DynamicRequest  * rqst)
{
    AmmServer_DynamicRequestReturnMemoryHandler(rqst,loadingImage);
    return 0;
}

void * loadingGIFContent(struct AmmServer_DynamicRequest  * rqst)
{
    AmmServer_DynamicRequestReturnMemoryHandler(rqst,loadingGif);
    return 0;
}

void * prepare_image_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
    if (_GETexists(rqst,"i"))
    {
        int imageChannel = _GETuint(rqst,"i");
        if ( (imageChannel>=0) && (imageChannel<MAX_IMAGES_CONCURRENTLY) )
        {
         AmmServer_FreeMemoryHandler(&imageFile[imageChannel]);
         char filename[512]={0};
         snprintf(filename,512,"%s/%05u.png",IMAGE_DIRECTORY,imageChannel);
         imageFile[imageChannel] = AmmServer_ReadFileToMemoryHandler(filename);
         if (imageFile[imageChannel]!=0)
         {
           AmmServer_DynamicRequestReturnMemoryHandler(rqst,imageFile[imageChannel]);
         } else
         {
           AmmServer_DynamicRequestReturnMemoryHandler(rqst,loadingImage);
         }
         return 0;
        }
    }

    AmmServer_DynamicRequestReturnMemoryHandler(rqst,loadingImage);
    return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * sendMail(struct AmmServer_DynamicRequest  * rqst)
{
  if  ((_GETexists(rqst,"user")) && (_GETexists(rqst,"server") ) )
    {
        char user[MAX_QUERY_SIZE]   = {0};
        char server[MAX_QUERY_SIZE] = {0};
        if (
             ( _GETcpy(rqst,"user",user,MAX_QUERY_SIZE) ) &&
             ( _GETcpy(rqst,"server",server,MAX_QUERY_SIZE) )
           )
        {
          filterQuery(user);
          filterQuery(server);
          sendEmail(
                    user,
                    server,
                    "FORTH-ICS-CVRL Your AI Generated Images",
                    "Σας ευχαριστούμε που μας επισκευθήκατε.\nΣας επισυνάπτουμε τις εικόνες που δημιουργήσατε με την βοήθεια της Τεχνητής Νοημοσύνης.\nFORTH-ICS-CVRL - 2022\n \n \n "
                   );
        }
    }

  snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\"></head><body>Refresh</body></html>");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//This function prepares the content of  stats context , ( stats.content )
void * generateImagesBasedOnQuery(struct AmmServer_DynamicRequest  * rqst)
{
  if  (_GETexists(rqst,"query"))
    {
        char query[MAX_QUERY_SIZE]= {0};
        if ( _GETcpy(rqst,"query",query,MAX_QUERY_SIZE) )
        {
            fprintf(stderr,"\n\n\n\n\n\nQUERY HTTP: %s \n\n\n\n\n\n\n",query);
            filterQuery(query);
            fprintf(stderr,"\n\n\n\n\n\nQUERY ASCII: %s \n\n\n\n\n\n\n",query);

            char fullCommand[MAX_QUERY_SIZE+1024]={0};
            snprintf(fullCommand,MAX_QUERY_SIZE+1024,"/home/user/workspace/tex2imgOnlyOnce.sh \"%s\"",query);
            int i=system(fullCommand);
            fprintf(stderr,"Executed : %s \n",fullCommand);
            fprintf(stderr,"Response : %u \n",i);

            if (i!=0)
            {
                //SERVER IS BUSY
                snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><meta http-equiv=\"refresh\" content=\"15;URL='index.html'\"></head><body>Server is Busy, Please Wait, then try again</body></html>");
                rqst->contentSize=strlen(rqst->content);
                return 0;
            }

        }
    }
    //SERVER EXECUTED REQUEST SUCCESSFULLY
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\"></head><body>Refresh</body></html>");
    rqst->contentSize=strlen(rqst->content);
    return 0;
}





//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
    mkdir(uploads_root, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    AmmServer_AddResourceHandler(default_server,&uploadProcessor,"/upload.html",bufferPageSize,0,&processUploadCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
    AmmServer_DoNOTCacheResourceHandler(default_server,&uploadProcessor);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    AmmServer_AddResourceHandler(default_server,&indexPageContext ,"/index.html",16000,0,&prepare_index_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
    AmmServer_AddResourceHandler(default_server,&loadingGIFContext,"/loading.gif",644096,0,&loadingGIFContent,SAME_PAGE_FOR_ALL_CLIENTS);
    AmmServer_AddResourceHandler(default_server,&loadingPNGContext,"/loading.png",644096,0,&loadingPNGContent,SAME_PAGE_FOR_ALL_CLIENTS);
    AmmServer_AddResourceHandler(default_server,&logoPNGContext   ,"/logod.png",644096,0,&logoPNGContent,SAME_PAGE_FOR_ALL_CLIENTS);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    AmmServer_AddResourceHandler(default_server,&form             ,"/go",4096,0,&generateImagesBasedOnQuery,DIFFERENT_PAGE_FOR_EACH_CLIENT);
    AmmServer_AddResourceHandler(default_server,&sendMailContext  ,"/mail",4096,0,&sendMail,DIFFERENT_PAGE_FOR_EACH_CLIENT);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    AmmServer_AddResourceHandler(default_server,&imageContext     ,"/image.png",1024000,0,&prepare_image_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    indexPage    = AmmServer_ReadFileToMemoryHandler("src/Services/ImageGeneration/generation.html");
    loadingImage = AmmServer_ReadFileToMemoryHandler("src/Services/ImageGeneration/loading.png");
    loadingGif   = AmmServer_ReadFileToMemoryHandler("src/Services/ImageGeneration/loading.gif");
    logoImage    = AmmServer_ReadFileToMemoryHandler("src/Services/ImageGeneration/logod.png");
    //--------------------------------------------------------------------------------------------------------------------------------------------
    if ( (indexPage==0) || (loadingImage==0) || (loadingGif==0) || (logoImage==0)  )
    {
        AmmServer_Error("Could not find required files on disk");
        exit(0);
    }
    //--------------------------------------------------------------------------------------------------------------------------------------------

    char filename[512]={0};
    for (int i=0; i<MAX_IMAGES_CONCURRENTLY; i++)
    {
        snprintf(filename,512,"src/Services/ImageGeneration/%u.png",i);
        imageFile[i] = AmmServer_ReadFileToMemoryHandler(filename);
    }

    //Always be served fresh
    AmmServer_DoNOTCacheResource(default_server,"image.png");
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&form,1);
    AmmServer_RemoveResourceHandler(default_server,&sendMailContext,1);
    AmmServer_RemoveResourceHandler(default_server,&imageContext,1);
    AmmServer_RemoveResourceHandler(default_server,&logoPNGContext,1);
    AmmServer_RemoveResourceHandler(default_server,&loadingPNGContext,1);
    AmmServer_RemoveResourceHandler(default_server,&loadingGIFContext,1);
    AmmServer_RemoveResourceHandler(default_server,&indexPageContext,1);
    //--------------------------------------------------------------------------------------------------------------------------------------------
    AmmServer_FreeMemoryHandler(&indexPage);
    AmmServer_FreeMemoryHandler(&loadingImage);
    AmmServer_FreeMemoryHandler(&loadingGif);
    AmmServer_FreeMemoryHandler(&logoImage);
    for (int i=0; i<MAX_IMAGES_CONCURRENTLY; i++)
    {
     AmmServer_FreeMemoryHandler(&imageFile[i]);
    }
    //--------------------------------------------------------------------------------------------------------------------------------------------
}
/*! Dynamic content code ..! END ------------------------*/





int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //If we have a command line arguments we overwrite our buffers

    unsigned int i=0;
    for (i=0; i<argc; i++)
    {
        if ((strcmp(argv[i],"--root")==0)&&(argc>i+1))
        {
            snprintf(webserver_root,MAX_FILE_PATH,"%s",argv[i+1]);
        }
    }

    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;

    default_server = AmmServer_StartWithArgs(
                         "imagegeneration",
                         argc,argv,  //The internal server will use the arguments to change settings
                         //If you don't want this look at the AmmServer_Start call
                         bindIP,
                         port,
                         0, /*This means we don't want a specific configuration file*/
                         webserver_root,
                         templates_root
                     );

    if (!default_server)
    {
        AmmServer_Error("Could not start server , shutting down everything..");
        exit(1);
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
