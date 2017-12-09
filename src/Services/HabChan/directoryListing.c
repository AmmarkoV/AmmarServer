#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#include "directoryListing.h"

int dirmain (void)
{
  DIR *dp;
  struct dirent *ep;
  dp = opendir ("./");

  if (dp != NULL)
  {
    while (ep = readdir (dp))
      puts (ep->d_name);

    (void) closedir (dp);
  }
  else
    perror ("Couldn't open the directory");

  return 0;
}
