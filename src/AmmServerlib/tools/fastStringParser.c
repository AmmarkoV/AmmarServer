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

  if ( (ourNum==0) || (fsp->shortestStringLength<fsp->contents[ourNum].strLength) )
      { fsp->shortestStringLength = fsp->contents[ourNum].strLength; }

  if ( (ourNum==0) || (fsp->longestStringLength>fsp->contents[ourNum].strLength) )
      { fsp->longestStringLength  = fsp->contents[ourNum].strLength; }

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
   fastStringParser_addString(fspHTTPHeader,"ACCEPT-ENCODING:");
   fastStringParser_addString(fspHTTPHeader,"COOKIE:");
   fastStringParser_addString(fspHTTPHeader,"CONNECTION:");
   fastStringParser_addString(fspHTTPHeader,"DEFLATE");
   fastStringParser_addString(fspHTTPHeader,"HOST:");
   fastStringParser_addString(fspHTTPHeader,"IF-NONE-MATCH:");
   fastStringParser_addString(fspHTTPHeader,"IF-MODIFIED-SINCE:");
   fastStringParser_addString(fspHTTPHeader,"KEEP-ALIVE");
   fastStringParser_addString(fspHTTPHeader,"RANGE:");
   fastStringParser_addString(fspHTTPHeader,"REFERRER:");
   fastStringParser_addString(fspHTTPHeader,"REFERER:");
   fastStringParser_addString(fspHTTPHeader,"USER-AGENT:");

   return 1;
}

int fastStringParser_hasStringsWithCharAtIndex(struct fastStringParser * fsp,unsigned int index, char character )
{
  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    if (fsp->contents[i].str[index]==character) { return 1; }
  }
  return 0;
}


int fastStringParser_hasStringsWith2CharsAtIndexes(struct fastStringParser * fsp,unsigned int index1, char character1,unsigned int index2, char character2 )
{
  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    if ( (fsp->contents[i].str[index1]==character1) &&
         (fsp->contents[i].str[index2]==character2) )
         { return 1; }
  }
  return 0;
}


int fastStringParser_hasStringsWith3CharsAtIndexes(struct fastStringParser * fsp,unsigned int * resString, unsigned int index1, char character1,unsigned int index2, char character2 ,unsigned int index3, char character3 )
{
  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    if ( (fsp->contents[i].str[index1]==character1) &&
         (fsp->contents[i].str[index2]==character2) &&
         (fsp->contents[i].str[index3]==character3) )
         {
          *resString = i ;
          return 1; }
  }
  return 0;
}


int export_C_Scanner(struct fastStringParser * fsp,char * functionName)
{

  char filenameWithExtension[1024]={0};
  sprintf(filenameWithExtension,"%s.c",functionName);
  FILE * fp = fopen(filenameWithExtension,"w");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",functionName); return 0; }

  fprintf(fp,"#include <stdio.h>\n\n");
  fprintf(fp,"int scanFor%s(char * str) \n{\n",functionName);

  unsigned int charA='A',charB='A',charC='A';

 fprintf(fp," switch (str[0]) { \n");
  while (charA <= 'Z')
  {
   // -------------------    FIRST CHARACTER      --------------------------
   if (   fastStringParser_hasStringsWithCharAtIndex(fsp,0,charA)  )
   {

    fprintf(fp," case \'%c\' : \n",charA);
    fprintf(fp," switch (str[1]) \n  { \n");
    while (charB <= 'Z')
    {

     // -------------------    SECOND CHARACTER      --------------------------
     if (  fastStringParser_hasStringsWith2CharsAtIndexes(fsp,0,charA,1,charB)  )
     {
     fprintf(fp,"   case \'%c\' : \n",charB);
     fprintf(fp,"   switch (str[2]) \n   { \n");
     while (charC <= 'Z')
     {
       // -------------------    THIRD CHARACTER      --------------------------
       unsigned int lastIndex = 0 ;
       if (  fastStringParser_hasStringsWith3CharsAtIndexes(fsp,&lastIndex,0,charA,1,charB,2,charC) )
       {
         fprintf(fp,"     case \'%c\' : \n",charC);
         fprintf(fp,"     //%s; \n",fsp->contents[lastIndex].str);
         fprintf(fp,"     return %u; \n",lastIndex);
         fprintf(fp,"     break; \n");

       }
       // -------------------    THIRD CHARACTER      --------------------------
       ++charC;
     }
     fprintf(fp,"   }; \n");
     fprintf(fp,"   break; \n");
     }
     // -------------------    SECOND CHARACTER      --------------------------


     ++charB;
    }
    fprintf(fp," }; \n");
    fprintf(fp," break; \n");
   }
   // -------------------    FIRST CHARACTER      --------------------------


    ++charA;
 }
 fprintf(fp,"}; \n");





/*
  unsigned int character=0;
  while ( character < fsp->longestStringLength )
  {
     fprintf(fp," switch (str[%u]) { \n",character);
     char curChar='A';
     while (curChar<='Z')
     {
       fprintf(fp,"  case '%c' : ",curChar);
       fprintf(fp,"  break; \n");
      ++curChar;
     }
    fprintf(fp,"}; \n");
   ++character;
  }
*/




  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n");

  fprintf(fp,"\n\nint main(int argc, char *argv[]) \n {\n");
  fprintf(fp,"  if (argc<1) { fprintf(stderr,\"No parameter\\n\"); return 1; }\n");
  fprintf(fp,"  if ( scanFor%s(argv[0]) ) { fprintf(stderr,\"Found it\"); } \n  return 0; \n }\n",functionName);


  fclose(fp);

  return 1;
}












struct fastStringParser * fastSTringParser_createRulesFromFile(char* filename,unsigned int totalStrings)
{
  FILE * fp = fopen(filename,"r");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",filename); return 0; }

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

    //fprintf(stderr,"LINE : `%s`\n",line);
    fastStringParser_addString(fsp,line);
  }
  fclose(fp);


  return fsp;
}





int fastStringParser_close()
{


    return 0;
}




