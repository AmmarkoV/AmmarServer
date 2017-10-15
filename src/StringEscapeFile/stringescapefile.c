#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *replaceEscapedChar(const char *s, char ch, const char *repl) {
    int count = 0;
    const char *t;
    for(t=s; *t; t++)
        count += (*t == ch);

    size_t rlen = strlen(repl);
    char *res = malloc(strlen(s) + (rlen-1)*count + 1);
    char *ptr = res;
    for(t=s; *t; t++)
        {
          if(*t == ch) {
                         memcpy(ptr, repl, rlen);
                         ptr += rlen;
                       } else
                       {
                          *ptr++ = *t;
                       }
        }
    *ptr = 0;
    return res;
}


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

        int cleaned=0;
        char * firstStep  = replaceEscapedChar(line,'"',"\\\"");
        if (firstStep!=0)
        {
         char * secondStep = replaceEscapedChar(firstStep,'%',"%%");
         if (secondStep!=0)
         {
           fprintf(op,"%s\\\n",secondStep);
           free(secondStep); ++cleaned;
         }
         free(firstStep); ++cleaned;
        }

        if (cleaned!=2)
        {
           fprintf(op,"%s\\\n",line);
        }
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
