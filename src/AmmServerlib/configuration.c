#include "configuration.h"


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
