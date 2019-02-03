#include <stdio.h>     /* files */
#include <stdlib.h>     /* qsort */
#include <string.h>     /* memset */

#include "csvparse.h"

int main()
{
  unsigned int lineCounter=0;
  struct CSVParser *  csvParser = csvParserCreate(";",1);
  if (csvParser!=0)
  {
    if (csvParser_StartParsingFile(csvParser,"test.csv"))
    {
       while (csvParser_ParseNextLine(csvParser))
       {
         unsigned int i;
         for (i=0; i<csvParser_GetNumberOfFields(csvParser); i++)
         {
           if (csvParser_GetField(csvParser,i))
           {
             fprintf(stderr,"!!LINE %u/FIELD %u : %s\n",lineCounter,i,csvParser_GetField(csvParser,i));
           } else
           { fprintf(stderr,"Unable to get field %u for line %u\n",i,lineCounter); }
         }
         fprintf(stderr,"\n");
         ++lineCounter;
       }
      csvParser_StopParsingFile(csvParser);
    } else
    { fprintf(stderr,"Could not open csv file..\n"); }
   csvParserDestroy(csvParser);
  } else
  { fprintf(stderr,"Could not allocate csvParser object..\n"); }

 fprintf(stderr,"Finished..\n");
 return 0;
}
