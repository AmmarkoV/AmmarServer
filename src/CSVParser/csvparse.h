/** @file csvparse.h
* @brief A uniform and clean way to parse CSV files in C
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef CSVPARSE_H_INCLUDED
#define CSVPARSE_H_INCLUDED

#include <stdio.h>

 struct CSVParser
 {
    FILE * handle;
    unsigned int fileSize;
    unsigned int linesParsed;

    unsigned int numberOfFields;

    unsigned int fieldIDOflastDelimiter;
    unsigned int lastDelimiterSetToNull;
    char * lastDelimiterSetToNullPtr;
    char previousDelimiterValue;
    char * lastFieldResult;
    int haveAFieldResult;

    char * header;
    size_t headerLength;



    char * lastLine;
    size_t lastLineLength;

    char * filename;
    unsigned int progress;

    char * delimiters;
    unsigned int numberOfDelimiters;
    char numberOfDelimitersCounted;
 };



struct CSVParser *  csvParserCreate(char * delimiter , unsigned int numberOfDelimiters);
int  csvParserDestroy(struct CSVParser * csv);


int csvParser_CountNumberOfLines(struct CSVParser * csv,const char * filename);
int csvParser_StartParsingFile(struct CSVParser * csv,const char * filename);
int csvParser_StopParsingFile(struct CSVParser * csv);
unsigned int csvParser_GetNumberOfFields(struct CSVParser * csv);
char * csvParser_GetField(struct CSVParser * csv,unsigned int fieldNumber);
int csvParser_ParseNextLine(struct CSVParser * csv);

#endif // HASHMAP_H_INCLUDED
