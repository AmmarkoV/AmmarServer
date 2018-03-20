#include <stdio.h>
#include <stdlib.h>

#include "../StringRecognizer/fastStringParser.h"



int compileMessage(const char * filename)
{
  struct fastStringParser * fsp = fastSTringParser_createRulesFromFile(filename,64);



  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    fprintf(stderr,"STRING %u : %s\n",i,fsp->contents[i].str);
  }


  return 1;
}







int main(int argc, char *argv[])
{
  if (argc<2) { fprintf(stderr,"Please add a filename string as a parameter\n"); return 1; }

  if (strcmp(argv[1],"-msg")==0)
  {
     compileMessage(argv[2]);
  } else
  if (strcmp(argv[1],"-srv")==0)
  {

  }


  return 0;
}
