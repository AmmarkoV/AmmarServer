#include "configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_tools.h"

int PASSWORD_PROTECTION=0;
char * USERNAME=0;
char * PASSWORD=0;
char * BASE64PASSWORD=0;

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
   *dest[0]=0;
   strncpy(*dest,source,source_len);
   return 1;
}


int SetUsernameAndPassword(char * username,char * password)
{

  unsigned int pass_size = 2; // : and \0
  if ( username !=0 ) { pass_size += strlen(username); }
  if ( password !=0 ) { pass_size += strlen(password); }

  if ( pass_size>MAX_QUERY ) { fprintf(stderr,"Error : SetUsernameAndPassword was given a huge string to convert..!"); return 0; }

  char * mixed_string = malloc(sizeof (char) * pass_size );
  if (mixed_string==0) { fprintf(stderr,"Error : Could not allocate memory in SetUsernameAndPassword\n"); }
  mixed_string[0]=0;

  if (username!=0) { strcpy(mixed_string,username); } else { strcpy(mixed_string,""); }
  strcat(mixed_string,":");
  if (password!=0) { strcat(mixed_string,password); }



  int result =0;
  char * base64pass = malloc(sizeof (char) *  (pass_size*2) );
  if (base64pass==0) { fprintf(stderr,"Error : Could not allocate memory in SetUsernameAndPassword\n"); }
  base64pass[0]=0;

  result=encodeToBase64(mixed_string,strlen(mixed_string),base64pass,pass_size*2);
  if (result)
   { fprintf(stderr,"\nUsername and Password %s converted to %s \n",mixed_string,base64pass);
     AssignStr(&BASE64PASSWORD,base64pass);
     PASSWORD_PROTECTION=1;
   } else
   { fprintf(stderr,"\nCould not encode Username and Password %s:%s \n",mixed_string); }

   free(mixed_string);
   free(base64pass);

 return result;
}
