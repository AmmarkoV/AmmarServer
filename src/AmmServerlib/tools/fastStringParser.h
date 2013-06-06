#ifndef FASTSTRINGPARSER_H_INCLUDED
#define FASTSTRINGPARSER_H_INCLUDED


struct fspString
{
  unsigned short letterFirstOccurance[128];
  unsigned short letterLastOccurance[128];
  char * str;
  unsigned int strLength;

};


struct fastStringParser
{
  struct fspString * contents;
  unsigned int stringsLoaded;
  unsigned int MAXstringsLoaded;

  unsigned int shortestStringLength;
  unsigned int longestStringLength;
};

int export_C_Scanner(struct fastStringParser * fsp,char * filename);

struct fastStringParser * fastSTringParser_createRulesFromFile(char* filename,unsigned int totalStrings);

#endif // FASTSTRINGPARSER_H_INCLUDED
