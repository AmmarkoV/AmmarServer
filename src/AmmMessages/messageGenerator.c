#include "messageGenerator.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>

#include "../StringRecognizer/fastStringParser.h"
#include "../InputParser/InputParser_C.h"

#define MAXIMUM_FILENAME_WITH_EXTENSION 1024
#define MAXIMUM_LINE_LENGTH 1024


char * varTypesList[] = {   "int64"   , "unsigned long %s;\n" , "%s=atoi(%s);"               , "%lu"  ,
                            "int32"   , "unsigned int %s;\n"  , "%s=atoi(%s);"               , "%u"   ,
                            "int"     , "unsigned long %s;\n" , "%s=atoi(%s);"               , "%u"   ,
                            "float"   , "float %s;\n"         , "%s=atof(%s);"               , "%0.5f",
                            "float32" , "float %s;\n"         , "%s=atof(%s);"               , "%0.5f",
                            "float64" , "double %s;\n"        , "%s=atof(%s);"               , "%f"   ,
                            "bool"    , "int %s;\n"           , "%s=atoi(%s);"               , "%u"   ,
                            "string"  , "char %s[512];\n"     , "snprintf(%s,512,\"%%s\",%s);", "%s"  ,
                            //DONT FORGET TO UPDATE  varTypesListNumber
                            "fail"  , "fail"     , "fail",  "fail"
                        };
unsigned int varTypesListNumber = 8;



int typeExists(const char * rosType)
{
  int retval=0;
  unsigned int i=0;
  for (i=0; i<varTypesListNumber; i++)
  {
     if (strcasecmp(rosType,varTypesList[i*4+0])==0) { retval=1; }
  }

  return retval;
}


int writeType(
                 FILE * fp ,
                 const char * rosType,
                 const char * variableID
                )
{
  int retval=0;

  unsigned int i=0;
  for (i=0; i<varTypesListNumber; i++)
  {
     if (strcasecmp(rosType,varTypesList[i*4+0])==0) { fprintf(fp,varTypesList[i*4+1],variableID); retval=1; }
  }

  return retval;
}


int writeConversion(
                 FILE * fp ,
                 const char * rosType,
                 const char * varDestination,
                 const char * varName
                )
{
  int retval=0;
  unsigned int i=0;
  for (i=0; i<varTypesListNumber; i++)
  {
     if (strcasecmp(rosType,varTypesList[i*4+0])==0) { fprintf(fp,varTypesList[i*4+2],varDestination,varName); retval=1; }
  }
 return retval;
}



void writeGETPrintf(
                 FILE * fp ,
                 struct fastStringParser * fsp,
                 const char * functionName
                )
{
 char variableType[256]={0};
 char variableID[256]  ={0};
 struct InputParserC * ipc = InputParser_Create(512,5);
 InputParser_SetDelimeter(ipc,1,' ');

  //First output Initial GET URI
  fprintf(fp,"\"/%s.html?",functionName);

  //The output the formatting string
  unsigned int numberOfArguments=0;
  unsigned int i=0,z=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    int arguments = InputParser_SeperateWordsCC(ipc,fsp->contents[i].str,1);
    if (arguments==2)
    {
     InputParser_GetWord(ipc,0,variableType,256);
     InputParser_GetWord(ipc,1,variableID,256);
     fprintf(fp,"%s=",variableID);
     for (z=0; z<varTypesListNumber; z++)
     {
       if (strcasecmp(variableType,varTypesList[z*4+0])==0)
         {
              fprintf(fp,"%s&",varTypesList[z*4+3]);
              ++numberOfArguments;
         }
     }
    }
  }
  fprintf(fp,"t=%%u");
  fprintf(fp,"\",");

  //Then output the arguments..
  unsigned int secondCountOfArguments=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    int arguments = InputParser_SeperateWordsCC(ipc,fsp->contents[i].str,1);
    if (arguments==2)
    {
     InputParser_GetWord(ipc,0,variableType,256);
     InputParser_GetWord(ipc,1,variableID,256);
     for (z=0; z<varTypesListNumber; z++)
     {
       if (strcasecmp(variableType,varTypesList[z*4+0])==0)
         {
          fprintf(fp,"msg->%s,",variableID);
          ++secondCountOfArguments;
         }
     }
    }
  }
  fprintf(fp,"rand()%%10000");

  InputParser_Destroy(ipc);
}




int writeServerCallback(
                           FILE * fp ,
                           const char *  functionName,
                           struct fastStringParser * fsp
                       )
{
  fprintf(fp,"static void * %sHTTPServer(struct AmmServer_DynamicRequest  * rqst)\n",functionName);
  fprintf(fp,"{\n");

  fprintf(fp,"char value[256]={0};\n");

 char variableType[256]={0};
 char variableID[256]  ={0};
 struct InputParserC * ipc = InputParser_Create(512,5);
 InputParser_SetDelimeter(ipc,1,' ');

  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    int arguments = InputParser_SeperateWordsCC(ipc,fsp->contents[i].str,1);
    if (arguments==2)
    {
        InputParser_GetWord(ipc,0,variableType,256);
        InputParser_GetWord(ipc,1,variableID,256);

        if (typeExists(variableType))
        {
          char destination[256]={0};
          snprintf(destination,256,"%sStatic.%s",functionName,variableID);

          fprintf(fp," if ( _GETcpy(rqst,(char*)\"%s\",value,255) )   {  ",variableID);
          writeConversion(
                          fp ,
                          variableType,
                          destination,
                          "value"
                         );
          fprintf(fp,"  }  \n");
        }
    }
  }


  fprintf(fp," #if DEBUG_PRINT\n");
     fprintf(fp,"fprintf(stderr,\"HTTPServer: Received a %s message \\n \");\n",functionName);
     fprintf(fp,"print_%s(&%sStatic);\n",functionName,functionName);
     fprintf(fp,"fprintf(stderr,\" \\n \");\n");
  fprintf(fp," #endif\n");

  fprintf(fp,"++%sStatic.timestampInit;\n",functionName);


  fprintf(fp,"if (!write_%s(&%sBridge,&%sStatic)) { AmmServer_Error(\"Could not send %s to mmaped bridge \");  }",functionName,functionName,functionName,functionName);


  fprintf(fp,"snprintf(rqst->content,rqst->MAXcontentSize,\"<html><body>OK</body></html>\");\n");
  fprintf(fp,"rqst->contentSize=strlen(rqst->content);\n\n");
  fprintf(fp,"return 0;\n");

  InputParser_Destroy(ipc);



  fprintf(fp,"  \n");
  fprintf(fp,"}\n\n");
 return 1;
}






int writeStruct(
                 FILE * fp ,
                 const char * line
                )
{
 struct InputParserC * ipc = InputParser_Create(512,5);
 InputParser_SetDelimeter(ipc,1,' ');

 int arguments = InputParser_SeperateWordsCC(ipc,line,1);
 char variableType[256]={0};
 char variableID[256]  ={0};

 //fprintf(stderr,"%u arguments \n",arguments);
 if (arguments==2)
 {
     InputParser_GetWord(ipc,0,variableType,256);
     InputParser_GetWord(ipc,1,variableID,256);
     if (!writeType(fp,variableType,variableID) )
     {
       fprintf(fp,"//%s\n",line); //Line as a comment
     }
 } else
 {
     fprintf(fp,"//%s\n",line);
 }

 InputParser_Destroy(ipc);
return 1;
}


int compileMessage(const char * filename,const char * label,const char * pathToMMap)
{
  struct fastStringParser * fsp = fastSTringParser_createRulesFromFile(filename,64);

  const char *  functionName = label;
  unsigned int functionNameLength = strlen(functionName);
  fsp->functionName  = (char* ) malloc(sizeof(char) * (1+functionNameLength));
  if (fsp->functionName==0) { fprintf(stderr,"Could not allocate memory for function name\n"); return 0; }
  strncpy(fsp->functionName,functionName,functionNameLength);
  fsp->functionName[functionNameLength]=0;

  convertTo_ENUM_ID(fsp->functionName);


  char filenameWithExtension[MAXIMUM_FILENAME_WITH_EXTENSION+1]={0};



  //PRINT OUT THE HEADER
  snprintf(filenameWithExtension,MAXIMUM_FILENAME_WITH_EXTENSION,"%s.h",functionName);


  fprintf(stderr,"File Output %s\n",filenameWithExtension);

  FILE * fp = fopen(filenameWithExtension,"wb");
  if (fp == 0) { fprintf(stderr,"Could not open output file %s\n",functionName); return 0; }
  fflush(fp);
  fprintf(fp,"\n");


  fprintf(fp,"/** @file %s.h\n",functionName);
  fprintf(fp,"* @brief A tool that generates code that can span messages across computers\n");
  fprintf(fp,"* @author Ammar Qammaz (AmmarkoV)\n");
  fprintf(fp,"*/\n\n");

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);


  fprintf(fp,"/* \
                 \nThis file was automatically generated @ %02d-%02d-%02d %02d:%02d:%02d using AmmMessages \
                 \nhttps://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmMessages\
                 \nPlease note that changes you make here may be automatically overwritten \
                 \nif the AmmMessages generator runs again..!\
              \n */ \n\n" ,
          tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);


  fprintf(fp,"#ifndef %s_H_INCLUDED\n",fsp->functionName);
  fprintf(fp,"#define %s_H_INCLUDED\n\n\n",fsp->functionName);

  fprintf(fp,"#include <stdio.h> \n\n");
  fprintf(fp,"#include <string.h> \n\n");
  fprintf(fp,"#include \"mmapBridge.h\" \n\n");

  fprintf(fp,"const char * pathToMMAP%s=\"%s/%s.mmap\";",functionName,pathToMMap,functionName);

  fprintf(fp,"static struct bridgeContext %sBridge={0};\n\n",functionName);

//----------------------------------------------------------------------------------------------
  fprintf(fp,"struct %sMessage\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  unsigned long timestampInit;\n"); //We always want the first bytes to be like this
  fprintf(fp,"  void * callbackOnNewData;\n");
  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
      writeStruct( fp , fsp->contents[i].str );
  }
  fprintf(fp,"};\n\n");
//----------------------------------------------------------------------------------------------

  fprintf(fp,"\n\n/** @brief This is the static memory location where we receive stuff so we don't even have to declare this..*/\n");
  fprintf(fp,"static struct %sMessage %sStatic={0};\n\n",functionName,functionName);

  fprintf(fp,"\n\n/** @brief Register a callback that will get called when %s is updated*/\n",functionName);
  fprintf(fp,"static int registerCallbackOnNewData_%s(void * callback)\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"    %sBridge.callbackOnNewData = callback;\n",functionName);
  fprintf(fp,"    return 1;\n");
  fprintf(fp,"}\n\n");



  fprintf(fp,"static int new_%s_MessageAvailiable(struct bridgeContext * nbc, struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp," if (nbc==0)       { return 0; }\n");
  fprintf(fp," if (nbc->mode!=1) { return 0; }\n");
  fprintf(fp," struct bridgePayloadHeader * incomingCommand = (struct %sMessage *) nbc->map;\n",functionName);
  fprintf(fp," if (nbc->lastMsgTimestamp != incomingCommand->timestampInit ) { return 1; }\n");
  fprintf(fp,"return 0;\n");
  fprintf(fp,"}\n");


  fprintf(fp,"\n\n/** @brief Send a %s message through the bridge\n",functionName);
  fprintf(fp,"* @ingroup stringParsing\n");
  fprintf(fp,"* @param The bridge context\n");
  fprintf(fp,"* @param The message to send\n");
  fprintf(fp,"* @retval See above enumerator*/\n");
  fprintf(fp,"static int write_%s(struct bridgeContext * nbc,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp," #if DEBUG_PRINT\n");
  fprintf(fp,"    printf(\"\\nWriting %s message to bridge.. \\n \");\n",functionName);
  fprintf(fp," #endif\n");
  fprintf(fp,"   if (writeBridge(nbc, (void*) msg , sizeof(struct %sMessage) ) )\n",functionName);
  fprintf(fp,"   {\n");
  fprintf(fp,"    nbc->lastMsgTimestamp = msg->timestampInit;\n");
  fprintf(fp,"    return 1;\n");
  fprintf(fp,"   }\n");
  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\n/** @brief Receive a %s message through the bridge\n",functionName);
  fprintf(fp,"* @ingroup stringParsing\n");
  fprintf(fp,"* @param The bridge context\n");
  fprintf(fp,"* @param The message to receive\n");
  fprintf(fp,"* @retval See above enumerator*/\n");
  fprintf(fp,"static int read_%s(struct bridgeContext * nbc,struct %sMessage* msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  if (readBridge(nbc, (void*) msg , sizeof(struct %sMessage) ) )\n",functionName);
  fprintf(fp,"  {\n");

  fprintf(fp,"      if (nbc->callbackOnNewData!=0)\n");
  fprintf(fp,"          {\n");
  fprintf(fp,"             void ( *DoCallback) ( struct %sMessage * ) = 0 ;\n",functionName);
  fprintf(fp,"             DoCallback=nbc->callbackOnNewData;\n");
  fprintf(fp,"             DoCallback(msg);\n");
  fprintf(fp,"          }\n");

  fprintf(fp,"    nbc->lastMsgTimestamp = msg->timestampInit;\n");
  fprintf(fp,"    return 1;\n");
  fprintf(fp,"  }\n");
  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n\n");


  fprintf(fp,"\n\n/** @brief Empty bridge  state for %s */\n",functionName);
  fprintf(fp,"static int empty_%s()\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  memset(&%sStatic,0,sizeof(struct %sMessage));\n",functionName,functionName);
  fprintf(fp,"  return 1;\n");
  fprintf(fp,"}\n\n");



  fprintf(fp,"\n\n/** @brief This call crafts a request to %s.html and serializes our message*/\n",functionName);
  fprintf(fp,"static int packToHTTPGETRequest_%s(char * buffer,unsigned int bufferSize,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  return snprintf(buffer,bufferSize,");
      writeGETPrintf(fp,fsp,functionName);
  fprintf(fp,");\n");
  fprintf(fp,"}\n\n");


  fprintf(fp,"\n\n/** @brief Print message for %s */\n",functionName);
  fprintf(fp,"static int print_%s(struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp," char buffer[2049]={0}; unsigned int bufferSize=2048;\n");
  fprintf(fp," packToHTTPGETRequest_%s(buffer,bufferSize,msg);\n",functionName);
  fprintf(fp," printf(\"%%s\",buffer);\n");
  fprintf(fp," fflush(stdout);\n");
  fprintf(fp," return 1;\n");
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\n/** @brief Initialize a bridge to write values %s */\n",functionName);
  fprintf(fp,"static int initializeForWriting_%s()\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"if ( initializeWritingBridge(&%sBridge , pathToMMAP%s , sizeof(struct %sMessage)) )",functionName,functionName,functionName);
  fprintf(fp,"    { \n");
  fprintf(fp,"      struct %sMessage empty={0};\n",functionName);
  fprintf(fp,"      empty.timestampInit = rand()%%10000;\n");
  fprintf(fp,"        if ( write_%s(&%sBridge,&empty) ) { return 1; } else { return 0; }\n ",functionName,functionName);
  fprintf(fp,"    } else { return 0; }\n");
  fprintf(fp,"}\n\n");



  fprintf(fp,"\n\n/** @brief Initialize a bridge to read values %s */\n",functionName);
  fprintf(fp,"static int initializeForReading_%s()\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"if ( initializeReadingBridge(&%sBridge , pathToMMAP%s , sizeof(struct %sMessage)) )",functionName,functionName,functionName);
  fprintf(fp,"    { \n");
  fprintf(fp,"      %sStatic.timestampInit = rand()%%10000;\n",functionName);
  fprintf(fp,"        if ( read_%s(&%sBridge,&%sStatic) ) { return 1; } else { return 0; }\n ",functionName,functionName,functionName);
  fprintf(fp,"    } else { return 0; }\n");
  fprintf(fp,"}\n\n");



  // ------------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have AmmarServer included then we don't need the rest of the code*/\n");
  fprintf(fp,"#ifdef AMMSERVERLIB_H_INCLUDED\n");

  fprintf(fp,"\n\n/** @brief This is the Resource handler context that will manage requests to %s.html */\n",functionName);
  fprintf(fp,"static struct AmmServer_RH_Context %sRH={0};\n",functionName);
    writeServerCallback(
                           fp ,
                           functionName,
                           fsp
                       );
  fprintf(fp,"\n\n/** @brief This function adds %s.html to the webserver and makes it listen for GET commands and forward them to the mmaped structure %sStatic */\n",functionName,functionName);
  fprintf(fp,"static int %sAddToHTTPServer(struct AmmServer_Instance * instance)\n",functionName);
  fprintf(fp,"{\n");

  fprintf(fp,"if ( initializeForWriting_%s() )",functionName);
  fprintf(fp,"    { AmmServer_Success(\"Successfully initialized mmaped bridge\");");
  fprintf(fp,"    }   else");
  fprintf(fp,"    { AmmServer_Error(\"Could not initialize mmaped bridge \");      }");

  fprintf(fp,"  return AmmServer_AddResourceHandler(instance,&%sRH,\"/%s.html\",2048+sizeof(struct %sMessage),0,&%sHTTPServer,SAME_PAGE_FOR_ALL_CLIENTS);\n",functionName,functionName,functionName,functionName);
  fprintf(fp,"}\n\n");

  fprintf(fp,"static int %sRemoveFromHTTPServer(struct AmmServer_Instance * instance)\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  AmmServer_RemoveResourceHandler(instance,&%sRH,1); \n",functionName);
  fprintf(fp,"  closeWritingBridge(&%sBridge);\n",functionName);
  fprintf(fp,"  return 1;\n");
  fprintf(fp,"}\n");
  fprintf(fp,"#endif\n\n");
//----------------------------------------------------------------------------------------------


//------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have AmmarClient included then we won't use it and we don't need the rest of the code*/\n");
  fprintf(fp,"#ifdef AMMCLIENT_H_INCLUDED\n");
  fprintf(fp,"\n\n/** @brief If this instance of your code is running on a machine that is connected through TCP/IP and you want to send your data you can do it using this call %s.html */\n",functionName);
  fprintf(fp,"static int sendToServer_%s(struct AmmClient_Instance * instance,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
   fprintf(fp,"char buffer[2049]={0}; unsigned int bufferSize=2048;\n");
   fprintf(fp,"packToHTTPGETRequest_%s(buffer,bufferSize,msg);\n",functionName);

   fprintf(fp,"char http[2049]={0}; unsigned int httpSize=2048;\n");
   fprintf(fp,"snprintf(http,httpSize,\"GET %%s HTTP/1.1\\nConnection: keep-alive\\n\\n\",buffer);\n");
   fprintf(fp,"return AmmClient_Send(instance,http,strlen(http),1);\n");
  fprintf(fp,"}\n");
  fprintf(fp,"#endif\n\n");
//------------------------------------------------------------------------




  fprintf(fp,"#endif\n");

  fclose(fp);
  return 1;
}





