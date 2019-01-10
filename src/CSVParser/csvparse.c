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


unsigned int csvParser_GetNumberOfFields(struct CSVParser * csv)
{
   if (csv==0) { return 0; }
   return csv->numberOfFields;
}


char * csvParser_FindAnyDelimiter(char * line,const char * delimiterString,unsigned int numberOfDelimiters)
{
  char * ptr = line;
  while (*ptr!=0)
  {
    unsigned int i=0;
    for (i=0; i<numberOfDelimiters; i++)
    {
      if (delimiterString[i]==*ptr)
      {
        return ptr;
      }
    }
    ++ptr;
  }

  return 0;
}


char * csvParser_GetField(struct CSVParser * csv,unsigned int fieldNumber)
{
  if (csv->handle!=0)
  {
    if (csv->lastLine!=0)
    {
      if ( (csv->haveAFieldResult) && (fieldNumber == csv->fieldIDOflastDelimiter + 1) )
      { //We are asking for the same result..!
        return csv->lastFieldResult;
      } else
      if (fieldNumber == csv->fieldIDOflastDelimiter + 1)
      { //We are having a sequential access pattern..!

      } else
      { //Do whole work from scratch..

      }



      unsigned int i=0;


      for (i=0; i<fieldNumber-csv->fieldIDOflastDelimiter; i++)
      {
          char * delimiterLocation = csvParser_FindAnyDelimiter(csv->lastLine,csv->delimiters,csv->numberOfDelimiters);
          if (delimiterLocation!=0)
          {

          }
      }


      if (fieldNumber==0)
      {
        return csv->lastLine;
      }

      ++csv->linesParsed;
      csv->lastLine=0;
      return csv->lastLine;
    }
  }

  csv->haveAFieldResult=0;
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
