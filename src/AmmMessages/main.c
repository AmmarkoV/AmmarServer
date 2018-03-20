#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>

#include "../StringRecognizer/fastStringParser.h"



#define MAXIMUM_FILENAME_WITH_EXTENSION 1024
#define MAXIMUM_LINE_LENGTH 1024
#define MAXIMUM_LEVELS 123
#define ACTIVATED_LEVELS 3


int compileMessage(const char * filename,const char * label)
{
  struct fastStringParser * fsp = fastSTringParser_createRulesFromFile(filename,64);



  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    fprintf(stderr,"STRING %u : %s\n",i,fsp->contents[i].str);
  }


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

  fprintf(fp,"#include \"mmapBridge.h\" \n\n",fsp->functionName);




  fprintf(fp,"struct %sMessage\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  unsigned long timestampInit;\n",functionName);
  fprintf(fp,"};\n\n");








  fprintf(fp,"\n\n/** @brief Send a %s message through the bridge\n",functionName);
  fprintf(fp,"* @ingroup stringParsing\n");
  fprintf(fp,"* @param The bridge context\n");
  fprintf(fp,"* @param The message to send\n");
  fprintf(fp,"* @retval See above enumerator*/\n");
  fprintf(fp,"static int write_%s(struct bridgeContext * nbc,struct %sMessage * msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"    return writeBridge(nbc, (void*) msg , sizeof(struct %sMessage) );\n",functionName);
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\n/** @brief Receive a %s message through the bridge\n",functionName);
  fprintf(fp,"* @ingroup stringParsing\n");
  fprintf(fp,"* @param The bridge context\n");
  fprintf(fp,"* @param The message to receive\n");
  fprintf(fp,"* @retval See above enumerator*/\n");
  fprintf(fp,"int read_%s(struct bridgeContext * nbc,struct %sMessage* msg)\n",functionName,functionName);
  fprintf(fp,"{\n");
  fprintf(fp,"  if (readBridge(nbc, (void*) msg , sizeof(struct %sMessage) ) )\n",functionName);
  fprintf(fp,"  {\n");
  fprintf(fp,"    nbc->lastMsgTimestamp = msg->timestampInit;\n");
  fprintf(fp,"    return 1;\n");
  fprintf(fp,"  }\n");
  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n\n");


  fprintf(fp,"#endif\n");

  fclose(fp);
  return 1;
}







int main(int argc, char *argv[])
{
  if (argc<2) { fprintf(stderr,"Please add a filename string as a parameter\n"); return 1; }

  if (strcmp(argv[1],"-msg")==0)
  {
     compileMessage(argv[2],argv[3]);
  } else
  if (strcmp(argv[1],"-srv")==0)
  {

  }


  return 0;
}
