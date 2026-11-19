#include <stdio.h>
#include <stdlib.h>

#include "fastStringParser.h"


int main(int argc, char *argv[])
{
  if (argc<2) { fprintf(stderr,"Please add a filename string as a parameter\n"); return 1; } //argv[0] is the program

  struct fastStringParser * fsp = fastSTringParser_createRulesFromFile(argv[1],64);
  if (fsp==0) { return 1; }

  int ok = export_C_Scanner(fsp,argv[1]);
  fastStringParser_close(fsp);

  return (ok) ? 0 : 1; //Failures used to exit 0 , so generateAmmServerScanners.sh carried on with stale files
}
