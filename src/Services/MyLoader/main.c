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
#include "../../AmmServerlib/AmmServerlib.h"

#define DEFAULT_BINDING_PORT 8085

unsigned int maxUploadFileSizeAllowedMB=4; /*MB*/

char webserver_root[MAX_FILE_PATH]="src/Services/MyLoader/res/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char uploads_root[MAX_FILE_PATH]="uploads/";
char templates_root[MAX_FILE_PATH]="public_html/templates/";

unsigned int uploadsFilesSize=0;
unsigned int uploadsDataSize=0;
//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};
struct AmmServer_RH_Context uploadProcessor={0};
struct AmmServer_RH_Context indexProcessor={0};
struct AmmServer_RH_Context vFileProcessor={0};
struct AmmServer_RH_Context vFileProcessorCompat={0};
struct AmmServer_RH_Context fileProcessor={0};
struct AmmServer_RH_Context fileProcessorCompat={0};
struct AmmServer_RH_Context randomProcessor={0};

struct AmmServer_MemoryHandler * indexPage=0;
struct AmmServer_MemoryHandler * vFilePage=0;
struct AmmServer_MemoryHandler * errorPage=0;


unsigned int getUploadsSizeLive()
{
  if (uploadsFilesSize==0)
  {
    char command[2049]={0};
    char sizeOfUploadsString[257]={0};

    snprintf(command,2048,"du -sb %s%s | cut -f1",webserver_root,uploads_root);
    if ( AmmServer_ExecuteCommandLine(command,sizeOfUploadsString,256) )
    {
      uploadsFilesSize = atoi(sizeOfUploadsString);
      fprintf(stderr,"getUploadsSizeLive raw result = %s ( %u ) \n",sizeOfUploadsString,uploadsFilesSize);
    }
  }
 return uploadsFilesSize;
}

//This function prepares the content of  stats context , ( stats.content )
void * prepare_error_callback(struct AmmServer_DynamicRequest  * rqst)
{
  snprintf(rqst->content,rqst->MAXcontentSize,"%s",errorPage->content);
  rqst->contentSize=errorPage->contentCurrentLength;
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_index_callback(struct AmmServer_DynamicRequest  * rqst)
{
  struct AmmServer_MemoryHandler * videoMH = AmmServer_CopyMemoryHandler(indexPage);

  unsigned int linkID=1+rand()%6;

  char stringBuffer[29]={0};
  snprintf(stringBuffer,28,"banner_%u",linkID);

  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,2,"$NAME_OF_THIS_MYLOADER_SERVER$","AmmarServer");
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$BANNERLINK$",stringBuffer);


  snprintf(stringBuffer,28,"%u",maxUploadFileSizeAllowedMB);
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$MAXIMUM_UPLOAD_SIZE$", stringBuffer);

  snprintf(stringBuffer,28,"%0.2f MB",(float) getUploadsSizeLive()/(1024*1024));
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$TOTAL_SHARED_DATA$", stringBuffer);


  snprintf(stringBuffer,28,"%0.2f MB",(float) default_server->statistics.totalUploadBytes/(1024*1024));
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$TOTAL_UPLOAD_SIZE$", stringBuffer);

  memcpy (rqst->content , videoMH->content , videoMH->contentCurrentLength );
  rqst->contentSize=videoMH->contentCurrentLength ;

  AmmServer_FreeMemoryHandler(&videoMH);
  return 0;
}


void * render_vfile(struct AmmServer_DynamicRequest  * rqst, const char * fileRequested)
{
  struct AmmServer_MemoryHandler * videoMH = AmmServer_CopyMemoryHandler(vFilePage);

  char filenameToAccess[1025];
  snprintf(filenameToAccess,1024,"%s%s",uploads_root,fileRequested);

  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$NAME_OF_THIS_MYLOADER_SERVER$","AmmarServer");
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$NAME_OF_THIS_MYLOADER_FILE$",fileRequested);


  char randomString[129];
  snprintf(randomString,128,"%u",rand()%100000);
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,3,"$RANDOMNUMBERS$",randomString);


  //todo: have different embed point if it is video etc..

  char embed[2049];
  if (AmmServer_FileIsText(fileRequested))
  {
     snprintf(embed,2048," <iframe src=\"%s\" width=\"70%%\"></iframe><br><a  href=\"%s\">Download the file</a>",filenameToAccess, filenameToAccess);
  }
   else
  if (AmmServer_FileIsAudio(fileRequested)) // audio
  {
    snprintf(embed,2048,"<audio autoplay controls><source src=\"%s\" type=\"audio/ogg\" />\
                         <p> Try this page on HTML5 capable browsers</p>);\
                          </audio><br>\
                          <a  href=\"%s\">Download the audio file</a>" ,filenameToAccess, filenameToAccess );
  } else
  if (AmmServer_FileIsImage(fileRequested)) // image
  {
    snprintf(embed,2048," <a href=\"%s\"><img src=\"%s\" width=\"70%%\" alt=\"Uploaded Image\" ></a><br><br>\
                           <a href=\"%s\">%s</a>" ,filenameToAccess, filenameToAccess , filenameToAccess , fileRequested );
  } else
  if (AmmServer_FileIsVideo(fileRequested)) // video
  {
     snprintf(embed,2048,"<video poster=\"images/video_logo.png\" autoplay controls>\
                          <source src=\"%s\" type='video/webm; codecs=\"vp8, vorbis\"'  /></video>\
                           <br><a  href=\"%s\">Download the video file</a>",filenameToAccess, filenameToAccess );
  } else
  if (AmmServer_FileIsFlash(fileRequested))
  {
      snprintf(embed,2048,"<object classid=\"clsid:D27CDB6E-AE6D-11cf-96B8-444553540000\"\
                           codebase=\"http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,29,0\" width=\"640\" height=\"480\">\
                           <param name=\"movie\" value=\"%s\">\
                           <param name=\"quality\" value=\"high\">\
                           <embed src=\"%s\" quality=\"high\" pluginspage=\"http://www.macromedia.com/go/getflashplayer\" type=\"application/x-shockwave-flash\" width=\"640\" height=\"480\"></embed>\
                   </object>\
                     <br><a  href=\"%s\">Download the flash file</a><br><br>",filenameToAccess,filenameToAccess,filenameToAccess);
  } else
  {
    snprintf(embed,2048,"<a href=\"%s\"><img src=\"images/host_logo.png\"><br><br>Binary file : %s</a>",filenameToAccess,fileRequested);
  }
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$PLACE_TO_EMBED_CONTENT$",embed);

  snprintf(embed,2048,"vfile.html?i=%s",fileRequested);
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,2,"$PLACE_TO_EMBED_PERMALINK$",embed);


  memcpy (rqst->content , videoMH->content , videoMH->contentCurrentLength );
  rqst->contentSize=videoMH->contentCurrentLength ;

  AmmServer_FreeMemoryHandler(&videoMH);
 return 0;
}








void * render_upload(struct AmmServer_DynamicRequest  * rqst, const char * fileRequested)
{
  struct AmmServer_MemoryHandler * videoMH = AmmServer_CopyMemoryHandler(vFilePage);

  char filenameToAccess[1025]={0};
  snprintf(filenameToAccess,1024,"%s%s",uploads_root,fileRequested);

  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$NAME_OF_THIS_MYLOADER_SERVER$","AmmarServer");
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$NAME_OF_THIS_MYLOADER_FILE$",fileRequested);


  char randomString[129]={0};
  snprintf(randomString,128,"%u",rand()%100000);
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,3,"$RANDOMNUMBERS$",randomString);

  char embed[2049]={0};
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,1,"$PLACE_TO_EMBED_CONTENT$",embed);

  snprintf(embed,2048,"vfile.html?i=%s",fileRequested);
  AmmServer_ReplaceAllVarsInMemoryHandler(videoMH,2,"$PLACE_TO_EMBED_PERMALINK$",embed);


  memcpy (rqst->content , videoMH->content , videoMH->contentCurrentLength );
  rqst->contentSize=videoMH->contentCurrentLength ;

  AmmServer_FreeMemoryHandler(&videoMH);
 return 0;
}




//This function prepares the content of  stats context , ( stats.content )
void * prepare_vfile_callback(struct AmmServer_DynamicRequest  * rqst)
{
  int fileIsOk=0;
  char fileRequested[129]={0};
  if ( _GET(default_server,rqst,"i",fileRequested,128) )
              {
                fprintf(stderr,"Requested file %s \n",fileRequested);
                fileIsOk=1;
              }

  if (!AmmServer_StringHasSafePath(uploads_root,fileRequested))
  {
    AmmServer_Error("AmmServer_StringHasSafePath(\"%s\",\"%s\")=bad request ",uploads_root,fileRequested);
    fileIsOk=0;
  }

  if (!fileIsOk)
  {
    AmmServer_Error("File %s deemed as a bad request ",fileRequested);
    return prepare_error_callback(rqst);
  }

  return  render_vfile(rqst,fileRequested);
}


//This function could alter the content of the URI requested and then return 1
void request_override_callback(void * request)
{
  struct AmmServer_RequestOverride_Context * rqstContext = (struct AmmServer_RequestOverride_Context *) request;
  struct HTTPHeader * rqst = rqstContext->request;
  AmmServer_Warning("With URI : %s \n Filtered URI : %s \n GET Request : %s \n",rqst->resource,rqst->verified_local_resource, rqst->GETquery);

  if (strcmp("/favicon.ico",rqst->resource)==0 )  { return; /*Client requested favicon.ico , no resolving to do */ } else
  if (strcmp("/error.html",rqst->resource)==0 )   { return; /*Client requested error.html , no resolving to do */  } else
  if (strcmp("/upload.html",rqst->resource)==0 )  { return; /*Client requested error.html , no resolving to do */  } else
  if (strcmp("/",rqst->resource)==0 )             {  return; /*Client requested index.html , no resolving to do */  } else
  if (strcmp("/random.html",rqst->resource)==0 )  { return; /*Client requested index.html , no resolving to do */  }

  return;
}


char * getBackRandomFileDigits(unsigned int numberOfDigits)
{
 char * response= (char *) malloc(sizeof(char)* (numberOfDigits+1));

 unsigned int i=0,range=0;
 for (i=0; i<numberOfDigits; i++)
 {
   range='z'-'a';
   response[i]='a'+rand()%range;
 }
response[numberOfDigits]=0;
return response;
}


void * processUploadCallback(struct AmmServer_DynamicRequest  * rqst)
{
  //md5sum file.jpg | cut -d' ' -f1
  char * storeID = getBackRandomFileDigits(32);

  if (storeID!=0)
  {
   char uploadedFileUNSANITIZEDPath[513]={0};
   AmmServer_POSTNameOfFile (default_server,rqst,0,uploadedFileUNSANITIZEDPath,512);
   AmmServer_Warning("Unsanitized filename is %s \n",uploadedFileUNSANITIZEDPath);

   if (AmmServer_StringHasSafePath(uploads_root,uploadedFileUNSANITIZEDPath))
   {
    char * uploadedFilePath = uploadedFileUNSANITIZEDPath;
    char finalPath[2049]={0};
    snprintf(finalPath,2048,"%s/%s/%s-%s",webserver_root,uploads_root,storeID,uploadedFilePath);
    AmmServer_POSTArgToFile (default_server,rqst,0,finalPath);

    //This is slightly bigger ( plus the header but almost correct )
    uploadsFilesSize+=rqst->POST_request_length;

    //snprintf(finalPath,512,"%s/%s/%s.raw",webserver_root,uploads_root,storeID);
    //AmmServer_WriteFileFromMemory(finalPath,rqst->POST_request,rqst->POST_request_length);

    //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
    snprintf(finalPath,2048,"%s-%s",storeID,uploadedFilePath);
    return render_upload(rqst,finalPath);
   }
   free(storeID);
  } else
  {
    return prepare_error_callback(rqst);
  }
  return 0;
}


void * prepare_random_callback(struct AmmServer_DynamicRequest  * rqst)
{
  char command[2049]={0};
  char fileToServe[2049]={0};

  snprintf(command,2048,"find %s%s -type f | shuf -n 1",webserver_root,uploads_root);
  if ( AmmServer_ExecuteCommandLine(command,fileToServe,2048) )
  {
     AmmServer_ReplaceCharInString(fileToServe,10,0); //Strip new line
     AmmServer_ReplaceCharInString(fileToServe,13,0); //Strip new line
     AmmServer_Warning("Random File Selected is : `%s`\n",fileToServe);

     const char * baseName=AmmServer_GetBasenameFromPath(fileToServe);
     AmmServer_Warning("Random File Basename is : `%s`\n",baseName);

     snprintf(command,2048,"%s%s%s",webserver_root,uploads_root,baseName);
     if (AmmServer_FileExists(command))
     {
       return render_vfile(rqst,baseName);
     }
  }
  return prepare_error_callback(rqst);
}



//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_SetIntSettingValue(default_server,AMMSET_MAX_POST_TRANSACTION_SIZE,(maxUploadFileSizeAllowedMB+1)*1024*1024); //+1MB for headers etc..
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  AmmServer_AddResourceHandler(default_server,&uploadProcessor,"/upload.html",webserver_root,4096,0,&processUploadCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_DoNOTCacheResourceHandler(default_server,&uploadProcessor);

  AmmServer_AddResourceHandler(default_server,&indexProcessor,"/index.html",webserver_root,4096,0,&prepare_index_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&vFileProcessor,"/vfile.html",webserver_root,4096,0,&prepare_vfile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&vFileProcessorCompat,"/vfile.php",webserver_root,4096,0,&prepare_vfile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&fileProcessor,"/file.html",webserver_root,4096,0,&prepare_vfile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&fileProcessorCompat,"/file.php",webserver_root,4096,0,&prepare_vfile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);

  AmmServer_AddResourceHandler(default_server,&randomProcessor,"/random.html",webserver_root,4096,0,&prepare_random_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_DoNOTCacheResourceHandler(default_server,&randomProcessor);



  fprintf(stderr,"Reading master index file..  ");
  indexPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyLoader/res/index.html");
  vFilePage=AmmServer_ReadFileToMemoryHandler("src/Services/MyLoader/res/vfile.html");
  errorPage=AmmServer_ReadFileToMemoryHandler("src/Services/MyLoader/res/error.html");

  fprintf(stderr,"current length %u , size is %u \n",indexPage->contentCurrentLength , indexPage->contentSize);

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&uploadProcessor,1);
    AmmServer_RemoveResourceHandler(default_server,&indexProcessor,1);
    AmmServer_RemoveResourceHandler(default_server,&vFileProcessor,1);
    AmmServer_FreeMemoryHandler(&indexPage);
    AmmServer_FreeMemoryHandler(&vFilePage);
    AmmServer_FreeMemoryHandler(&errorPage);
}




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "myloader",
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
