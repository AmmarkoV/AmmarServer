#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int repairFilenameForC(char * io)
{
  unsigned int i=0;
  for (i=0; i<strlen(io); i++)
  {
    if (io[i]=='.')  { io[i]='_'; } else
    if (io[i]==';')  { io[i]='_'; } else
    if (io[i]=='/')  { io[i]='_'; } else
    if (io[i]=='\\') { io[i]='_'; }
  }

 return 1;
}


int embedFile(char * filename, char * output)
{
    FILE * file = fopen(filename, "rb");
    FILE * out = fopen(output, "w");


    repairFilenameForC(filename);


    unsigned char buffer[32];
    size_t count;
    fprintf(out, "#pragma once\n static unsigned char %s[] = { ",filename);
    while(!feof(file)){
            count = fread(buffer, 1, 32, file);

            for(int n = 0; n < count; ++n){
                    fprintf(out, "0x%02X, ", buffer[n]);
            };
    };
    fprintf(out, "};");
    fclose(file);
    fclose(out);
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
