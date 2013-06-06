#include <stdio.h>
#include <stdlib.h>

#include "../AmmServerlib/tools/fastStringParser.h"


int main(int argc, char *argv[])
{
  if (argc<1) { fprintf(stderr,"Please add a filename string as a parameter\n"); return 1; }

  struct fastStringParser * fsp = fastSTringParser_createRulesFromFile(argv[1],40);
  return 0;
}
