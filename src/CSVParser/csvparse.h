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


    char * lastLine;

    char * filename;
    unsigned int progress;
 };


#endif // HASHMAP_H_INCLUDED
