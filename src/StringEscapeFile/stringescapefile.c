#include <stdio.h>
#include <stdlib.h>


int escapeFile(const char * inputFile, const char * outputFile)
{
 FILE *fp =0 , * op=0;
 fp=fopen(inputFile,"r");
 op=fopen(outputFile,"w");
 if ((fp!=0) && (op!=0) )
 {
   ssize_t read;
   char * line = NULL;
   size_t len = 0;

    fprintf(op,"\"");
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if ( ( line[read-1]==10 ) || ( line[read-1]==13 ) ) { line[read-1]=0;  }
        if ( ( line[read-2]==10 ) || ( line[read-2]==13 ) ) { line[read-2]=0;  }

        fprintf(op,"%s\\\n",line);
    }
    fprintf(op,"\"\n");

   fclose(fp);
   fclose(op);
   return 1;
 }
 return 0;
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
