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

    const char * delimiters;
    unsigned int numberOfDelimiters;
    char numberOfDelimitersCounted;
 };



/** @brief Create a parser , the delimiter string is borrowed ( not copied ) and must outlive the parser */
struct CSVParser *  csvParserCreate(const char * delimiter , unsigned int numberOfDelimiters);
/** @brief Stops parsing ( if needed ) and frees the parser */
int  csvParserDestroy(struct CSVParser * csv);


int csvParser_CountNumberOfLines(struct CSVParser * csv,const char * filename);
int csvParser_StartParsingFile(struct CSVParser * csv,const char * filename);
/** @brief Close the file and free the line buffer , the parser stays allocated and must still be freed with csvParserDestroy() */
int csvParser_StopParsingFile(struct CSVParser * csv);
unsigned int csvParser_GetNumberOfFields(struct CSVParser * csv);
/** @brief Get a field of the current line ( 0 if fieldNumber >= csvParser_GetNumberOfFields() ).
*   The returned string points inside the line buffer and is only valid until the next csvParser_GetField()
*   call for a different field , or csvParser_ParseNextLine() - copy it if you need it longer. */
char * csvParser_GetField(struct CSVParser * csv,unsigned int fieldNumber);
int csvParser_ParseNextLine(struct CSVParser * csv);

#endif // CSVPARSE_H_INCLUDED
