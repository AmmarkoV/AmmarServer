#include "fastStringParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fastStringParser * fspHTTPHeader = 0;

#define MAXIMUM_LINE_LENGTH 1024

enum fpsHTTPHeaderCodes
{
  NO_STRING = 0 ,
  AUTHORIZATION ,
  RANGE ,
  REFERRER ,
  REFERER ,
  HOST,
  ACCEPT_ENCODING ,
  DEFLATE,
  USER_AGENT,
  COOKIE,
  KEEP_ALIVE,
  IF_NONE_MATCH ,
  IF_MODIFIED_SINCE

};

int fastStringParser_addString(struct fastStringParser * fsp, char * str)
{
  unsigned int ourNum = fsp->stringsLoaded++;
  fsp->contents[ourNum].strLength=strlen(str);
  fsp->contents[ourNum].str = (char *) malloc(sizeof(char) * (fsp->contents[ourNum].strLength+1) );
  if (fsp->contents[ourNum].str != 0 )
  {
    strncpy(fsp->contents[ourNum].str,str,fsp->contents[ourNum].strLength);
    fsp->contents[ourNum].str[fsp->contents[ourNum].strLength]=0; // Null terminator

    return 1;
  }
  return 0;
}



struct fastStringParser *  fastStringParser_initialize(unsigned int totalStrings)
{
   fspHTTPHeader = (struct fastStringParser * ) malloc(sizeof( struct fastStringParser ));
   if (fspHTTPHeader== 0 ) { return 0; }

   fspHTTPHeader->stringsLoaded = 0;
   fspHTTPHeader->MAXstringsLoaded = totalStrings;
   fspHTTPHeader->contents = (struct fspString * ) malloc(sizeof( struct fspString )*fspHTTPHeader->MAXstringsLoaded);
   if (fspHTTPHeader->contents== 0 ) { return 0; }

  return fspHTTPHeader;
}



int fastStringParser_initializeHardCoded()
{
   fspHTTPHeader = fastStringParser_initialize(20);

   fastStringParser_addString(fspHTTPHeader,"AUTHORIZATION:");
   fastStringParser_addString(fspHTTPHeader,"RANGE:");
   fastStringParser_addString(fspHTTPHeader,"REFERRER:");
   fastStringParser_addString(fspHTTPHeader,"REFERER:");
   fastStringParser_addString(fspHTTPHeader,"HOST:");
   fastStringParser_addString(fspHTTPHeader,"ACCEPT-ENCODING:");
   fastStringParser_addString(fspHTTPHeader,"DEFLATE");
   fastStringParser_addString(fspHTTPHeader,"USER-AGENT:");
   fastStringParser_addString(fspHTTPHeader,"COOKIE:");
   fastStringParser_addString(fspHTTPHeader,"CONNECTION:");
   fastStringParser_addString(fspHTTPHeader,"KEEP-ALIVE");
   fastStringParser_addString(fspHTTPHeader,"IF-NONE-MATCH:");
   fastStringParser_addString(fspHTTPHeader,"IF-MODIFIED-SINCE:");

   return 1;
}




struct fastStringParser * fastSTringParser_createRulesFromFile(char* filename,unsigned int totalStrings)
{
  FILE * fp = fopen(filename,"r");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",filename); return 1; }

  struct fastStringParser *  fsp  = fastStringParser_initialize(totalStrings);
  if (fsp==0) { return 0; }

  char line[MAXIMUM_LINE_LENGTH]={0};
  unsigned int lineLength=0;
  while (fgets(line,MAXIMUM_LINE_LENGTH,fp)!=0)
  {
      lineLength = strlen(line);
      if ( lineLength > 0 )
        {
         if (line[lineLength-1]==10) { line[lineLength-1]=0; --lineLength; }
         if (line[lineLength-1]==13) { line[lineLength-1]=0; --lineLength; }
        }
      if ( lineLength > 1 )
        {
         if (line[lineLength-2]==10) { line[lineLength-2]=0; --lineLength; }
         if (line[lineLength-2]==13) { line[lineLength-2]=0; --lineLength; }
        }

    fprintf(stderr,"LINE : `%s`\n",line);
    fastStringParser_addString(fsp,line);
  }
  fclose(fp);


  return fsp;
}





int fastStringParser_close()
{


    return 0;
}




