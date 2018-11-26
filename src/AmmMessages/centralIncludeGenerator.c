#include "centralIncludeGenerator.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>


const char AMM_MESSAGES_VERSION[]={"v1.00002"};

int gatherEverything(int argc, char *argv[])
{
  int i=0;
  FILE * fp = fopen("allAmmMessages.h","wb");
  if (fp == 0) { fprintf(stderr,"Could not open output file allAmmMessages.h\n"); return 0; }
  fflush(fp);
  fprintf(fp,"\n");


  fprintf(fp,"/** @file allAmmMessages.h\n");
  fprintf(fp,"* @brief A tool that generates code that can span messages across computers\n");
  fprintf(fp,"* automatically generated with AmmMessages %s\n",AMM_MESSAGES_VERSION);
  fprintf(fp,"* @author Ammar Qammaz (AmmarkoV)\n");
  fprintf(fp,"*/\n\n");

  fprintf(fp,"#ifndef ALL_AMMMESSAGES_H_INCLUDED\n");
  fprintf(fp,"#define ALL_AMMMESSAGES_H_INCLUDED\n\n\n");

  fprintf(fp,"#include \"mmapBridge.h\" \n\n");
   for (i=2; i<argc; i++)
    { fprintf(fp,"#include \"%s.h\" \n",argv[i]); }



   fprintf(fp,"#ifdef __cplusplus\n");
   fprintf(fp,"extern \"C\"\n");
   fprintf(fp,"{\n");
   fprintf(fp,"#endif\n");

  fprintf(fp,"\n\nstatic int initializeAllMessagesForWriting()\n");
  fprintf(fp,"{\n");
  fprintf(fp," unsigned int count=0;\n");
    for (i=2; i<argc; i++) { fprintf(fp," count+=initializeForWriting_%s(); \n",argv[i]); }
  fprintf(fp," return (count==%u);\n",argc-2);
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\nstatic int initializeAllMessagesForReading()\n");
  fprintf(fp,"{\n");
  fprintf(fp," unsigned int count=0;\n");
    for (i=2; i<argc; i++) { fprintf(fp," count+=initializeForReading_%s(); \n",argv[i]); }
  fprintf(fp," return (count==%u);\n",argc-2);
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\nstatic int sampleAllMessages()\n");
  fprintf(fp,"{\n");
  fprintf(fp," unsigned int count=0;\n");
    for (i=2; i<argc; i++)
    {
      fprintf(fp," if (new_%s_MessageAvailiable(&%sBridge,&%sStatic)) {  \n",argv[i],argv[i],argv[i]);
        fprintf(fp," count+=read_%s(&%sBridge,&%sStatic); }  \n",argv[i],argv[i],argv[i]);
    }
  fprintf(fp," return (count==%u);\n",argc-2);
  fprintf(fp,"}\n\n");


//------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have AmmarServer included then we won't use it and we don't need the rest of the code*/\n");
  fprintf(fp,"#ifdef AMMSERVERLIB_H_INCLUDED\n");
  fprintf(fp,"static int addAllToHTTPServer(struct AmmServer_Instance * instance)\n");
  fprintf(fp,"{\n");
  fprintf(fp," unsigned int count=0;\n");
    for (i=2; i<argc; i++)
    { fprintf(fp,"count+=%sAddToHTTPServer(instance);\n",argv[i]); }
  fprintf(fp," return (count==%u);\n",argc-2);
  fprintf(fp,"}\n\n");


  fprintf(fp,"static int removeAllFromHTTPServer(struct AmmServer_Instance * instance)\n");
  fprintf(fp,"{\n");
  fprintf(fp," unsigned int count=0;\n");
      for (i=2; i<argc; i++)
    { fprintf(fp,"count+=%sRemoveFromHTTPServer(instance);\n",argv[i]); }
  fprintf(fp," return (count==%u);\n",argc-2);
  fprintf(fp,"}\n\n");
  fprintf(fp,"#endif\n\n");
//------------------------------------------------------------------------

  fprintf(fp,"#ifdef __cplusplus\n");
  fprintf(fp,"}\n");
  fprintf(fp,"#endif\n");


  fprintf(fp,"#endif\n");

  fclose(fp);
  return 1;
}
