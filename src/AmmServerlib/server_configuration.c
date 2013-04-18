#include "server_configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools/http_tools.h"
#include "InputParser/InputParser_C.h"


/*
   THERE ARE THREE KIND OF CONFIGURATION OPTIONS ,
      A ) SOME THAT ARE CONSTANT , AND DEFINED IN CONFIGURATION.H
      B ) SOME THAT ARE COMMON FOR ALL INSTANCES , AND CAN BE MODIFIED ON RUNTIME , YOU CAN SEE THEM UNDER HERE..
      C ) SOME THAT ARE INSTANCE SPECIFIC , AND THEY ARE DECLARED IN  AmmServerlib.h INSIDE struct AmmServer_Instance_Settings  ..
*/

char USERNAME_UID_FOR_DAEMON[MAX_FILE_PATH]="www-data";  //one interesting value here is `whoami` since it will input the username of the current user :P
int  CHANGE_TO_UID=1500; //Non superuser system

signed int CHANGE_PRIORITY=0;

int varSocketTimeoutREAD_ms=10 /*Seconds*/  *1000;
int varSocketTimeoutWRITE_ms=10 /*Seconds*/ * 1000;

//CACHE
unsigned char CACHING_ENABLED=1;
unsigned char DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=1;
int MAX_SEPERATE_CACHE_ITEMS = 2000;
int MAX_CACHE_SIZE_IN_MB = 128;
int MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB = 3;




int AccessLogEnable=0;
char AccessLog[MAX_FILE_PATH]="access.log";

int ErrorLogEnable=0;
char ErrorLog[MAX_FILE_PATH]="error.log";


char TemplatesInternalURI[MAX_RESOURCE]="_asvres_/";
//Please note that the file server limits the filenames _asvres_/filename.jpg is OK
//a filename like _asvres_/filenamemplampla.jpg will return a 404

// ------------------------------------------------------------------------------------------------------

int EmmitPossibleConfigurationWarnings(struct AmmServer_Instance * instance)
{
  fprintf(stderr,"TODO: TOP PRIORITY -> Implement POST !FILE! requests , and couple them to dynamic content ..\n");
  fprintf(stderr,"TODO: Implement download resume capabilities ( range head request ) ..\n");
  fprintf(stderr,"TODO: require the Host: header from HTTP 1.1 clients\n");
  fprintf(stderr,"TODO: accept absolute URL's in a request\n");
  fprintf(stderr,"TODO: accept requests with chunked data\n");
  fprintf(stderr,"TODO: use the \"100 Continue\" response appropriately\n");
  fprintf(stderr,"TODO: handle requests with If-Modified-Since: or If-Unmodified-Since: headers\n");
  fprintf(stderr,"TODO: Add configuration file ammServ.conf parsing..\n");
  fprintf(stderr,"TODO: Add detailed input header parsing\n");
  fprintf(stderr,"TODO: Improve directory listings ( add filesizes , dates etc ) \n");
  fprintf(stderr,"TODO: Improve implemented file caching mechanism ( add string comparison to make code hash collision free ) \n");
  fprintf(stderr,"TODO: Improve dynamic content handling ( coming from programs statically linked to the webserver ) ..\n");
  fprintf(stderr,"TODO: Add apache like logging capabilities\n");


  if (instance->settings.BINDING_PORT<=1000 )
   {
     warning("Please note that for binding ports below 1000 you need root privileges \nPlease also note that running this non hardened version as root is not recommended..\n");
   }


  if (ENABLE_COMPRESSION)
   {
       fprintf(stderr,"Please note that compression doesn't yet emmit the payload as per HTTP  RFC 1950, RFC 1951, and RFC 1952 \n");
       fprintf(stderr,"To make things worse content sensing is kind of broken since it is based on the GetContentType TEXT type instead of a user configurable extension blacklist/whitelist\n");
   }
  return 1;
}






/*! MAJOR TODO :P , so that we can parse the configuration file.. !*/
static void ParseConfigString(struct AmmServer_Instance * instance,struct InputParserC * ipc,char * inpt)
{
  unsigned int words_count = InputParser_SeperateWords(ipc,inpt,0);
  if ( words_count > 0 )
    {
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"TIMEOUT"))
        {
            int timeout_value = InputParser_GetWordInt(ipc,1);
            varSocketTimeoutREAD_ms = timeout_value*1000;
            varSocketTimeoutWRITE_ms = timeout_value*1000;
        } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"KEEPALIVE"))
        {
             int keepalive_value = 0;
             if (InputParser_WordCompareNoCaseAuto(ipc,1,(char*)"ON")) { keepalive_value =1; }
        } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"LISTEN"))      { instance->settings.BINDING_PORT = InputParser_GetWordInt(ipc,1); } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"RUNASPRIORITY"))   { CHANGE_PRIORITY  = InputParser_GetWordInt(ipc,1); } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"RUNASUSER"))       { InputParser_GetWord(ipc,1,USERNAME_UID_FOR_DAEMON,MAX_FILE_PATH); } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"IFUSERDOESNTEXISTRUNASUID")) { CHANGE_TO_UID = InputParser_GetWordInt(ipc,1); } else
      //CONFIGURE CACHING BEHAVIOUR
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"CACHING"))                 { CACHING_ENABLED=0; if (InputParser_WordCompareNoCaseAuto(ipc,1,(char*)"ON")) { CACHING_ENABLED=1; } } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXCACHESIZE"))            { MAX_CACHE_SIZE_IN_MB = InputParser_GetWordInt(ipc,1); } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXCACHESIZEFOREACHFILE")) {  MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB = InputParser_GetWordInt(ipc,1); } else
      if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXSEPERATECACHEITEMS"))   { MAX_SEPERATE_CACHE_ITEMS = InputParser_GetWordInt(ipc,1); }
    }
}


int LoadConfigurationFile(struct AmmServer_Instance * instance,char * conf_file)
{
  char line[MAX_CONFIGURATION_FILE_LINE_SIZE]={0};
  FILE * pFile;
  pFile = 0;
  if ( (conf_file!=0)&&(strlen(conf_file)>1) ) {  pFile = fopen (conf_file ,"r");  } //We may want to open a particluar configuration file

  if (ENABLE_AUTOMATIC_CONFIGURATION_LOADING)
   { //We may want or we may not want automatic configuration loading from the files below ..
     //A particularly embedded service may not want to use this files for security reasons for example...
     if (pFile==0) {  pFile = fopen ("ammarServer.conf" ,"r");  } //First lets try for a local ammarServer.conf ( i.e. development directory scenario )
     if (pFile==0) {  pFile = fopen ("/etc/ammarServer.conf" ,"r");  } //Second try lets try for the real system installation scenario ( /etc/ammarServer.conf )
     if (pFile==0) {  pFile = fopen ("default.conf" ,"r");  } //If we can't find a regular conf file lets check for a default one
   }

  if (pFile!=0 )
    {

       struct InputParserC * ipc=0;
       ipc = InputParser_Create(MAX_CONFIGURATION_FILE_LINE_SIZE,1);
       InputParser_SetDelimeter(ipc,0,' ');

      int line_length=0;
      int c=0;
      do
        {
          c = getc (pFile);

          if ( MAX_CONFIGURATION_FILE_LINE_SIZE-1 <= line_length )
             {
               fprintf(stderr,"Oveflow while loading configuration file \n");
               line[MAX_CONFIGURATION_FILE_LINE_SIZE-1]=0;
               ParseConfigString(instance,ipc,line);
               line_length=0;
             } else
          if (c == '\n')
            {
              ParseConfigString(instance,ipc,line);
              line_length=0;
            }
          else
            {
              line[line_length]=c;
              ++line_length;
              line[line_length]=0; // always append null termination ;P
            }
        }
      while (c != EOF);
      fclose (pFile);

      InputParser_Destroy(ipc);

      return 1;
    }
  else
    {
      fprintf(stderr,"Cannot find and open an AmmarServer Configuration file \n");
      if (!ENABLE_AUTOMATIC_CONFIGURATION_LOADING) { fprintf(stderr,"Please note that ENABLE_AUTOMATIC_CONFIGURATION_LOADING=0 in configuration.h \n"); }
    }

  return 0;
}

/*Kind of twisted function TODO : improve it :P*/
int AssignStr(char ** dest , char * source)
{
    //We want to replace **dest with a pointer to a valid point
    //in memory that contains a copy of source.. :P

   //Beginning we would like to free any allocation and set dest to point at null!
   if (*dest!=0) { free(*dest); }
   *dest=0;
   //If the source is null , so is the dest
   if (source==0) { return 1; }

   unsigned int source_len=strlen(source);
   *dest = malloc(sizeof(char) * (source_len+1) );

   if (*dest==0) { fprintf(stderr,"Could not allocate string to assign value\n"); return 0; }
   char * destination = *dest;

   destination[0]=0;
   strncpy(destination,source,source_len);
   destination[source_len]=0; // <- This should happen automatically but we reinforce it here..!

   return 1;
}


int SetUsernameAndPassword(struct AmmServer_Instance * instance,char * username,char * password)
{

  unsigned int pass_size = 2; // : and \0
  if ( username !=0 ) { pass_size += strlen(username); }
  if ( password !=0 ) { pass_size += strlen(password); }

  if ( pass_size>MAX_QUERY ) { fprintf(stderr,"Error : SetUsernameAndPassword was given a huge string to convert..!"); return 0; }

  char * mixed_string = malloc(sizeof (char) * pass_size );
  if (mixed_string==0) { fprintf(stderr,"Error : Could not allocate memory in SetUsernameAndPassword\n"); return 0; }

  mixed_string[0]=0;
  if (username!=0) { strcpy(mixed_string,username); }
  strcat(mixed_string,":");
  if (password!=0) { strcat(mixed_string,password); }



  char * base64pass = malloc(sizeof (char) *  ((pass_size*2)+1 ));
  if (base64pass==0) { fprintf(stderr,"Error : Could not allocate memory in SetUsernameAndPassword\n"); return 0; }
  base64pass[0]=0;

  int result=encodeToBase64(mixed_string,strlen(mixed_string),base64pass,pass_size*2);
  if (result)
   { fprintf(stderr,"\nUsername and Password %s converted to %s \n",mixed_string,base64pass);
     AssignStr(&instance->settings.BASE64PASSWORD,base64pass);
     instance->settings.PASSWORD_PROTECTION=1;
   } else
   { fprintf(stderr,"\nCould not encode Username and Password %s \n",mixed_string); }

   free(mixed_string);
   free(base64pass);

 return result;
}
