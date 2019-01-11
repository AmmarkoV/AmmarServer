#include <stdio.h>     /* files */
#include <stdlib.h>     /* qsort */
#include <string.h>     /* memset */


#include "csvparse.h"


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */


struct CSVParser *  csvParserCreate( const char * delimiter , unsigned int numberOfDelimiters)
{
    struct CSVParser * newParser = malloc(sizeof(struct CSVParser));
    if (newParser!=0)
    {
      memset(newParser,0,sizeof(struct CSVParser));
      newParser->delimiters=delimiter;
      newParser->numberOfDelimiters=numberOfDelimiters;
      return newParser;
    }
   return 0;
}

int csvParser_StartParsingFile(struct CSVParser * csv,const char * filename)
{
    if (csv==0) { fprintf(stderr,"csvParser: csvParserStartParsingFile cannot load file `%s` without an allocated hashmap structure \n",filename); return 0; }

    csv->handle = fopen(filename,"r");
    if (csv->handle!=0)
    {
     return 1;
    }

 return 0;
}



int csvParser_StopParsingFile(struct CSVParser * csv)
{
    if (csv==0) { return 0; }
    if (csv->lastLine!=0) { free(csv->lastLine); csv->lastLine=0; }
    if (csv->handle!=0)
    {
       fclose(csv->handle);
    }

    free(csv);
    return 0;
}

char * csvParser_FindAnyDelimiter(char * line,const char * delimiterString,unsigned int numberOfDelimiters)
{
  fprintf(stderr,"csvParser_FindAnyDelimiter\n");

  if (line==0) { return 0; }
  char * ptr = line;
  unsigned int i=0,count=0;

  while (*ptr!=0)
  {
    fprintf(stderr,"character %u is %c(%u)\n",count,*ptr,*ptr);

    if ((*ptr==0)||(*ptr==10)||(*ptr==13))
    {
      return ptr;
    }
      else
    {
     for (i=0; i<numberOfDelimiters; i++)
     {
       //fprintf(stderr," comparing to %u delimiters\n",numberOfDelimiters);
       if (delimiterString[i]==*ptr)
       {
        return ptr;
       }
     }
    }

    ++count;
    ++ptr;
  }

  return 0;
}


unsigned int csvParser_CountNumberOfFields(struct CSVParser * csv)
{
   if (csv==0) { return 0; }
   csv->numberOfFields=0;
   if (csv->lastLine==0) { return 0; }

   fprintf(stderr,"csvParser_CountNumberOfFields\n");
   char * r = csv->lastLine;
   ++csv->numberOfFields;
   fprintf(stderr,"r=%s\n",r);
   while ( (r!=0)&&(*r!=0))
    {
      r = csvParser_FindAnyDelimiter(r,csv->delimiters,csv->numberOfDelimiters);
      if (r!=0)
      {
       ++csv->numberOfFields;
       ++r;
      }
    }

   fprintf(stderr,"Number of fields %u\n",csv->numberOfFields);
   csv->numberOfDelimitersCounted=1;
   return csv->numberOfFields;
}

unsigned int csvParser_GetNumberOfFields(struct CSVParser * csv)
{
   if (csv==0) { return 0; }
   if (csv->numberOfDelimitersCounted)
    {
      return csv->numberOfFields;
    } else
    {
      if ( csvParser_CountNumberOfFields(csv) )
      {
       return csv->numberOfFields;
      }
    }
  return 0;
}



int csvParser_ParseNextLine(struct CSVParser * csv)
{
  if (csv->handle!=0)
  {
    ssize_t read;
    if ((read = getline(&csv->lastLine, &csv->lastLineLength, csv->handle)) != -1)
    {
      ++csv->linesParsed;
      return 1;
    }
  }
  return 0;
}


char * csvParser_GetField(struct CSVParser * csv,unsigned int fieldNumber)
{
  if (csv->lastDelimiterSetToNull)
  {
    *csv->lastDelimiterSetToNullPtr = csv->previousDelimiterValue;
    csv->lastDelimiterSetToNull=0;
  }

  fprintf(stderr,"csvParser_GetField(%s,field=%u)\n",csv->lastLine,fieldNumber);

  if (csv->handle!=0)
  {
    if (csv->lastLine!=0)
    {
      if ( (csv->haveAFieldResult) && (fieldNumber == csv->fieldIDOflastDelimiter) )
      { //We are asking for the same result..!
        return csv->lastFieldResult;
      } else
      /*
      if (fieldNumber == csv->fieldIDOflastDelimiter + 1)
      { //We are having a sequential access pattern..!
        char * nextDelimiter = csvParser_FindAnyDelimiter(csv->lastFieldResult,csv->delimiters,csv->numberOfDelimiters);
        if (nextDelimiter!=0)
        {

        }
      } else */
      { //Do whole work from scratch..
        unsigned int i=0;
        char * startOfVariable = csv->lastLine;

        for (i=0; i<fieldNumber; i++)
        {
          startOfVariable = csvParser_FindAnyDelimiter(startOfVariable,csv->delimiters,csv->numberOfDelimiters);
          if (startOfVariable!=0) { startOfVariable+=1; }
        }

        char * endOfVariable = csvParser_FindAnyDelimiter(startOfVariable,csv->delimiters,csv->numberOfDelimiters);
        csv->previousDelimiterValue = *endOfVariable;
        csv->fieldIDOflastDelimiter = fieldNumber;
        csv->haveAFieldResult=1;
        csv->lastDelimiterSetToNull=1;
        csv->lastDelimiterSetToNullPtr = endOfVariable;
        csv->lastFieldResult = startOfVariable;
        *endOfVariable=0;
        return startOfVariable;
      }
    }
  }

  csv->haveAFieldResult=0;
  return 0;
}


