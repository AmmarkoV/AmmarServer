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
    //linesParsed
   return 0;
}

int csvParser_StartParsingFile(struct CSVParser * csv,const char * filename)
{
    if (csv==0) { fprintf(stderr,"HashMap: csvParserStartParsingFile cannot load file `%s` without an allocated hashmap structure \n",filename); return 0; }
    return 0;
    /*
    FILE * pFile;
    pFile = fopen (filename,"rb");
    if (pFile!=0)
    {
      //TODO IMPLEMENT!
     fclose (pFile);
     return 0;
    }
    */


  csv->handle = fopen(filename,"r");
  if (csv->handle!=0)
  {
    return 1;
  }







    return 0;
}



int csvParser_ParseNextLine(struct CSVParser * csv)
{
  if (csv->handle!=0)
  {
    if (csv->lastLine!=0) { free(csv->lastLine); csv->lastLine=0; }

    char str[512];
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    if ((read = getline(&line, &len, csv->handle)) != -1)
    {
      ++csv->linesParsed;
      csv->lastLine=0;
      return 1;
    }
  }
  return 0;
}
