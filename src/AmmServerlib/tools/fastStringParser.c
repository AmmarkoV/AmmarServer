#include "fastStringParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fastStringParser * fspHTTPHeader = 0;

#define MAXIMUM_LINE_LENGTH 1024
#define MAXIMUM_LEVELS 123

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


int fastStringParser_hasStringsWithNConsecutiveChars(struct fastStringParser * fsp,unsigned int * resStringResultIndex, char * Sequence,unsigned int seqLength)
{
  unsigned int i=0,count = 0,correct=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    char * str1 = fsp->contents[i].str;
    char * str2 = Sequence;
    correct=0;
    for ( count=0; count<seqLength; count++ )
    {
      if (*str1==*str2) { ++correct; }
      ++str1;
      ++str2;
    }

    if ( correct == seqLength ) { *resStringResultIndex = i; return 1; }
  }
  return 0;
}


int fastStringParser_countStringsForNextChar(struct fastStringParser * fsp,unsigned int * resStringResultIndex,char * Sequence,unsigned int seqLength)
{
 unsigned int res=0;
 unsigned int i=0,count = 0,correct=0;

 Sequence[seqLength]='A';
 while (Sequence[seqLength] <= 'Z')
 {
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    char * str1 = fsp->contents[i].str;
    char * str2 = Sequence;

    correct=0;
    for ( count=0; count<seqLength+1; count++ )
    {
      if (*str1==*str2) { ++correct; }
      ++str1; ++str2;
    }

    if ( correct == seqLength ) { ++res; *resStringResultIndex=i; }
  }
  ++Sequence[seqLength];
 }
  return res;
}



int recursiveTraverser(FILE * fp,struct fastStringParser * fsp,char * functionName,char * cArray,unsigned int level)
{
  if (level==3) { return 0; }

  unsigned int resStringResultIndex=0;

     if ( fastStringParser_countStringsForNextChar(fsp,&resStringResultIndex,cArray,level+1)>1 )
     {
       fprintf(fp," switch (str[%u]) { \n",level);
       while (cArray[level] <= 'Z')
       {
        if ( fastStringParser_hasStringsWithNConsecutiveChars(fsp,&resStringResultIndex,cArray,level+1)  )
        {
          fprintf(fp," case \'%c\' : \n",cArray[level]);
           recursiveTraverser(fp,fsp,functionName,cArray,level+1);
          fprintf(fp," break; \n");
        }

        ++cArray[level];
       }

       fprintf(fp,"}; \n");
     } else
     {
       if (level==0)
        { fprintf(fp," case \'%c\' : \n",cArray[level]); }

       fprintf(fp," return %u; \n",resStringResultIndex);
       fprintf(fp," //%s \n",fsp->contents[resStringResultIndex].str);

       if (level==0)
       { fprintf(fp," break; \n"); }
     }

 return 1;
}




int export_C_Scanner(struct fastStringParser * fsp,char * functionName)
{

  char filenameWithExtension[1024]={0};
  sprintf(filenameWithExtension,"%s.c",functionName);
  FILE * fp = fopen(filenameWithExtension,"w");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",functionName); return 0; }

  char cArray[MAXIMUM_LEVELS]={0};
  int i=0;
  for (i=0; i<MAXIMUM_LEVELS; i++ ) { cArray[i]='A'; }


  fprintf(fp,"#include <stdio.h>\n\n");
  fprintf(fp,"int scanFor_%s(char * str) \n{\n",functionName);

  recursiveTraverser(fp,fsp,functionName,cArray,0);

  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n");

  fprintf(fp,"\n\nint main(int argc, char *argv[]) \n {\n");
  fprintf(fp,"  if (argc<1) { fprintf(stderr,\"No parameter\\n\"); return 1; }\n");
  fprintf(fp,"  if ( scanFor_%s(argv[0]) ) { fprintf(stderr,\"Found it\"); } \n  return 0; \n }\n",functionName);


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




