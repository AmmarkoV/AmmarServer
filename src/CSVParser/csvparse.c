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


struct CSVParser *  csvParserCreate(const char * delimiter , unsigned int numberOfDelimiters)
{
    if ( (delimiter==0) && (numberOfDelimiters!=0) ) { fprintf(stderr,"csvParser: Cannot create a parser with %u null delimiters\n",numberOfDelimiters); return 0; }
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


int  csvParserDestroy(struct CSVParser * csv)
{
   if (csv==0) { return 0; }
   csvParser_StopParsingFile(csv); //Harmless if already stopped , avoids leaking the handle/line if it was not
   free(csv);
   return 0;
}


static int csvParser_IsEndOfLine(char c)
{
  return ( (c==0) || (c==10) || (c==13) );
}

//Returns a pointer to the first delimiter or end of line ( \n , \r or the terminating 0 ) , never 0 unless line is 0
char * csvParser_FindAnyDelimiter(char * line,const char * delimiterString,unsigned int numberOfDelimiters)
{
  //fprintf(stderr,"csvParser_FindAnyDelimiter\n");
  if (line==0) { return 0; }
  char * ptr = line;
  unsigned int i=0;

  while (!csvParser_IsEndOfLine(*ptr))
  {
     for (i=0; i<numberOfDelimiters; i++)
     {
       if (delimiterString[i]==*ptr)
       {
        return ptr;
       }
     }
    ++ptr;
  }

  return ptr;
}


unsigned int csvParser_CountNumberOfFields(struct CSVParser * csv)
{
   if (csv==0) { return 0; }
   csv->numberOfFields=0;
   csv->numberOfDelimitersCounted=0;
   if (csv->lastLine==0) { return 0; }

   //fprintf(stderr,"csvParser_CountNumberOfFields\n");
   //N delimiters before the end of the line separate N+1 fields ( an empty line is a single empty field )
   char * r = csv->lastLine;
   csv->numberOfFields=1;
   while (1)
    {
      r = csvParser_FindAnyDelimiter(r,csv->delimiters,csv->numberOfDelimiters);
      if (csvParser_IsEndOfLine(*r)) { break; }
      ++csv->numberOfFields;
      ++r;
    }

   //fprintf(stderr,"Number of fields %u\n",csv->numberOfFields);
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





char * csvParser_GetField(struct CSVParser * csv,unsigned int fieldNumber)
{
  //fprintf(stderr,"csvParser_GetField(%s,field=%u)\n",csv->lastLine,fieldNumber);
  if (csv==0) { return 0; }

  if (csv->handle!=0)
  {
    if ( (csv->lastLine!=0) && (fieldNumber < csvParser_GetNumberOfFields(csv)) )
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

        if (csv->lastDelimiterSetToNull)
         {
          *csv->lastDelimiterSetToNullPtr = csv->previousDelimiterValue;
          csv->lastDelimiterSetToNull=0;
         }

        unsigned int i=0;
        char * startOfVariable = csv->lastLine;

        //fieldNumber < numberOfFields so there are at least fieldNumber delimiters to step over
        for (i=0; i<fieldNumber; i++)
        {
          startOfVariable = csvParser_FindAnyDelimiter(startOfVariable,csv->delimiters,csv->numberOfDelimiters) + 1;
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



int csvParser_ParseNextLine(struct CSVParser * csv)
{
  if (csv==0) { return 0; }
  //Everything we cached points into the previous line ( which getline() will overwrite or reallocate )
  csv->haveAFieldResult=0;
  csv->lastDelimiterSetToNull=0;
  csv->lastDelimiterSetToNullPtr=0;
  csv->lastFieldResult=0;
  csv->numberOfDelimitersCounted=0;
  csv->numberOfFields=0;

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


int csvParser_CountNumberOfLines(struct CSVParser * csv,const char * filename)
{
   (void) csv; //Unused , kept for API compatibility
   if (filename==0) { return 0; }
   unsigned int totalLinesCount=0;

   FILE *fp  = fopen(filename,"r");
   int currentCharacter; //int , not char , so a 0xFF byte is not mistaken for EOF ( and EOF is seen where char is unsigned )
   int previousCharacter='\n';

   if (fp != NULL)
   {
    while((currentCharacter = fgetc(fp)) !=  EOF)
       {
        if(currentCharacter == '\n')  { totalLinesCount ++; }
        previousCharacter=currentCharacter;
       }
    if (previousCharacter!='\n') { totalLinesCount ++; } //Last line without a trailing newline
    fclose(fp);
   }
 return totalLinesCount;
}



int csvParser_StartParsingFile(struct CSVParser * csv,const char * filename)
{
    if (csv==0) { fprintf(stderr,"csvParser: csvParserStartParsingFile cannot load file `%s` without an allocated csvParser structure \n",(filename!=0) ? filename : "(null)"); return 0; }
    if (filename==0) { return 0; }

    csvParser_StopParsingFile(csv); //Close any file we were already parsing , and reset the per-line state
    csv->linesParsed=0;

    csv->handle = fopen(filename,"r");
    if (csv->handle!=0)
    {
     return 1;
    }

 return 0;
}



//Closes the file and frees the line buffer , the parser itself stays allocated until csvParserDestroy()
int csvParser_StopParsingFile(struct CSVParser * csv)
{
    if (csv==0) { return 0; }
    if (csv->lastLine!=0) { free(csv->lastLine); csv->lastLine=0; }
    csv->lastLineLength=0;
    csv->haveAFieldResult=0;
    csv->lastDelimiterSetToNull=0;
    csv->lastDelimiterSetToNullPtr=0;
    csv->lastFieldResult=0;
    csv->numberOfDelimitersCounted=0;
    csv->numberOfFields=0;
    if (csv->handle!=0)
    {
       fclose(csv->handle);
       csv->handle=0;
    }
    return 0;
}
