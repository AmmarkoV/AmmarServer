#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Turns a path into a valid C identifier : anything that can't appear in one ( . / \ ; - space .. ) becomes '_' ,
//and a leading digit gets a '_' prefix. Caller frees the result.
char * repairFilenameForC(const char * filename)
{
  size_t length = strlen(filename);
  char * identifier = (char*) malloc(length+2);
  if (identifier==0) { return 0; }

  char * o = identifier;
  if ( (filename[0]>='0') && (filename[0]<='9') ) { *o++='_'; }
  size_t i=0;
  for (i=0; i<length; i++)
  {
    char c = filename[i];
    int valid = ( (c>='a')&&(c<='z') ) || ( (c>='A')&&(c<='Z') ) || ( (c>='0')&&(c<='9') ) || (c=='_');
    *o++ = (valid) ? c : '_';
  }
  *o=0;
 return identifier;
}


int embedFile(const char * filename,const char * output)
{
    FILE * file = fopen(filename, "rb");
    if (file==0) { fprintf(stderr,"Could not open input file %s\n",filename); return 0; }

    char * identifier = repairFilenameForC(filename);
    if (identifier==0) { fclose(file); return 0; }

    FILE * out = fopen(output, "w");
    if (out==0) { fprintf(stderr,"Could not open output file %s\n",output); free(identifier); fclose(file); return 0; }

    unsigned char buffer[32];
    size_t count;
    fprintf(out, "#pragma once\n static unsigned char %s[] = { ",identifier);
    free(identifier);
    //Loop on what fread returns , a read error ( e.g. the input is a directory ) never sets feof() and used to loop forever
    while ( (count = fread(buffer, 1, sizeof(buffer), file)) > 0 )
    {
            size_t n;
            for(n = 0; n < count; ++n){
                    fprintf(out, "0x%02X, ", buffer[n]);
            };
    };
    fprintf(out, "};");

    int readFailed  = ferror(file);
    fclose(file);
    int writeFailed = ferror(out);
    if (fclose(out)!=0) { writeFailed=1; }

    if (readFailed)  { fprintf(stderr,"Could not read input file %s\n",filename); }
    if (writeFailed) { fprintf(stderr,"Could not write output file %s\n",output); }
    if ( (readFailed) || (writeFailed) ) { remove(output); return 0; } //Don't leave a truncated header behind
    return 1;
};




int main(int argc, char *argv[])
{
    if (argc<3)
    {
      fprintf(stderr,"Not enough arguments , filetostring inputFile.in outputFile.out\n");
      return 1;
    }

    printf("Dumping File to Escaped file!\n");
    if ( embedFile(argv[1],argv[2]) )
       {
        printf("Success!\n");
        return 0;
       } else
       {
        fprintf(stderr,"Could not do conversion..\n");
       }
    return 1;
}
