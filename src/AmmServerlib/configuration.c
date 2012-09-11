#include "configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int PASSWORD_PROTECTION=0;
char * PASSWORD=0;

int varSocketTimeoutREAD_ms=5000;
int varSocketTimeoutWRITE_ms=5000;

int AccessLogEnable=0;
char AccessLog[MAX_FILE_PATH]="access.log";

int ErrorLogEnable=0;
char ErrorLog[MAX_FILE_PATH]="error.log";


char TemplatesInternalURI[MAX_RESOURCE]="_asvres_/";
//Please note that the file server limits filenames _asvres_/filename.jpg is OK
//a filename like _asvres_/filenamemplampla.jpg will return a 404


int LoadConfigurationFile(char * conf_file)
{
  /*TODO : Stub*/
  return 0;
}

/*Kind of twisted function TODO : improve it :P*/
int AssignStr(char ** dest , char * source)
{  //THIS ISN'T WORKING!
   if (*dest!=0) { free(*dest); }
   *dest=0;
   if (source==0) { return 1; }

   unsigned int source_len=strlen(source);
   *dest = malloc(sizeof(char) * source_len);

   if (*dest==0) { return 0; }
   strncpy(*dest,source,source_len);
   return 1;
}
