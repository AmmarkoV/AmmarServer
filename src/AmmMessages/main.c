#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messageGenerator.h"
#include "centralIncludeGenerator.h"


int main(int argc, char *argv[])
{
  if (argc<2) { fprintf(stderr,"Please add a filename string as a parameter\n"); return 1; }

  if (strcmp(argv[1],"-gather")==0)
  {
     gatherEverything(argc,argv);
  } else
  if (strcmp(argv[1],"-msg")==0)
  {
     compileMessage(argv[2],argv[3],argv[4]);
  } else
  if (strcmp(argv[1],"-srv")==0)
  {
     fprintf(stderr,"Services not implemented..\n");
  }


  return 0;
}
