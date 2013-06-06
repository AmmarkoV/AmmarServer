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
};



#endif // FASTSTRINGPARSER_H_INCLUDED
