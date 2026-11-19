#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Writes one line as the inside of a C string literal :  \ -> \\  ( used to become a C escape , e.g. <\/title> warned
//and \n turned into a newline ) , " -> \"  and  % -> %%  ( the generated strings are used as printf formats )
static void writeEscapedLine(FILE * op,const char * line)
{
  const char * t;
  for (t=line; *t; t++)
  {
    if (*t=='\\') { fputs("\\\\",op); } else
    if (*t=='"')  { fputs("\\\"",op); } else
    if (*t=='%')  { fputs("%%",op);   } else
                  { fputc(*t,op);     }
  }
}


int escapeFile(const char * inputFile, const char * outputFile)
{
 //Open the input first , opening the output truncates it and a missing input used to wipe an existing output
 FILE * fp=fopen(inputFile,"r");
 if (fp==0) { fprintf(stderr,"Could not open input file %s\n",inputFile); return 0; }
 FILE * op=fopen(outputFile,"w");
 if (op==0) { fprintf(stderr,"Could not open output file %s\n",outputFile); fclose(fp); return 0; }

   ssize_t read;
   char * line = NULL;
   size_t len = 0;

    fprintf(op,"\"");
    while ((read = getline(&line, &len, fp)) != -1)
    {
        //Strip the line ending ( \n , \r\n ) , a blank line has read==1 and used to read line[-1]
        while ( (read>0) && ( (line[read-1]==10) || (line[read-1]==13) ) ) { line[--read]=0; }

        writeEscapedLine(op,line);
        fprintf(op,"\\\n");
    }
    fprintf(op,"\"\n");
    free(line);

   int readFailed  = ferror(fp);
   fclose(fp);
   int writeFailed = ferror(op);
   if (fclose(op)!=0) { writeFailed=1; }

   if (readFailed)  { fprintf(stderr,"Could not read input file %s\n",inputFile); }
   if (writeFailed) { fprintf(stderr,"Could not write output file %s\n",outputFile); }
   if ( (readFailed) || (writeFailed) ) { remove(outputFile); return 0; } //Don't leave a truncated string behind
   return 1;
}


int main(int argc, char *argv[])
{
    if (argc<3)
    {
      fprintf(stderr,"Not enough arguments , stringescapefile inputFile.in outputFile.out\n");
      return 1;
    }

    printf("Dumping File to Escaped file!\n");
    if ( escapeFile(argv[1],argv[2]) )
       {
        printf("Success!\n");
        return 0;
       } else
       {
        fprintf(stderr,"Could not do conversion..\n");
       }
    return 1;
}
