#include "messageGenerator.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>

#include "centralIncludeGenerator.h"

#include "../StringRecognizer/fastStringParser.h"
#include "../InputParser/InputParser_C.h"

#define MAXIMUM_FILENAME_WITH_EXTENSION 1024
#define MAXIMUM_LINE_LENGTH 1024

                         //    0                 1                    2                         3           4                5
                         // .MSG TYPE       REAL C TYPE         CONVERT TO STRING            printf type comparison         cast from string for comparison
char * varTypesList[] = {   "int64"   , "unsigned long %s;\n" , "%s=atoi(%s);"               , "%lu"  ,  "%s==%s"          ,  "%s==atoi(%s)" ,
                            "int32"   , "unsigned int %s;\n"  , "%s=atoi(%s);"               , "%u"   ,  "%s==%s"          ,  "%s==atoi(%s)" ,
                            "int"     , "unsigned int %s;\n"  , "%s=atoi(%s);"               , "%u"   ,  "%s==%s"          ,  "%s==atoi(%s)" ,
                            "float"   , "float %s;\n"         , "%s=atof(%s);"               , "%0.5f",  "%s==%s"          ,  "%s==atof(%s)" ,
                            "float32" , "float %s;\n"         , "%s=atof(%s);"               , "%0.5f",  "%s==%s"          ,  "%s==atof(%s)" ,
                            "float64" , "double %s;\n"        , "%s=atof(%s);"               , "%f"   ,  "%s==%s"          ,  "%s==atof(%s)" ,
                            "bool"    , "int %s;\n"           , "%s=atoi(%s);"               , "%u"   ,  "%s==%s"          ,  "%s==atoi(%s)" ,
                            "string"  , "char %s[512];\n"     , "snprintf(%s,512,\"%%s\",%s);", "%s"  ,  "strcmp(%s,%s)==0",  "strcmp(%s,%s)==0" ,
                            //DONT FORGET TO UPDATE  varTypesListNumber
                            "fail"    , "fail"                , "fail"                        ,"fail" ,  "fail"            , "fail"
                        };
#define numberOfFields 6
unsigned int varTypesListNumber = 8;



int typeExists(const char * rosType)
{
  int retval=0;
  unsigned int i=0;
  for (i=0; i<varTypesListNumber; i++)
  {
     if (strcasecmp(rosType,varTypesList[i*numberOfFields+0])==0) { retval=1; }
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
     if (strcasecmp(rosType,varTypesList[i*numberOfFields+0])==0) { fprintf(fp,varTypesList[i*numberOfFields+1],variableID); retval=1; }
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
     if (strcasecmp(rosType,varTypesList[i*numberOfFields+0])==0) { fprintf(fp,varTypesList[i*numberOfFields+2],varDestination,varName); retval=1; }
  }
 return retval;
}



int writeComparison(
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
     if (strcasecmp(rosType,varTypesList[i*numberOfFields+0])==0) { fprintf(fp,varTypesList[i*numberOfFields+4],varDestination,varName); retval=1; }
  }
 return retval;
}


int writeComparisonFromString(
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
     if (strcasecmp(rosType,varTypesList[i*numberOfFields+0])==0) { fprintf(fp,varTypesList[i*numberOfFields+5],varDestination,varName); retval=1; }
  }
 return retval;
}



void writeGETScanf(
                    FILE * fp ,
                    struct fastStringParser * fsp,
                    const char * functionName
                   )
{
 fprintf(fp,"char variableValue[256]={0};\n");
 fprintf(fp,"char variableID[256]={0};\n");
 fprintf(fp,"struct InputParserC * ipc = InputParser_Create(512,3);\n");
 fprintf(fp,"InputParser_SetDelimeter(ipc,0,'?');\n");
 fprintf(fp,"InputParser_SetDelimeter(ipc,1,'&');\n");
 fprintf(fp,"InputParser_SetDelimeter(ipc,2,'=');\n");


// fprintf(fp,"const char newLine[]={10,13,10,13,0};\n");
// fprintf(fp,"const char newLine[]={10,10,0};\n");
 fprintf(fp,"unsigned int endOfLine=0;\n");
 fprintf(fp,"char * body = strstrDoubleNewline(buffer,bufferSize,&endOfLine);\n");
 fprintf(fp,"if (body==0) {  fprintf(stderr,\"Couldnt find body.. \\n\");  body=buffer; } \n");

 fprintf(fp,"int arguments = InputParser_SeperateWordsCC(ipc,body,1);\n");
 //fprintf(fp,"fprintf(stderr,\"Unpacked to %%u arguments (%%s) \",arguments,body);\n");

 fprintf(fp,"unsigned int i=0;\n");
 fprintf(fp,"unsigned int numberOfLoops=(unsigned int) arguments/2;\n");
 fprintf(fp,"for (i=0; i<numberOfLoops; i++)\n");
 fprintf(fp," {\n");
 //fprintf(fp,"   fprintf(stderr,\" Loop i = %%u / %%u \\n \",i,numberOfLoops);  \n");
   fprintf(fp,"unsigned int item=(i*2)+1;\n");
 //fprintf(fp,"   fprintf(stderr,\" Item = %%u / %%u \\n \",item,numberOfLoops);  \n");
 fprintf(fp,"    if (\n");
 fprintf(fp,"        ( InputParser_GetWord(ipc,item,variableID,256) ) && \n");
 fprintf(fp,"        ( InputParser_GetWord(ipc,item+1,variableValue,256) ) \n");
 fprintf(fp,"       )\n");
 fprintf(fp,"    {\n");


 //fprintf(fp,"    fprintf(stderr,\"ID = %%s , Value = %%s \\n \",variableID,variableValue);\n");
  //------------------------------------------------------------
  //------------------------------------------------------------
  //------------------------------------------------------------
  //------------------------------------------------------------
  //Well shit.. This is truly messy Forget all of the above this is an if then block that checks if variableID is equal to something else..

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

          fprintf(fp," if ( strcmp(\"%s\",variableID)==0 )",variableID);
          fprintf(fp,"   {  \n");

          writeConversion(
                          fp ,
                          variableType,
                          destination,
                          "variableValue"
                         );

          fprintf(fp,"    //fprintf(stderr,\"Unpacked %s = %%s \\n \",variableValue);\n",destination);
          fprintf(fp,"    } else \n");
        }
    }
  }
  fprintf(fp," {   } \n");

  InputParser_Destroy(ipc);

  //------------------------------------------------------------
  //------------------------------------------------------------
  //------------------------------------------------------------
  //------------------------------------------------------------



 fprintf(fp,"    }\n");

  fprintf(fp," else { fprintf(stderr,\"Failed to resolve for i = %%u \\n \",i);  } \n");

 fprintf(fp," }\n");
 fprintf(fp,"InputParser_Destroy(ipc);\n");
 fprintf(fp,"return 1;\n");
}



void writeGETPrintf(
                 FILE * fp ,
                 struct fastStringParser * fsp,
                 const char * functionName
                )
{

 fprintf(fp,"  return snprintf(buffer,bufferSize,");

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
       if (strcasecmp(variableType,varTypesList[z*numberOfFields+0])==0)
         {
              fprintf(fp,"%s&",varTypesList[z*numberOfFields+3]);
              ++numberOfArguments;
         }
     }
    }
  }
  fprintf(fp,"serialNumber=%%lu&");
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
       if (strcasecmp(variableType,varTypesList[z*numberOfFields+0])==0)
         {
          fprintf(fp,"msg->%s,",variableID);
          ++secondCountOfArguments;
         }
     }
    }
  }
  fprintf(fp,"msg->serialNumber,");
  fprintf(fp,"rand()%%10000");

  InputParser_Destroy(ipc);
}




int writeServerViewerCallback(
                              FILE * fp ,
                              const char *  functionName,
                              struct fastStringParser * fsp
                             )
{
  fprintf(fp,"\n/** @brief This is the callback when someone requests %sViewer.html that allows HTTP clients to check the state of a message*/\n",functionName);
  fprintf(fp,"static void * %sHTTPServerViewer(struct AmmServer_DynamicRequest  * rqst)\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"    read_%s(&%sBridge,&%sStatic);\n",functionName,functionName,functionName);
  fprintf(fp,"    packToHTTPGETRequest_%s(rqst->content,rqst->MAXcontentSize,&%sStatic);\n",functionName,functionName);
  fprintf(fp,"    rqst->contentSize=strlen(rqst->content);\n");
  fprintf(fp,"    return 0; \n");
  fprintf(fp,"}\n");

  return 1;
}



int writeServerWriterCallback(
                           FILE * fp ,
                           const char *  functionName,
                           struct fastStringParser * fsp
                       )
{
  fprintf(fp,"\n/** @brief This is the callback when someone requests %s.html in order to write a new message*/\n",functionName);
  fprintf(fp,"static void * %sHTTPServerWriter(struct AmmServer_DynamicRequest  * rqst)\n",functionName);
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


  fprintf(fp,"snprintf(rqst->content,rqst->MAXcontentSize,\"<html><body>SUCCESS</body></html>\");\n");
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




char* replaceChar(char* str, char find, char replace)
{
    char *current_pos = strchr(str,find);
    while (current_pos)
        {
         *current_pos = replace;
          current_pos = strchr(current_pos,find);
        }
    return str;
}


char * strLastchr(char * input , char delim)
{
   char * lastChr = strchr(input,delim);
   if (lastChr==0) { return 0; }

   char * thisChr = strchr(lastChr,delim);
   while (thisChr!=0)
    {
        lastChr = thisChr;
        thisChr = strchr(lastChr+1,delim);
    }

   return lastChr;
}


int compileMessage(const char * filename,char * label,const char * pathToMMap)
{
  struct fastStringParser * fsp = fastSTringParser_createRulesFromFile(filename,64);

  char *  functionName = label;
  char filenameWithExtension[MAXIMUM_FILENAME_WITH_EXTENSION+1]={0};
  //PRINT OUT THE HEADER
  snprintf(filenameWithExtension,MAXIMUM_FILENAME_WITH_EXTENSION,"%s.h",functionName);


  char * ommitPath = strLastchr(label,'/'); //strchr(label,'/');
  if (ommitPath!=0)
  {
    functionName = ommitPath+1;
  }

  //replaceChar(functionName,'/','_');
  //replaceChar(functionName,'\\','_');
  //replaceChar(functionName,'!','_');

  unsigned int functionNameLength = strlen(functionName);
  fsp->functionName  = (char* ) malloc(sizeof(char) * (1+functionNameLength));
  if (fsp->functionName==0) { fprintf(stderr,"Could not allocate memory for function name\n"); return 0; }
  strncpy(fsp->functionName,functionName,functionNameLength);
  fsp->functionName[functionNameLength]=0;

  convertTo_ENUM_ID(fsp->functionName);







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
                 \nThis file was automatically generated @ %02d-%02d-%02d %02d:%02d:%02d using AmmMessages %s \
                 \nhttps://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmMessages\
                 \nPlease note that changes you make here may be automatically overwritten \
                 \nif the AmmMessages generator runs again..!\
              \n */ \n\n" ,
          tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec,AMM_MESSAGES_VERSION);


  fprintf(fp,"#ifndef %s_H_INCLUDED\n",fsp->functionName);
  fprintf(fp,"#define %s_H_INCLUDED\n\n\n",fsp->functionName);

  fprintf(fp,"#include <stdio.h>\n");
  fprintf(fp,"#include <string.h>\n");
  fprintf(fp,"#include \"mmapBridge.h\"\n\n\n");



   fprintf(fp,"#ifdef __cplusplus\n");
   fprintf(fp,"extern \"C\"\n");
   fprintf(fp,"{\n");
   fprintf(fp,"#endif\n");


//  fprintf(fp,"const char * pathToMMAP%s=\"%s/%s.mmap\";",functionName,pathToMMap,functionName);
   fprintf(fp,"/** @brief This is the path to mmap descriptor*/\n");
   fprintf(fp,"static const char * pathToMMAP%s=\"%s/%s.mmap\";\n",functionName,pathToMMap,functionName);

   fprintf(fp,"/** @brief This is the bridge structure that holds the mmap information*/\n");
   fprintf(fp,"static struct bridgeContext %sBridge={0};\n\n",functionName);

//----------------------------------------------------------------------------------------------
  fprintf(fp,"\n/** @brief This is the automatic structure generated by the .msg files you provded to AmmMessages*/\n");
  fprintf(fp,"struct %sMessage\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  //These 2 fields exist in all messages and are included automatically for book keeping..!\n");
  fprintf(fp,"  unsigned long timestampInit; //We always want the first bytes to be like this \n");
  fprintf(fp,"  unsigned long serialNumber;  //It is helpful to automatically count when we do write operations\n");
  fprintf(fp,"  //The rest are taken from the .msg file you provided to AmmMessage Generator\n");
  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
      writeStruct( fp , fsp->contents[i].str );
  }
  fprintf(fp,"};\n\n");
//----------------------------------------------------------------------------------------------

  fprintf(fp,"\n\n/** @brief This is the static memory location where we receive stuff so we don't even have to declare this..*/\n");
  fprintf(fp,"static struct %sMessage %sStatic={0};\n\n",functionName,functionName);

  fprintf(fp,"\n/** @brief Typedef is needed to declare callbacks in C++*/\n");
  fprintf(fp,"typedef void (*callback_%s)(struct %sMessage *);\n\n",functionName,functionName);


  fprintf(fp,"\n\n/** @brief Register a callback that will get called when %s is updated*/\n",functionName);
  fprintf(fp,"static int registerCallbackOnNewData_%s(callback_%s callback)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"    %sBridge.callbackOnNewData = (void*) callback;\n",functionName);
  fprintf(fp,"    return 1;\n");
  fprintf(fp,"}\n\n");



  fprintf(fp,"static int new_%s_MessageAvailiable(struct bridgeContext * nbc, struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp," if (nbc==0)       { return 0; }\n");
  fprintf(fp," if (nbc->mode!=1) { return 0; }\n");
  fprintf(fp," struct %sMessage * incomingCommand = (struct %sMessage *) nbc->map;\n",functionName,functionName);
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
  fprintf(fp,"    printf(\"Writing %s message to bridge.. \\n \");\n",functionName);
  fprintf(fp," #endif\n");
  fprintf(fp,"   ++msg->serialNumber; \n");
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
  fprintf(fp,"             DoCallback=(void ( *) ( struct %sMessage * )) nbc->callbackOnNewData;\n",functionName);
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
      writeGETPrintf(fp,fsp,functionName);
  fprintf(fp,");\n");
  fprintf(fp,"}\n\n");



  // ------------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have InputParser included then we don't need the tokenization code*/\n");
  fprintf(fp,"#ifdef  _INPUTPARSER_C_H_\n");
  fprintf(fp,"static int unpackFromHTTPGETRequest_%s(struct %sMessage * msg,char * buffer,unsigned int bufferSize)\n",functionName,functionName);
  fprintf(fp,"{\n");
      writeGETScanf(fp,fsp,functionName);
  fprintf(fp,"}\n");
  fprintf(fp,"#else\n");
  fprintf(fp,"static int unpackFromHTTPGETRequest_%s(struct %sMessage * msg,char * buffer,unsigned int bufferSize)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  fprintf(stderr,RED \"You have forgotten to #include InputParser_C.h before all message headers when compiling..\\n \" NORMAL);");
  fprintf(fp,"  fprintf(stderr,RED \"Please do that and recompile if you want to access unpackFromHTTPGETRequest_%s \\n  \" NORMAL);",functionName);
  fprintf(fp,"  return 0;\n");
  fprintf(fp,"}\n");
  fprintf(fp,"#endif //_INPUTPARSER_C_H_\n");



  fprintf(fp,"\n\n/** @brief Print message for %s */\n",functionName);
  fprintf(fp,"static int print_%s(struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp," char buffer[2049]={0}; unsigned int bufferSize=2048;\n");
  fprintf(fp," packToHTTPGETRequest_%s(buffer,bufferSize,msg);\n",functionName);
  fprintf(fp," printf(\"%%s\",buffer);\n");
  fprintf(fp," fflush(stdout);\n");
  fprintf(fp," return 1;\n");
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\n/** @brief Initialize a bridge to write %s values */\n",functionName);
  fprintf(fp,"static int initializeForWriting_%s()\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"if ( initializeWritingBridge(&%sBridge , pathToMMAP%s , sizeof(struct %sMessage)) )",functionName,functionName,functionName);
  fprintf(fp,"    { \n");
  fprintf(fp,"      struct %sMessage empty={0};\n",functionName);
  fprintf(fp,"      empty.timestampInit = rand()%%10000;\n");
  fprintf(fp,"        if ( write_%s(&%sBridge,&empty) ) { return 1; } else { return 0; }\n ",functionName,functionName);
  fprintf(fp,"    } else { return 0; }\n");
  fprintf(fp,"}\n\n");



  fprintf(fp,"\n\n/** @brief Initialize a bridge to read %s values */\n",functionName);
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
  fprintf(fp,"static struct AmmServer_RH_Context %sWriterRH={0};\n",functionName);
  fprintf(fp,"/** @brief This is the Resource handler context that will manage requests to %sViewer.html */\n",functionName);
  fprintf(fp,"static struct AmmServer_RH_Context %sViewerRH={0};\n",functionName);


   writeServerWriterCallback(
                              fp ,
                              functionName,
                              fsp
                             );


   writeServerViewerCallback(
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

  fprintf(fp,"  int i=0;\n");
  fprintf(fp,"  i+= AmmServer_AddResourceHandler(instance,&%sViewerRH,\"/%sViewer.html\",4096+sizeof(struct %sMessage),0,(void*) &%sHTTPServerViewer,SAME_PAGE_FOR_ALL_CLIENTS);\n",functionName,functionName,functionName,functionName);
  fprintf(fp,"  i+= AmmServer_AddResourceHandler(instance,&%sWriterRH,\"/%s.html\",4096+sizeof(struct %sMessage),0,(void*) &%sHTTPServerWriter,SAME_PAGE_FOR_ALL_CLIENTS);\n",functionName,functionName,functionName,functionName);

  fprintf(fp,"  //Always serve fresh page through dynamic requests\n");
  fprintf(fp,"  AmmServer_DoNOTCacheResourceHandler(instance,&%sViewerRH);\n",functionName);
  fprintf(fp,"  AmmServer_DoNOTCacheResourceHandler(instance,&%sWriterRH);\n",functionName);
  fprintf(fp,"  return (i==2); \n");
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\n/** @brief This function removes %s messages from the webserver and closes the mmaped memory etc*/\n",functionName);
  fprintf(fp,"static int %sRemoveFromHTTPServer(struct AmmServer_Instance * instance)\n",functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  AmmServer_RemoveResourceHandler(instance,&%sViewerRH,1); \n",functionName);
  fprintf(fp,"  AmmServer_RemoveResourceHandler(instance,&%sWriterRH,1); \n",functionName);
  fprintf(fp,"  closeWritingBridge(&%sBridge);\n",functionName);
  fprintf(fp,"  return 1;\n");
  fprintf(fp,"}\n");
  fprintf(fp,"#endif //AMMSERVERLIB_H_INCLUDED \n\n");
//----------------------------------------------------------------------------------------------


//------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have AmmarClient included then we won't use it and we don't need the rest of the code*/\n");
  fprintf(fp,"#ifdef AMMCLIENT_H_INCLUDED\n");
  fprintf(fp,"\n\n/** @brief If this instance of your code is running on a machine that is connected through TCP/IP and you want to send your data you can do it using this call %s.html */\n",functionName);
  fprintf(fp,"static int tryToSendToServer_%s(struct AmmClient_Instance * instance,struct %sMessage * msg,unsigned int tries,unsigned int maxTries)\n",functionName,functionName);
  fprintf(fp,"{\n");
   fprintf(fp,"char buffer[2049]={0}; unsigned int bufferSize=2048;\n");
   fprintf(fp,"packToHTTPGETRequest_%s(buffer,bufferSize,msg);\n",functionName);

   fprintf(fp,"char http[2049]={0}; unsigned int httpSize=2048;\n");
   //fprintf(fp,"snprintf(http,httpSize,\"GET %%s HTTP/1.1\\nConnection: keep-alive\\n\\n\",buffer);\n");

   fprintf(fp,"    if ( AmmClient_RecvFile(instance,buffer,http,&httpSize,1/*keep connection alive*/,1/*really fast*/) )\n");
   fprintf(fp,"    {\n");
   fprintf(fp,"      if (strstr(http,\"SUCCESS\")!=0)\n");
   fprintf(fp,"      {\n");
   fprintf(fp,"        //fprintf(stderr,\"Got back %%s\\n\",http);\n");
   fprintf(fp,"        return 1;\n");
   fprintf(fp,"      }\n");
   fprintf(fp,"    }\n");
   fprintf(fp," fprintf(stderr,RED \"Failed to send %s message ( try %%u/%%u )..\\n\" NORMAL , tries , maxTries);\n",functionName);
   fprintf(fp," fprintf(stderr,\"Got back %%s\\n\",http);\n");
   fprintf(fp," return 0;\n");
  fprintf(fp,"}\n");


  fprintf(fp,"\n\n/** @brief This function sends a message %s to a webserver via AmmClient*/\n",functionName);
  fprintf(fp,"static int sendToServer_%s(struct AmmClient_Instance * instance,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"   ++msg->serialNumber; \n");
  fprintf(fp,"   int MAXtries=5;\n");
  fprintf(fp,"   int tries=0;\n");
  fprintf(fp,"   while (tries<MAXtries)\n");
  fprintf(fp,"   {\n");
  fprintf(fp,"     if (tryToSendToServer_%s(instance,msg,tries,MAXtries)) { return 1; }\n",functionName);
  fprintf(fp,"     ++tries;\n");
  fprintf(fp,"   }\n");
   fprintf(fp," fprintf(stderr,RED \" All tries to send %s message failed..\\n\" NORMAL);\n",functionName);
  fprintf(fp,"  return 0;\n");
  fprintf(fp,"}\n");


  //------------------------------------------------------------------------
  fprintf(fp,"#ifdef  _INPUTPARSER_C_H_\n");

  fprintf(fp,"\n\n/** @brief This function tries to update the given message from a webserver via AmmClient */\n");
  fprintf(fp,"static int tryToReadStateFromServer_%s(struct AmmClient_Instance * instance,struct %sMessage * msg,unsigned int tries,unsigned int maxTries)\n",functionName,functionName);
  fprintf(fp,"{\n");
   fprintf(fp,"char http[4097]={0}; unsigned int httpSize=4096;\n");
   fprintf(fp,"    if ( AmmClient_RecvFile(instance,\"%sViewer.html\",http,&httpSize,1/*keep connection alive*/,1/*really fast*/) )\n",functionName);
   fprintf(fp,"    {\n");
   fprintf(fp,"     //fprintf(stderr,\"Got back %%s\\n\",http);\n");
   fprintf(fp,"     return unpackFromHTTPGETRequest_%s(msg,http,httpSize);\n",functionName);
   fprintf(fp,"    }\n");
   fprintf(fp," fprintf(stderr,RED \"Failed to read full state for %s message ( try %%u/%%u )..\\n\" NORMAL , tries , maxTries);\n",functionName);
   fprintf(fp," return 0;\n");
   fprintf(fp,"}\n");


  fprintf(fp,"\n\n/** @brief This function reads %s message state from a webserver via AmmClient it retries 5 times and returns 1 for success and 0 when it fails*/\n",functionName);
  fprintf(fp,"static int readStateFromServer_%s(struct AmmClient_Instance * instance,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"   int MAXtries=5;\n");
  fprintf(fp,"   int tries=0;\n");
  fprintf(fp,"   while (tries<MAXtries)\n");
  fprintf(fp,"   {\n");
  fprintf(fp,"     if (tryToReadStateFromServer_%s(instance,msg,tries,MAXtries)) { return 1; }\n",functionName);
  fprintf(fp,"     ++tries;\n");
  fprintf(fp,"   }\n");
   fprintf(fp," fprintf(stderr,RED \" All tries to read state for %s message failed..\\n\" NORMAL);\n",functionName);
  fprintf(fp,"  return 0;\n");
  fprintf(fp,"}\n");

  fprintf(fp,"#else\n");
  fprintf(fp,"\n\n/** @brief This function is a dummy function with the same signature as the real one\n");
  fprintf(fp,"   and is only injected when you haven't included AmmClient correctly to keep linking errors at bay*/\n");
  fprintf(fp,"static int readStateFromServer_%s(struct AmmClient_Instance * instance,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  fprintf(stderr,RED \"You have forgotten to #include InputParser_C.h before all message headers when compiling..\\n \" NORMAL);");
  fprintf(fp,"  fprintf(stderr,RED \"Please do that and recompile if you want to access readStateFromServer_%s \\n  \" NORMAL);",functionName);
  fprintf(fp,"  return 0;\n");
  fprintf(fp,"}\n");

  fprintf(fp,"#endif //_INPUTPARSER_C_H_\n\n");
  //------------------------------------------------------------------------



  fprintf(fp,"#endif //AMMCLIENT_H_INCLUDED \n\n");
//------------------------------------------------------------------------


  fprintf(fp,"#ifdef __cplusplus\n");
  fprintf(fp,"}\n");
  fprintf(fp,"#endif\n");



  fprintf(fp,"#endif\n");

  fclose(fp);
  return 1;
}





