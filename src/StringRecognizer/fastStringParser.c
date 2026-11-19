#include "fastStringParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

struct fastStringParser * fspHTTPHeader = 0;

#define MAXIMUM_FILENAME_WITH_EXTENSION 1024
#define MAXIMUM_LINE_LENGTH 1024
#define MAXIMUM_LEVELS 123
#define ACTIVATED_LEVELS 3

char acceptedChars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";



//Upper-cases letters , turns '-' and '.' into '_' , keeps digits and '_' , and drops everything else ( e.g. ':' ).
//The old in-place loop only got a trailing ':' right , a character in the middle mangled the name ( "A:B" -> "ABB" )
void convertTo_ENUM_ID(char *sPtr)
{
  unsigned int source=0 , target=0;
  for (source=0; sPtr[source]!=0; source++)
   {
     unsigned char c = (unsigned char) toupper((unsigned char) sPtr[source]);
     if ( (c=='-') || (c=='.') )                                    { sPtr[target++]='_'; } else
     if ( (c=='_') || ( (c>='A')&&(c<='Z') ) || ( (c>='0')&&(c<='9') ) ) { sPtr[target++]=(char) c; }
   }
  sPtr[target]=0;
}



int fastStringParser_addString(struct fastStringParser * fsp, char * str)
{
  //TODO : Check here if there are smaller strings with the same prefix as us declared before
  //here if found then swap them with current word

  if ( (fsp==0) || (str==0) ) { return 0; }
  if (fsp->stringsLoaded >= fsp->MAXstringsLoaded) //contents[] has room for MAXstringsLoaded , more used to overflow it
  {
    fprintf(stderr,"Too many strings , only %u fit , cannot add `%s`\n",fsp->MAXstringsLoaded,str);
    return 0;
  }

  unsigned int ourNum = fsp->stringsLoaded++;
  fsp->contents[ourNum].strLength=strlen(str);

  if ( (ourNum==0) || (fsp->shortestStringLength > fsp->contents[ourNum].strLength) )
      { fsp->shortestStringLength = fsp->contents[ourNum].strLength; }

  if ( (ourNum==0) || (fsp->longestStringLength < fsp->contents[ourNum].strLength) )
      { fsp->longestStringLength  = fsp->contents[ourNum].strLength; }

  fsp->contents[ourNum].str = (char *) malloc(sizeof(char) * (fsp->contents[ourNum].strLength+1) );
  if (fsp->contents[ourNum].str != 0 )
  {
    strncpy(fsp->contents[ourNum].str,str,fsp->contents[ourNum].strLength);
    fsp->contents[ourNum].str[fsp->contents[ourNum].strLength]=0; // Null terminator
  } else
  {
    return 0;
  }


  fsp->contents[ourNum].strIDFriendly = (char *) malloc(sizeof(char) * (fsp->contents[ourNum].strLength+1) );
  if (fsp->contents[ourNum].strIDFriendly != 0 )
  {
    strncpy(fsp->contents[ourNum].strIDFriendly,str,fsp->contents[ourNum].strLength);
    fsp->contents[ourNum].strIDFriendly[fsp->contents[ourNum].strLength]=0; // Null terminator
    convertTo_ENUM_ID(fsp->contents[ourNum].strIDFriendly);
  } else
  {
    if (fsp->contents[ourNum].str!=0) { free(fsp->contents[ourNum].str); fsp->contents[ourNum].str=0; }
    return 0;
  }

  return 1;
}

struct fastStringParser *  fastStringParser_initialize(unsigned int totalStrings)
{
   fspHTTPHeader = (struct fastStringParser * ) malloc(sizeof( struct fastStringParser ));
   if (fspHTTPHeader== 0 ) { return 0; }
   memset(fspHTTPHeader,0,sizeof(struct fastStringParser));

   fspHTTPHeader->stringsLoaded = 0;
   fspHTTPHeader->MAXstringsLoaded = totalStrings;
   fspHTTPHeader->contents = (struct fspString * ) calloc(fspHTTPHeader->MAXstringsLoaded+1,sizeof( struct fspString ));
   fspHTTPHeader->shortestStringLength = 0;
   fspHTTPHeader->longestStringLength = 0;

   if (fspHTTPHeader->contents== 0 ) { free(fspHTTPHeader); fspHTTPHeader=0; return 0; }

  return fspHTTPHeader;
}

//Byte count of word i , and 0 past its terminator : comparing a prefix longer than a word used to read past its buffer
static inline char wordCharAt(struct fastStringParser * fsp,unsigned int i,unsigned int position)
{
  return (position<=fsp->contents[i].strLength) ? fsp->contents[i].str[position] : 0;
}


int fastStringParser_hasStringsWithNConsecutiveChars(struct fastStringParser * fsp,unsigned int * resStringResultIndex, char * Sequence,unsigned int seqLength)
{
  int res = 0;
  unsigned int i=0,count = 0,correct=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    correct=0;
    for ( count=0; count<seqLength; count++ )
    {
      if (wordCharAt(fsp,i,count)==Sequence[count]) { ++correct; }
    }


    if ( correct == seqLength ) {
                                   *resStringResultIndex = i;
                                   ++res;
                                   //fprintf(stderr,"Comparing %s with %s : ",Sequence,fsp->contents[i].str);
                                   //fprintf(stderr,"HIT\n");
                                } else
                                { /*fprintf(stderr,"MISS\n");*/ }

  }
  return res ;
}


unsigned int fastStringParser_countStringsForNextChar(struct fastStringParser * fsp,unsigned int * resStringResultIndex,char * Sequence,unsigned int seqLength)
{
 unsigned int res=0;
 Sequence[seqLength+1]=0;
 Sequence[seqLength]='A';

 unsigned int curCh=0;
 while (curCh <= strlen(acceptedChars))
 {
   Sequence[seqLength] =  acceptedChars[curCh];
   res+=fastStringParser_hasStringsWithNConsecutiveChars(fsp,resStringResultIndex,Sequence,seqLength+1);
   ++curCh;
  }

  Sequence[seqLength]=0;
  //fprintf(stderr,"%u strings with prefix %s ( length %u ) \n",res,Sequence,seqLength);

  return res;
}

void addLevelSpaces(FILE * fp , unsigned int level)
{
  int i=0;
  for (i=0; i<level*4; i++)
  {
    fprintf(fp," ");
  }
}



/* The generated scanners compare without toupper() or strncasecmp() : an input byte ORed with 0x20 where the word
 * has a letter has to equal the word's lowercase byte , which is exact ( only 'A' and 'a' turn into 'a' ) , and
 * every other byte has to match as it is. The bytes are compared 8 , 4 , 2 or 1 at a time. */
static void emitByteLiteral(FILE * fp,const char * word,unsigned int offset,unsigned int n,int mask)
{
  unsigned int i=0;
  fprintf(fp,"\"");
  for (i=0; i<n; i++)
  {
    unsigned char c = (unsigned char) word[offset+i];
    unsigned char b = (mask) ? ( isalpha(c) ? 0x20 : 0x00 ) : (unsigned char) tolower(c);
    if ( (mask) || (!isprint(b)) || (b=='"') || (b=='\\') || (b=='?') ) { fprintf(fp,"\\%03o",b); } else
                                                                       { fputc(b,fp); }
  }
  fprintf(fp,"\"");
}

static void emitPiece(FILE * fp,const char * word,unsigned int offset,unsigned int n)
{
  fprintf(fp," && sr_match%u(str+%u,",n,offset);
  emitByteLiteral(fp,word,offset,n,1);
  fprintf(fp,",");
  emitByteLiteral(fp,word,offset,n,0);
  fprintf(fp,")");
}

/* The test for one word : long enough , every byte matching in pieces that may overlap but never go past the
 * word , and , for a word shorter than the bytes the switches above it looked at , the terminator the
 * "case 0" dispatch asks for. */
static void emitWordMatch(FILE * fp,struct fastStringParser * fsp,unsigned int i,unsigned int dispatchedLevels)
{
  const char * w = fsp->contents[i].str;
  unsigned int L = fsp->contents[i].strLength;
  unsigned int at = 0;

  fprintf(fp,"( (strLength>=%u)",L);
  if (L>=8)
  {
    for (at=0; at+8<=L; at+=8) { emitPiece(fp,w,at,8); }
    if (at<L)                  { emitPiece(fp,w,L-8,8); }
  } else
  if (L>=4) { emitPiece(fp,w,0,4); if (L>4) { emitPiece(fp,w,L-4,4); } } else
  if (L>=2) { emitPiece(fp,w,0,2); if (L>2) { emitPiece(fp,w,L-2,2); } } else
  if (L==1) { emitPiece(fp,w,0,1); }

  if (L<dispatchedLevels) { fprintf(fp," && (str[%u]==0)",L); }
  fprintf(fp," )");
}

/* What a switch compares a dispatched character against : the byte ORed with 0x20 , the same fold the switch
 * applies to the input. Within acceptedChars and the terminator no two characters fold to the same value , and
 * whatever else folds onto one of them is turned away by the exact test at the leaf. */
static unsigned int foldedChar(char c) { return ((unsigned int)(unsigned char) c) | 0x20; }

static void emitCaseComment(FILE * fp,char c)
{
  if (c==0) { fprintf(fp," /*Null Terminator*/"); } else { fprintf(fp," /* '%c' */",c); }
}

int printIfAllPossibleStrings(FILE * fp , struct fastStringParser * fsp , char * Sequence,unsigned int seqLength)
{
  unsigned int i=0,count=0 , correct = 0 , results =0 ;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    correct=0;
    for ( count=0; count<seqLength; count++ )
    {
      if (wordCharAt(fsp,i,count)==Sequence[count]) { ++correct; }
    }

    if ( correct == seqLength ) {
                                  addLevelSpaces(fp , seqLength);
                                  if (results>0) { fprintf(fp," else "); }
                                  fprintf(fp," if ");
                                  emitWordMatch(fp,fsp,i,seqLength);
                                  fprintf(fp," { return %s_%s; } /* %s */ \n",
                                          fsp->functionName,
                                          fsp->contents[i].strIDFriendly,
                                          fsp->contents[i].str );

                                  ++results;
                                }
  }
  return 1;
}



int printAllEnumeratorItems(FILE * fp , struct fastStringParser * fsp,char * functionName)
{
  fprintf(fp,"enum { \n");
  fprintf(fp," %s_EMPTY=0,\n",fsp->functionName);

  unsigned int i=0;
  for (i=0; i<fsp->stringsLoaded; i++)
  {
    fprintf(fp," %s_%s, // %u \n",fsp->functionName,fsp->contents[i].strIDFriendly,i+1);
  }

  fprintf(fp," %s_END_OF_ITEMS\n",fsp->functionName);

  fprintf(fp,"};\n\n");

  return 1;
}



int recursiveTraverser(FILE * fp,struct fastStringParser * fsp,char * functionName,char * cArray,unsigned int level)
{
  if (level>=ACTIVATED_LEVELS) { return 0; }

  unsigned int resStringResultIndex=0;
  unsigned int nextLevelStrings=fastStringParser_countStringsForNextChar(fsp,&resStringResultIndex,cArray,level);

  if ( (nextLevelStrings>1) && (level==0) && (ACTIVATED_LEVELS>=2) && (fsp->shortestStringLength>=2) )
     {
      /* Levels 0 and 1 in one switch on the first two bytes , both folded , instead of a switch per byte. Every
       * word is at least two bytes long , so both bytes are within strLength. */
      unsigned int curCh0=0,curCh1=0;
      fprintf(fp," switch ( ( ( ((unsigned int)(unsigned char) str[0]) << 8 ) | ((unsigned char) str[1]) ) | 0x2020 ) { \n");
      for (curCh0=0; curCh0<=strlen(acceptedChars); curCh0++)
       {
        cArray[0]=acceptedChars[curCh0]; cArray[1]=0;
        if (!fastStringParser_hasStringsWithNConsecutiveChars(fsp,&resStringResultIndex,cArray,1)) { continue; }

        unsigned int level1Strings=fastStringParser_countStringsForNextChar(fsp,&resStringResultIndex,cArray,1);
        if (level1Strings==1)
        {
          //One word starts with this byte : its own second byte is all the switch needs
          char c1 = fsp->contents[resStringResultIndex].str[1];
          fprintf(fp," case 0x%04x :",(foldedChar(cArray[0])<<8)|foldedChar(c1));
          emitCaseComment(fp,cArray[0]); emitCaseComment(fp,c1); fprintf(fp," \n");
          addLevelSpaces(fp,1);
          fprintf(fp," if ");
          emitWordMatch(fp,fsp,resStringResultIndex,2);
          fprintf(fp," { return %s_%s; } /* %s */ \n",fsp->functionName,fsp->contents[resStringResultIndex].strIDFriendly,fsp->contents[resStringResultIndex].str);
          fprintf(fp," break; \n");
        } else
        for (curCh1=0; curCh1<=strlen(acceptedChars); curCh1++)
        {
          cArray[1]=acceptedChars[curCh1]; cArray[2]=0;
          if (!fastStringParser_hasStringsWithNConsecutiveChars(fsp,&resStringResultIndex,cArray,2)) { continue; }
          fprintf(fp," case 0x%04x :",(foldedChar(cArray[0])<<8)|foldedChar(cArray[1]));
          emitCaseComment(fp,cArray[0]); emitCaseComment(fp,cArray[1]); fprintf(fp," \n");
          if (ACTIVATED_LEVELS>2) { recursiveTraverser(fp,fsp,functionName,cArray,2); } else
                                  { printIfAllPossibleStrings(fp,fsp,cArray,2); }
          fprintf(fp," break; \n");
        }
       }
      cArray[0]=0; cArray[1]=0;
      fprintf(fp,"}; \n");
     } else
  if ( nextLevelStrings>1 )
     {
      unsigned int cases=0;
      addLevelSpaces(fp , level);
      fprintf(fp," switch ( ((unsigned char) str[%u]) | 0x20 ) { \n",level);

      cArray[level]='A';
      cArray[level+1]=0;
      //TODO: Add '-' character for strings like IF-MODIFIED-ETC
      unsigned int curCh=0;
      while (curCh <= strlen(acceptedChars))
       {
        cArray[level]=acceptedChars[curCh];
        if ( fastStringParser_hasStringsWithNConsecutiveChars(fsp,&resStringResultIndex,cArray,level+1)  )
        {
          addLevelSpaces(fp , level);

          fprintf(fp," case 0x%02x :",foldedChar(cArray[level]));
          emitCaseComment(fp,cArray[level]); fprintf(fp," \n");
           if ( level < ACTIVATED_LEVELS-1 ) { recursiveTraverser(fp,fsp,functionName,cArray,level+1); } else
                                             { printIfAllPossibleStrings(fp , fsp , cArray, level+1); }
          addLevelSpaces(fp , level);
          fprintf(fp," break; \n");
          ++cases;
        }

         ++curCh;
       }
       cArray[level]=0;


       if (cases==0) { fprintf(fp,"//BUG :  nextLevelStrings were supposed to be non-zero"); }

       addLevelSpaces(fp , level);
       fprintf(fp,"}; \n");
     } else
     if ( nextLevelStrings==1 )
     {
       addLevelSpaces(fp , level);
       fprintf(fp," if ");
       emitWordMatch(fp,fsp,resStringResultIndex,level);
       fprintf(fp," { return %s_%s; } /* %s */ \n",
                   fsp->functionName,
                   fsp->contents[resStringResultIndex].strIDFriendly,
                   fsp->contents[resStringResultIndex].str );
     }
     else
     {
       fprintf(fp," //Error ( %s ) \n",fsp->contents[resStringResultIndex].str);
     }

 return 1;
}




int export_C_Scanner(struct fastStringParser * fsp,char * functionName)
{
  if (fsp==0) { fprintf(stderr,"export_C_Scanner called with empty string parser\n"); return 0; }
  if (functionName==0) { fprintf(stderr,"export_C_Scanner called with empty function name\n"); return 0; }

  unsigned int functionNameLength = strlen(functionName);
  fsp->functionName  = (char* ) malloc(sizeof(char) * (1+functionNameLength));
  if (fsp->functionName==0) { fprintf(stderr,"Could not allocate memory for function name\n"); return 0; }
  strncpy(fsp->functionName,functionName,functionNameLength);
  fsp->functionName[functionNameLength]=0;

  convertTo_ENUM_ID(fsp->functionName);


  char filenameWithExtension[MAXIMUM_FILENAME_WITH_EXTENSION+1]={0};


  //PRINT OUT THE HEADER
  snprintf(filenameWithExtension,MAXIMUM_FILENAME_WITH_EXTENSION,"%s.h",functionName);
  FILE * fp = fopen(filenameWithExtension,"wb");
  if (fp == 0) { fprintf(stderr,"Could not open output file %s\n",functionName); return 0; }
  fflush(fp);
  fprintf(fp,"\n");


  fprintf(fp,"/** @file %s.h\n",functionName);
  fprintf(fp,"* @brief A tool that scans for a string in a very fast and robust way\n");
  fprintf(fp,"* @author Ammar Qammaz (AmmarkoV)\n");
  fprintf(fp,"*/\n\n");

  fprintf(fp,"#ifndef %s_H_INCLUDED\n",fsp->functionName);
  fprintf(fp,"#define %s_H_INCLUDED\n\n\n",fsp->functionName);


      fprintf(fp,"/** @brief Enumerator for the IDs of %s so we can know what the result was*/\n",functionName);
      printAllEnumeratorItems(fp, fsp, functionName);


  fprintf(fp,"\n\n/** @brief Scan a string for one of the words of the %s word set\n",functionName);
  fprintf(fp,"* @ingroup stringParsing\n");
  fprintf(fp,"* @param Input String , to be scanned\n");
  fprintf(fp,"* @param Length of Input String\n");
  fprintf(fp,"* @retval See above enumerator*/\n");
  fprintf(fp," int scanFor_%s(const char * str,unsigned int strLength); \n\n",functionName);
  fprintf(fp,"#endif\n");
  fclose(fp);


  //PRINT OUT THE MAIN FILE

  snprintf(filenameWithExtension,MAXIMUM_FILENAME_WITH_EXTENSION,"%s.c",functionName);
  fp = fopen(filenameWithExtension,"wb");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",functionName); return 0; }

  fflush(fp);
  fprintf(fp,"\n");

  char cArray[MAXIMUM_LEVELS]={0};
  int i=0;
  for (i=0; i<MAXIMUM_LEVELS; i++ ) { cArray[i]=0;/*'A';*/ }


  time_t t = time(NULL);
  struct tm tm = *localtime(&t);


  fprintf(fp,"/* \
                 \nThis file was automatically generated @ %02d-%02d-%02d %02d:%02d:%02d using StringRecognizer \
                 \nhttps://github.com/AmmarkoV/AmmarServer/tree/master/src/StringRecognizer\
                 \nPlease note that changes you make here may be automatically overwritten \
                 \nif the String Recognizer generator runs again..!\
              \n */ \n\n" ,
          tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);


  fprintf(fp,"#include <stdint.h>\n");
  fprintf(fp,"#include <string.h>\n");
  fprintf(fp,"#include \"%s.h\"\n\n",functionName);

  fprintf(fp,"/* Case-insensitive compare of 8 , 4 , 2 or 1 bytes : mask has 0x20 where the word has a letter , so an\n");
  fprintf(fp,"   input byte ORed with it has to equal the word's lowercase byte , and every other byte has to match exactly. */\n");
  fprintf(fp,"static inline int sr_match8(const char * s,const char * mask,const char * want) { uint64_t a,m,w; memcpy(&a,s,8); memcpy(&m,mask,8); memcpy(&w,want,8); return (a|m)==w; }\n");
  fprintf(fp,"static inline int sr_match4(const char * s,const char * mask,const char * want) { uint32_t a,m,w; memcpy(&a,s,4); memcpy(&m,mask,4); memcpy(&w,want,4); return (a|m)==w; }\n");
  fprintf(fp,"static inline int sr_match2(const char * s,const char * mask,const char * want) { uint16_t a,m,w; memcpy(&a,s,2); memcpy(&m,mask,2); memcpy(&w,want,2); return (a|m)==w; }\n");
  fprintf(fp,"static inline int sr_match1(const char * s,const char * mask,const char * want) { return (unsigned char)(*s|*mask)==(unsigned char)*want; }\n\n");

  fprintf(fp,"int scanFor_%s(const char * str,unsigned int strLength) \n{\n",functionName);

     fprintf(fp," if (str==0) { return 0; } \n");
     fprintf(fp," if (strLength<%u) { return 0; } \n\n",fsp->shortestStringLength);
     recursiveTraverser(fp,fsp,functionName,cArray,0);

  fprintf(fp," return 0;\n");
  fprintf(fp,"}\n");

  /*
  fprintf(fp,"\n\nint main(int argc, char *argv[]) \n {\n");
  fprintf(fp,"  if (argc<1) { fprintf(stderr,\"No parameter\\n\"); return 1; }\n");
  fprintf(fp,"  if ( scanFor_%s(argv[0]) ) { fprintf(stderr,\"Found it\"); } \n  return 0; \n }\n",functionName);
*/

  fclose(fp);

  return 1;
}






struct fastStringParser * fastSTringParser_createRulesFromFile(const char* filename,unsigned int totalStrings)
{
  FILE * fp = fopen(filename,"r");
  if (fp == 0) { fprintf(stderr,"Could not open input file %s\n",filename); return 0; }

  struct fastStringParser *  fsp  = fastStringParser_initialize(totalStrings);
  if (fsp==0) { fclose(fp); return 0; }

  char line[MAXIMUM_LINE_LENGTH]={0};
  unsigned int lineLength=0;
  while (fgets(line,MAXIMUM_LINE_LENGTH,fp)!=0)
  {
      lineLength = strlen(line);
      if ( lineLength > 0 )
        {
         if (line[lineLength-1]==10) { line[lineLength-1]=0; --lineLength; } else
         if (line[lineLength-1]==13) { line[lineLength-1]=0; --lineLength; }
        }
      if ( lineLength > 1 )
        {
         if (line[lineLength-2]==10) { line[lineLength-2]=0; --lineLength; } else
         if (line[lineLength-2]==13) { line[lineLength-2]=0; --lineLength; }
        }

    //fprintf(stderr,"LINE : `%s`\n",line);
    if (lineLength==0) { continue; } //A blank line would become an empty word with an empty enum name
    if (!fastStringParser_addString(fsp,line)) { fclose(fp); fastStringParser_close(fsp); return 0; }
  }
  fclose(fp);

  if (fsp->stringsLoaded==0) { fprintf(stderr,"No strings in %s\n",filename); fastStringParser_close(fsp); return 0; }

  return fsp;
}





int fastStringParser_close(struct fastStringParser * fsp)
{
    if (fsp==0) { return 0; }
    unsigned int i=0;
    if (fsp->contents!=0)
    {
      for (i=0; i<fsp->stringsLoaded; i++)
      {
        free(fsp->contents[i].str);
        free(fsp->contents[i].strIDFriendly);
      }
      free(fsp->contents);
    }
    free(fsp->functionName);
    if (fsp==fspHTTPHeader) { fspHTTPHeader=0; }
    free(fsp);
    return 1;
}




