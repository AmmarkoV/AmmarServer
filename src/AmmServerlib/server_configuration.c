#include "server_configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "tools/http_tools.h"
#include "tools/logs.h"
#include "../InputParser/InputParser_C.h"



unsigned int GLOBAL_KILL_SERVER_SWITCH = 0;

/*
   THERE ARE THREE KIND OF CONFIGURATION OPTIONS ,
      A ) SOME THAT ARE CONSTANT , AND DEFINED IN CONFIGURATION.H
      B ) SOME THAT ARE COMMON FOR ALL INSTANCES , AND CAN BE MODIFIED ON RUNTIME , YOU CAN SEE THEM UNDER HERE..
      C ) SOME THAT ARE INSTANCE SPECIFIC , AND THEY ARE DECLARED IN  AmmServerlib.h INSIDE struct AmmServer_Instance_Settings  ..
*/

char USERNAME_UID_FOR_DAEMON[MAX_FILE_PATH]=DEFAULT_USERNAME_UID_FOR_DAEMON;  //one interesting value here is `whoami` since it will input the username of the current user :P
int  CHANGE_TO_UID=NON_ROOT_UID_IF_USER_FAILS; //Non superuser system

signed int CHANGE_PRIORITY=0;

int varSocketTimeoutREAD_seconds= DEFAULT_SOCKET_READ_TIMEOUT_SECS;
int varSocketTimeoutWRITE_seconds=DEFAULT_SOCKET_WRITE_TIMEOUT_SECS;

//KeepAlive can be switched off from ammServ.conf , in which case pending connections are closed after each response
int KEEPALIVE_ENABLED=1;

unsigned int MAX_CLIENT_PRESPAWNED_THREADS = MAX_CLIENT_PRESPAWNED_THREADS_DEFAULT;

//CACHE
unsigned char CACHING_ENABLED=1;
int MAX_SEPERATE_CACHE_ITEMS = 1024;
int MAX_CACHE_SIZE_IN_MB = 128;
int MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB = 3;

//SESSIONS
int SESSION_IDLE_TIMEOUT_SECONDS = 1800; //30 minutes
int MAX_SESSIONS = 10000;


int AccessLogEnable=1;
char AccessLog[MAX_FILE_PATH]="access.log";

int ErrorLogEnable=1;
char ErrorLog[MAX_FILE_PATH]="error.log";


char TemplatesInternalURI[MAX_RESOURCE] = TEMPLATE_INTERNAL_URI;


// ------------------------------------------------------------------------------------------------------



int instance_WeCanCommitMoreMemory(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes)
{
  if ((unsigned int) MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB*1024*1024<additional_mem_to_malloc_in_bytes) { fprintf(stderr,"This file exceedes the maximum cache size for individual files , it will not be cached\n");  return 0;  }
  if ((unsigned int) MAX_CACHE_SIZE_IN_MB*1024<instance->loaded_cache_items_Kbytes+additional_mem_to_malloc_in_bytes/1024)  { fprintf(stderr,"We have reached the soft cache limit of %u MB\n",MAX_CACHE_SIZE_IN_MB);  return 0; }
  return 1;
}


int instance_CountNewMallocOP(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes)
{
  instance->loaded_cache_items_Kbytes+=(additional_mem_to_malloc_in_bytes/1024);
  return 1;
}

int instance_CountFreeOP(struct AmmServer_Instance * instance,unsigned long additional_mem_to_malloc_in_bytes)
{
  instance->loaded_cache_items_Kbytes-=(additional_mem_to_malloc_in_bytes/1024);
  return 1;
}


int printDisclaimer()
{
  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd)) != NULL) { fprintf(stderr,"Current working dir: %s\n", cwd); }


  fprintf(stderr,"\n\nDISCLAIMER : \n");
  fprintf(stderr,"Please note that this server version is not thoroughly\n");
  fprintf(stderr," pen-tested so it is not meant for production deployment..\n");

  fprintf(stderr,"Bug reports and feedback are very welcome.. \n");
  fprintf(stderr,"via https://github.com/AmmarkoV/AmmarServer/issues\n\n");

  return 1;
}



int EmmitPossibleConfigurationWarnings(struct AmmServer_Instance * instance)
{
  /**
  @bug TOP PRIORITY -> Implement POST !FILE! requests , and couple them to dynamic content
  @bug  Add detailed input header parsing
  @bug  Improve directory listings ( add filesizes , dates etc )
  @bug  Improve implemented file caching mechanism ( add string comparison to make code hash collision free )
  @bug  Add apache like logging capabilities
  */

  if ( MAX_CLIENT_PRESPAWNED_THREADS == 0 )
  {
    warningID(ASV_WARNING_NO_PRESPAWNED_THREADS);
  }

  if (instance->settings.BINDING_PORT<=1000 )
   {
     fprintf(stderr,"Please note that for binding ports below 1000 you need root privileges \nPlease also note that running this non hardened version as root is not recommended..\n");
   }


  if (ENABLE_COMPRESSION)
   {
       fprintf(stderr,"Please note that compression doesn't yet emmit the payload as per HTTP  RFC 1950, RFC 1951, and RFC 1952 \n");
       fprintf(stderr,"To make things worse content sensing is kind of broken since it is based on the GetContentType TEXT type instead of a user configurable extension blacklist/whitelist\n");
   }
  return 1;
}






static int ConfigParseOnOff(struct InputParserC * ipc,unsigned int wordIndex,int * output,const char * key)
{
  if (InputParser_WordCompareNoCaseAuto(ipc,wordIndex,(char*)"ON"))  { *output=1; return 1; }
  if (InputParser_WordCompareNoCaseAuto(ipc,wordIndex,(char*)"OFF")) { *output=0; return 1; }
  fprintf(stderr,"Configuration : `%s` expects On or Off , ignoring the line\n",key);
  return 0;
}

static int ConfigParseInt(
                           struct InputParserC * ipc,
                           unsigned int wordIndex,
                           int minimum,
                           int maximum,
                           int * output,
                           const char * key
                         )
{
  char word[MAX_CONFIGURATION_FILE_LINE_SIZE+1]={0};
  if (!InputParser_GetWord(ipc,wordIndex,word,sizeof(word)))
   {
     fprintf(stderr,"Configuration : `%s` is missing a value , ignoring the line\n",key);
     return 0;
   }

  errno=0;
  char * end=0;
  long value=strtol(word,&end,10);
  if ( (errno!=0) || (end==word) || (*end!=0) || (value<minimum) || (value>maximum) )
   {
     fprintf(stderr,"Configuration : `%s` has an invalid value `%s` ( expected %d..%d ) , ignoring the line\n",key,word,minimum,maximum);
     return 0;
   }

  *output=(int) value;
  return 1;
}

static void ParseConfigString(struct AmmServer_Instance * instance,struct InputParserC * ipc,char * inpt)
{
  int words_count = InputParser_SeperateWords(ipc,inpt,0);
  if ( words_count <= 0 ) { return; }

  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"TIMEOUT"))
    {
      int timeout_value=0;
      if (ConfigParseInt(ipc,1,1,86400*365,&timeout_value,"Timeout"))
       {
         varSocketTimeoutREAD_seconds=timeout_value;
         varSocketTimeoutWRITE_seconds=timeout_value;
       }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"KEEPALIVE"))
    {
      int keepalive_value=KEEPALIVE_ENABLED;
      if (ConfigParseOnOff(ipc,1,&keepalive_value,"KeepAlive")) { KEEPALIVE_ENABLED=keepalive_value; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"LISTEN"))
    {
      int port=0;
      if (ConfigParseInt(ipc,1,1,65535,&port,"Listen")) { instance->settings.BINDING_PORT=port; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"RUNASPRIORITY"))
    {
      int priority=CHANGE_PRIORITY;
      if (ConfigParseInt(ipc,1,-20,20,&priority,"RunAsPriority")) { CHANGE_PRIORITY=priority; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"RUNASUSER"))
    {
      char user[MAX_FILE_PATH]={0};
      if (InputParser_GetWord(ipc,1,user,sizeof(user)))
       { strncpy(USERNAME_UID_FOR_DAEMON,user,MAX_FILE_PATH); }
      else
       { fprintf(stderr,"Configuration : `RunAsUser` is missing a value , ignoring the line\n"); }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"IFUSERDOESNTEXISTRUNASUID"))
    {
      int uid=CHANGE_TO_UID;
      if (ConfigParseInt(ipc,1,0,INT_MAX,&uid,"IfUserDoesntExistRunAsUID")) { CHANGE_TO_UID=uid; }
    } else
  //CONFIGURE CACHING BEHAVIOUR
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"CACHING"))
    {
      int caching_value=CACHING_ENABLED;
      if (ConfigParseOnOff(ipc,1,&caching_value,"Caching")) { CACHING_ENABLED=(unsigned char)caching_value; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXCACHESIZE"))
    {
      int value=MAX_CACHE_SIZE_IN_MB;
      if (ConfigParseInt(ipc,1,1,INT_MAX,&value,"MaxCacheSize")) { MAX_CACHE_SIZE_IN_MB=value; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXCACHESIZEFOREACHFILE"))
    {
      int value=MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB;
      if (ConfigParseInt(ipc,1,1,INT_MAX,&value,"MaxCacheSizeForEachFile")) { MAX_CACHE_SIZE_FOR_EACH_FILE_IN_MB=value; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXSEPERATECACHEITEMS"))
    {
      int value=MAX_SEPERATE_CACHE_ITEMS;
      if (ConfigParseInt(ipc,1,1,INT_MAX,&value,"MaxSeperateCacheItems")) { MAX_SEPERATE_CACHE_ITEMS=value; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"ENABLEPOST"))
    {
      int value=instance->settings.ENABLE_POST;
      if (ConfigParseOnOff(ipc,1,&value,"EnablePost")) { instance->settings.ENABLE_POST=value; }
    } else
  if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"MAXPOSTTRANSACTIONSIZE"))
    {
      int value=(int) instance->settings.MAX_POST_TRANSACTION_SIZE;
      if (ConfigParseInt(ipc,1,1,INT_MAX,&value,"MaxPostTransactionSize"))
       { instance->settings.MAX_POST_TRANSACTION_SIZE=(unsigned int) value; }
    } else
    {
      char key[MAX_CONFIGURATION_FILE_LINE_SIZE+1]={0};
      InputParser_GetWord(ipc,0,key,sizeof(key));
      fprintf(stderr,"Configuration : unknown key `%s` , ignoring the line\n",key);
    }
}

static FILE * OpenConfigurationFile(const char * conf_file,char * selected,unsigned int selectedSize)
{
  if ( (conf_file!=0) && (strlen(conf_file)>1) )
   {
     FILE * pFile=fopen(conf_file,"r");
     if (pFile!=0) { snprintf(selected,selectedSize,"%s",conf_file); }
     else          { fprintf(stderr,"Cannot open explicitly requested configuration file %s\n",conf_file); }
     return pFile; //An explicitly requested file never falls back to automatic discovery
   }

  if (!ENABLE_AUTOMATIC_CONFIGURATION_LOADING) { return 0; }

  const char * candidates[] =
  {
    "ammServ.conf",
    "/etc/ammServ.conf",
    "ammarServer.conf",
    "/etc/ammarServer.conf",
    "default.conf"
  };

  unsigned int i=0;
  for (i=0; i<(sizeof(candidates)/sizeof(candidates[0])); i++)
   {
     FILE * pFile=fopen(candidates[i],"r");
     if (pFile!=0)
      {
        snprintf(selected,selectedSize,"%s",candidates[i]);
        return pFile;
      }
   }

  return 0;
}

int LoadConfigurationFile(struct AmmServer_Instance * instance,const char * conf_file)
{
  char line[MAX_CONFIGURATION_FILE_LINE_SIZE]={0};
  char selected[MAX_FILE_PATH]={0};
  FILE * pFile=OpenConfigurationFile(conf_file,selected,sizeof(selected));

  if (pFile==0)
    {
      fprintf(stderr,"Cannot find and open an AmmarServer Configuration file \n");
      if (!ENABLE_AUTOMATIC_CONFIGURATION_LOADING) { fprintf(stderr,"Please note that ENABLE_AUTOMATIC_CONFIGURATION_LOADING=0 in configuration.h \n"); }
      return 0;
    }

  fprintf(stderr,"Loading configuration from %s\n",selected);

  struct InputParserC * ipc=0;
  ipc=InputParser_Create(MAX_CONFIGURATION_FILE_LINE_SIZE,2);
  InputParser_SetDelimeter(ipc,0,' ');
  InputParser_SetDelimeter(ipc,1,'\t');

  while (fgets(line,sizeof(line),pFile)!=0)
   {
     size_t lineLength=strlen(line);

     //A line longer than the buffer is skipped entirely instead of being parsed in chunks
     if ( (lineLength==sizeof(line)-1) && (line[lineLength-1]!='\n') )
      {
        int c=0;
        while ( ((c=fgetc(pFile))!='\n') && (c!=EOF) ) { }
        fprintf(stderr,"Configuration : line too long , ignoring it\n");
        continue;
      }

     while ( (lineLength>0) && ( (line[lineLength-1]=='\n') || (line[lineLength-1]=='\r') ) )
      { line[--lineLength]=0; }

     char * start=line;
     while ( (*start==' ') || (*start=='\t') ) { ++start; }
     if ( (*start==0) || (*start=='#') ) { continue; }

     ParseConfigString(instance,ipc,start);
   }

  fclose(pFile);
  InputParser_Destroy(ipc);
  return 1;
}

/*Kind of twisted function TODO : improve it :P*/
int AssignStr(char ** dest , const char * source)
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



void safeFree (void* ptr,size_t size)
{
  #if CLEAN_MEMORY_BEFORE_DEALLOCATION
    memset(ptr,0,size);
    free(ptr);
  #else
    free(ptr);
  #endif // CLEAN_MEMORY_BEFORE_DEALLOCATION
}


int SetUsernameAndPassword(struct AmmServer_Instance * instance,char * username,char * password)
{
  unsigned int pass_size = 2; // : and \0
  if ( username !=0 ) { pass_size += strlen(username); }
  if ( password !=0 ) { pass_size += strlen(password); }

  if ( pass_size>MAX_QUERY ) { fprintf(stderr,"Error : SetUsernameAndPassword was given a huge string to convert..!"); return 0; }


  unsigned long mixedStringSize = sizeof (char) * pass_size;
  char * mixed_string = malloc(mixedStringSize);
  if (mixed_string==0) { fprintf(stderr,"Error : Could not allocate memory in SetUsernameAndPassword\n"); return 0; }

  mixed_string[0]=0;
  if (username!=0) { strcpy(mixed_string,username); }
  strcat(mixed_string,":");
  if (password!=0) { strcat(mixed_string,password); }


  unsigned long base64passSize = sizeof (char) *  ((pass_size*2)+1 );
  char * base64pass = malloc(base64passSize);
  if (base64pass==0)
     {
      fprintf(stderr,"Error : Could not allocate memory in SetUsernameAndPassword\n");
      if (mixed_string!=0) { safeFree(mixed_string,mixedStringSize); }
      return 0;
     }
  base64pass[0]=0;

  int result=encodeToBase64(mixed_string,strlen(mixed_string),base64pass,pass_size*2);
  if (result)
   { fprintf(stderr,"\nUsername and Password %s converted to %s \n",mixed_string,base64pass);
     AssignStr(&instance->settings.BASE64PASSWORD,base64pass);
     instance->settings.PASSWORD_PROTECTION=1;
   } else
   { fprintf(stderr,"\nCould not encode Username and Password %s \n",mixed_string); }

   safeFree(mixed_string,mixedStringSize);
   safeFree(base64pass  ,base64passSize);

 return result;
}



