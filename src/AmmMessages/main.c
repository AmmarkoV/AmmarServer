#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messageGenerator.h"
#include "centralIncludeGenerator.h"


int main(int argc, char *argv[])
{
  if (argc<2) { fprintf(stderr,"Please add a filename string as a parameter\n"); return 1; }

  //Failures used to exit 0 , and -msg read argv[2..4] without checking they exist
  if (strcmp(argv[1],"-gather")==0)
  {
     return (gatherEverything(argc,argv)) ? 0 : 1;
  } else
  if (strcmp(argv[1],"-msg")==0)
  {
     if (argc<5) { fprintf(stderr,"Usage : %s -msg file.msg name pathToMMap\n",argv[0]); return 1; }
     return (compileMessage(argv[2],argv[3],argv[4])) ? 0 : 1;
  } else
  if (strcmp(argv[1],"-srv")==0)
  {
     fprintf(stderr,"Services not implemented..\n");
  }


  return 1;
}
