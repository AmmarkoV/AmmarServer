#include "centralIncludeGenerator.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <time.h>


int gatherEverything(int argc, char *argv[])
{
  int i=0;
  FILE * fp = fopen("allAmmMessages.h","wb");
  if (fp == 0) { fprintf(stderr,"Could not open output file allAmmMessages.h\n"); return 0; }
  fflush(fp);
  fprintf(fp,"\n");


  fprintf(fp,"/** @file allAmmMessages.h\n");
  fprintf(fp,"* @brief A tool that generates code that can span messages across computers\n");
  fprintf(fp,"* @author Ammar Qammaz (AmmarkoV)\n");
  fprintf(fp,"*/\n\n");

  fprintf(fp,"#ifndef ALL_AMMMESSAGES_H_INCLUDED\n");
  fprintf(fp,"#define ALL_AMMMESSAGES_H_INCLUDED\n\n\n");

  fprintf(fp,"#include \"mmapBridge.h\" \n\n");
   for (i=2; i<argc; i++)
    { fprintf(fp,"#include \"%s.h\" \n",argv[i]); }


  fprintf(fp,"\n\nstatic int initializeAllMessagesForWriting()\n");
  fprintf(fp,"{\n");
    for (i=2; i<argc; i++) { fprintf(fp," initializeForWriting_%s(); \n",argv[i]); }
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\nstatic int initializeAllMessagesForReading()\n");
  fprintf(fp,"{\n");
    for (i=2; i<argc; i++) { fprintf(fp," initializeForReading_%s(); \n",argv[i]); }
  fprintf(fp,"}\n\n");

  fprintf(fp,"\n\nstatic int sampleAllMessages()\n");
  fprintf(fp,"{\n");
    for (i=2; i<argc; i++)
    {
      fprintf(fp," if (newBridgeMessageAvailiable(&%sBridge)) {  \n",argv[i]);
        fprintf(fp," read_%s(&%sBridge,&%sStatic); }  \n",argv[i],argv[i],argv[i]);
    }
  fprintf(fp,"}\n\n");


//------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have AmmarServer included then we won't use it and we don't need the rest of the code*/\n");
  fprintf(fp,"#ifdef AMMSERVERLIB_H_INCLUDED\n");
  fprintf(fp,"static int addAllToHTTPServer(struct AmmServer_Instance * instance)\n");
  fprintf(fp,"{\n");
    for (i=2; i<argc; i++)
    { fprintf(fp,"%sAddToHTTPServer(instance);\n",argv[i]); }
  fprintf(fp,"}\n\n");


  fprintf(fp,"static int removeAllFromHTTPServer(struct AmmServer_Instance * instance)\n");
  fprintf(fp,"{\n");
      for (i=2; i<argc; i++)
    { fprintf(fp,"%sRemoveFromHTTPServer(instance);\n",argv[i]); }
  fprintf(fp,"}\n\n");
  fprintf(fp,"#endif\n\n");
//------------------------------------------------------------------------


//------------------------------------------------------------------------
  fprintf(fp,"\n\n/** @brief If we don't have AmmarClient included then we won't use it and we don't need the rest of the code*/\n");
  fprintf(fp,"#ifdef AMMCLIENT_H_INCLUDED\n");
  fprintf(fp,"static int sendAllToServer(struct AmmClient_Instance * instance)\n");
  fprintf(fp,"{\n");
   // for (i=2; i<argc; i++)
   // { fprintf(fp,"%sAddToHTTPServer(instance);\n",argv[i]); }
  fprintf(fp,"}\n\n");
  fprintf(fp,"#endif\n\n");
//------------------------------------------------------------------------








  fprintf(fp,"#endif\n");

  fclose(fp);
  return 1;
}
